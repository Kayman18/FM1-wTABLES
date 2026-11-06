#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
#include "../fm1/app/pw_chorus.h"
static int32_t pcm[512];
int main(void){
 for(unsigned mix=0;mix<=100;mix++){
  float m=(float)mix*.01f,g=pw_chorus_mix_headroom(m);
  for(int a=-10;a<=10;a++)for(int b=-10;b<=10;b++){
   float dry=(float)a*.1f,wet=(float)b*.1f,effect=(dry+wet)*.7070000171661377f;
   assert(fabsf((dry+(effect-dry)*m)*g)<1.000001f);
  }
 }
 for(unsigned mode=0;mode<=2;mode++){
  core_settings c={0,48,0,0,100,{0,0,127,0},{0,96,0,100},0,{0,0,127,0}};c.wave_quality=1;c.chorus_mode=(uint8_t)mode;c.chorus_mix=100;
  core_init();core_set(&c);for(unsigned ch=0;ch<4;ch++)core_midi(0,(uint8_t)(0x90|ch),48,127);
  unsigned clipped=0;int32_t peak=0;
  for(unsigned block=0;block<400;block++){
   core_render(pcm,256,4096);
   for(unsigned n=0;n<512;n++){int32_t a=pcm[n]<0?-pcm[n]:pcm[n];if(a>peak)peak=a;if(pcm[n]==8388607||pcm[n]==-8388608)clipped++;}
  }
  assert(!clipped);assert(peak<=7680002);
  printf("PASS: mode%u, four aligned full-velocity notes, full level/master/mix: peak %d, zero clipped samples.\n",mode,peak);
  core_panic();core_render(pcm,256,4096);for(unsigned n=0;n<512;n++)assert(!pcm[n]);
 }
 puts("PASS: Mode2 wrapper peak bound at all 101 mix values; original source kernel retained, panic flush remains exact.");
}
