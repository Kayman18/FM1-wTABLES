/* TEST70: WAVE POS / BASE WAV per-sample matrix routing stress. */
#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static int32_t pcm[512];
static uint32_t run_case(unsigned count,unsigned dest){
 core_init();core_parallel_set(1);
 core_settings s;pw_preset_default(&s);
 s.bank=0;s.position=42;s.level=78;s.voice_limit=(uint8_t)count;
 s.env=(pw_env_params){0,8,125,40};
 s.lfo.wave=45;s.lfo.rate=35;s.wave_env_depth=0;s.wave_env2_depth=0;
 s.filter.mode=PW_FILTER_LP;s.filter.mix=100;
 s.fx_reverb=1;s.reverb_model=3;s.reverb_mix=85;s.reverb_time_ms=9000;
 for(unsigned k=0;k<4;k++){s.matrix_source[k]=s.matrix_dest[k]=0;s.matrix_amount[k]=0;}
 s.matrix_source[0]=1;s.matrix_dest[0]=(uint8_t)dest;s.matrix_amount[0]=72;
 core_set(&s);
 for(unsigned i=0;i<count;i++)core_midi(0,0x90,(uint8_t)(48+i*3),100);
 uint32_t hash=2166136261u;
 for(unsigned b=0;b<180;b++){
  if(b==40){s.matrix_amount[0]=95;core_set(&s);}
  if(b==90){s.matrix_amount[0]=20;core_set(&s);}
  if(b==130){s.matrix_amount[0]=0;core_set(&s);}
  core_render(pcm,256,4096);
  assert(!core_worker_fault());
  if(b==8||b==42){assert(voice_job.matrix_position_route==(dest==3||dest==4));}
  for(unsigned i=0;i<512;i++){
   int64_t p=pcm[i];assert(p>=-8388608&&p<=8388608);
   hash^=(uint32_t)p;hash*=16777619u;
  }
 }
 assert(core_primary_count()<=count);
 return hash;
}
int main(void){
 assert(CORE_MAX_VOICES==8 && CORE_DEFAULT_VOICES==4 && VOICE_CHUNK==64);
 for(unsigned voices=4;voices<=6;voices+=2){
  uint32_t no=run_case(voices,0),cut=run_case(voices,1),res=run_case(voices,2),wave=run_case(voices,3),base=run_case(voices,4),pitch=run_case(voices,5),depth=run_case(voices,6);
  assert(no!=wave&&no!=base&&cut!=wave&&res!=base);
  printf("PASS TEST70: %u voices Clouds + 7 matrix destinations; wave %08x base %08x filter %08x resonance %08x\n",voices,wave,base,cut,res);
  (void)pitch;(void)depth;
 }
 return 0;
}
