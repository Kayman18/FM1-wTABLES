#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
/* With all wet samples forced to zero, changing Mix must preserve dry PCM. */
int main(void){int32_t dry[128*300],pcm[128];for(unsigned mix=0;mix<=100;mix+=20){
 core_init();core_settings c;pw_preset_get(0,&c);c.fx_reverb=1;c.reverb_mix=mix;c.level=25;c.reverb_model=3;core_set(&c);
 for(unsigned i=0;i<400;i++)core_render(pcm,64,4096);
 core_midi(0,0x90,60,100);
 for(unsigned b=0;b<300;b++){
  // Clear before each 64-sample block; shortest nonzero Plate taps exceed this.
  for(unsigned i=0;i<reverb.used;i++)delay_memory[0][i]=0;
  for(unsigned i=0;i<6;i++)reverb.low[i]=0;
  core_render(pcm,64,4096);
  if(!mix)for(unsigned i=0;i<128;i++)dry[b*128+i]=pcm[i];
  else for(unsigned i=0;i<128;i++)assert(abs(pcm[i]-dry[b*128+i])<=1);
 }
}
puts("PASS: dry PCM retained within one integer LSB at Mix 0/20/40/60/80/100% with silent wet return.");}
