#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
int main(void){
 for(unsigned dual=0;dual<2;dual++){
  core_init();core_parallel_set(dual);core_settings c;pw_preset_get(0,&c);
  assert(c.voice_limit==4);c.unison_count=1;c.trigger_mode=0;c.bloom_mode=c.playing=0;c.env=(pw_env_params){0,0,127,50};core_set(&c);
  int32_t pcm[512];for(unsigned warm=0;warm<200;warm++)core_render(pcm,256,4096);
  for(unsigned i=0;i<4;i++)core_midi(0,0x90,48+i*2,100);
  double energy[4]={0};
  for(unsigned b=0;b<20;b++){core_render(pcm,256,4096);for(unsigned i=0;i<4;i++)for(unsigned j=0;j<voice_job.count;j++)energy[i]+=fabsf(voice_job.output[i][j][0])+fabsf(voice_job.output[i][j][1]);}
  assert(core_primary_count()==4);for(unsigned i=0;i<4;i++)assert(energy[i]>0);
  core_midi(0,0x90,72,100);core_render(pcm,256,4096);assert(core_primary_count()==4);
  assert(pw_preset_apply(28,&c));assert(c.voice_limit==4);c.voice_limit=8;core_set(&c);assert(st.voice_limit==4);
  c.voice_limit=4;core_set(&c);core_render(pcm,256,4096);assert(core_primary_count()<=4);
 }
 puts("PASS: four audible voice slots in single/dual mode, fifth-note cap, preset retention, clamp and four-voice maximum.");
}
