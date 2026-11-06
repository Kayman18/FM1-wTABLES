#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t render_sig(unsigned wave,unsigned dest,unsigned amount,unsigned *matrix_overridden){
 core_init();core_parallel_set(0);
 core_settings c;assert(pw_preset_get(0,&c));
 c.bank=5;c.position=40;c.level=90;c.lfo.rate=80;c.lfo.wave=(int8_t)wave;
 c.lfo_phase_mode=1;c.unison_count=1;c.bloom_mode=0;c.delay_mode=0;c.fx_reverb=0;
 c.env=(pw_env_params){0,5,127,20};c.wave_env_depth=0;c.wave_env2_depth=0;
 c.matrix_source[0]=1;c.matrix_dest[0]=dest;c.matrix_amount[0]=(int8_t)amount;
 assert(pw_matrix_wave_depth_assigned(&c)==(dest==6));
 core_set(&c);core_midi(0,0x90,60,110);
 int32_t pcm[512];uint32_t hash=2166136261u;
 for(unsigned b=0;b<140;b++){
  core_render(pcm,256,4096);
  for(unsigned i=0;i<512;i++){hash^=(uint32_t)pcm[i];hash*=16777619u;}
  assert(!core_worker_fault());
  if(b==5){*matrix_overridden=voice_job.matrix_wave_override;assert(voice_job.matrix_active==(dest==6));}
 }
 return hash;
}
int main(void){
 core_settings c;assert(pw_preset_get(0,&c));
 for(unsigned i=0;i<4;i++){c.matrix_source[i]=0;c.matrix_dest[i]=0;c.matrix_amount[i]=0;}
 c.matrix_source[0]=1;c.matrix_dest[0]=6;c.matrix_amount[0]=0;
 assert(pw_matrix_wave_depth_assigned(&c));
 pw_matrix m;pw_matrix_init(&m);
 assert(pw_matrix_active(&m,&c));
 c.matrix_source[0]=0;assert(!pw_matrix_wave_depth_assigned(&c));
 assert(!pw_matrix_active(&m,&c));
 assert(!strcmp(pw_matrix_dest_name(6),"WAVE DEPTH %"));
 assert(!strcmp(pw_matrix_dest_name(7),"NONE"));
 unsigned o0=0,o1=0;
 uint32_t yes0=render_sig(0,6,0,&o0),yes100=render_sig(100,6,0,&o1);
 assert(o0==1&&o1==1);assert(yes0==yes100); // zero matrix amount overrides local knob
 uint32_t signal0=render_sig(0,6,100,&o0),signal100=render_sig(100,6,100,&o1);
 assert(signal0==signal100); // active matrix route overrides local knob
 uint32_t no0=render_sig(0,0,0,&o0),no100=render_sig(100,0,0,&o1);
 assert(o0==0&&o1==0);assert(no0!=no100); // original manual LFO depth is intact
 printf("PASS TEST61: matrix Wave Depth override (amount 0 and 100), fallback knob preserved; hashes %08x %08x\n",yes0,signal0);
 return 0;
}
