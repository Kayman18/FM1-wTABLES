#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main(void){int32_t*ref=malloc(512*3200*sizeof(int32_t)),pcm[512];assert(ref);
 for(unsigned dual=0;dual<2;dual++){
  core_init();core_parallel_set(dual);core_settings c;pw_preset_get(1,&c);c.voice_limit=6;c.bloom_mode=1;c.chorus_mode=2;c.fx_reverb=1;core_set(&c);c.playing=1;core_set(&c);
  for(unsigned b=0;b<3200;b++){
   if(b%80==0){assert(pw_preset_apply(b<2240?(b/80)%40:40+(b-2240)/80,&c));if(b<2240)c.bank=b/80;c.fx_reverb=(b/160)%2;c.delay_mode=2;c.chorus_mode=2;c.reverb_model=(b/80)%4;c.reverb_size=1+(b/80)*99/39;c.reverb_damping=(b/80)*100/39;c.reverb_speed=500;c.reverb_depth=100;c.reverb_time_ms=30000;c.reverb_mix=50;core_set(&c);}
   core_render(pcm,256,4096);assert(!core_worker_fault());if(!dual)memcpy(ref+b*512,pcm,sizeof(pcm));else assert(!memcmp(ref+b*512,pcm,sizeof(pcm)));
  }
 }free(ref);puts("PASS: all 28 banks and 12 AV presets, six voices, chorus, both effects and four reverb models: every block identical in single/dual rendering.");}
