#ifdef REFERENCE71
#include "reference71/test_engine.c"
#else
#include "../fm1/app/test_engine.c"
#endif
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
static int32_t pcm[512];
static void warm(void){for(unsigned i=0;i<200;i++)core_render(pcm,256,4096);}
int main(int argc,char **argv){
 FILE *out=argc>1?fopen(argv[1],"wb"):0;assert(argc==1||out);
 for(unsigned voices=4;voices<=6;voices+=2)for(unsigned dual=0;dual<2;dual++)for(unsigned mode=0;mode<3;mode++){
  core_init();core_parallel_set(dual);core_settings s;pw_preset_default(&s);
  s.voice_limit=voices;s.unison_count=mode==2?2:1;s.bank=mode*3;
  s.position=55;s.env=(pw_env_params){0,30,127,100};s.env2=(pw_env_params){10,40,80,70};
  s.wave_quality=1;s.lfo.rate=45;s.lfo.wave=35;s.wave_env_depth=20;s.wave_env2_depth=-15;
  s.filter=(pw_filter_params){PW_FILTER_LP,80,40,100};
  s.fx_reverb=1;s.reverb_model=3;s.reverb_mix=70;s.reverb_time_ms=9000;s.chorus_mode=2;
  s.tracking.velocity_wave=25;s.tracking.pitch_wave=-10;s.unison_wave_spread=45;
  for(unsigned r=0;r<4;r++){s.matrix_source[r]=1+r*3;s.matrix_dest[r]=r==0?3:r==1?5:r==2?6:1;s.matrix_amount[r]=r==1?-87:90;}
  if(mode==1){s.lfo_pitch_cents=130;s.env2_pitch=15;s.fine_tune=7;}
  if(mode==2){s.bloom_mode=1;s.playing=1;s.bloom_branches=7;s.bloom_path=117;s.bloom_mutation=100;s.bloom_ratchet=3;s.sequencer_bpm=210;}
  core_set(&s);warm();
  if(mode!=2)for(unsigned i=0;i<voices;i++)core_midi(0,0x90,48+i*3,100);
  uint32_t hash=2166136261u;clock_t start=clock();
  for(unsigned b=0;b<1800;b++){
   if(b==450){s.matrix_amount[0]=-90;s.matrix_amount[1]=100;core_set(&s);}
   if(b==900){s.matrix_amount[2]=-100;s.tracking.pitch_wave=80;core_set(&s);}
   if(b==1200){s.matrix_dest[0]=4;s.lfo_pitch_cents=0;s.env2_pitch=0;s.fine_tune=0;core_set(&s);}
   if(mode!=2&&b%100==99){for(unsigned i=0;i<voices;i++)core_midi(0,0x90,48+i*3+(b%7),100);}
   core_render(pcm,256,4096);assert(!core_worker_fault());assert(core_primary_count()<=voices);
   for(unsigned k=0;k<512;k++){assert(pcm[k]>=-8388608&&pcm[k]<=8388607);hash^=(uint32_t)pcm[k];hash*=16777619u;}
   if(out)assert(fwrite(pcm,sizeof(pcm),1,out)==1);
  }
  double elapsed=(double)(clock()-start)/CLOCKS_PER_SEC;
  printf("voices=%u dual=%u mode=%u hash=%08x cpu_seconds=%.6f\n",voices,dual,mode,hash,elapsed);
 }
 if(out)assert(!fclose(out));
 return 0;
}
