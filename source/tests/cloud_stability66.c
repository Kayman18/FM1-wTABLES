#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "pw_reverb.h"
#define MEM 35328
static float mem[MEM];
static float frand01(unsigned *seed){*seed=*seed*1664525u+1013904223u;return (float)(*seed>>8)*(1.f/16777216.f);}
int main(void){
 pw_reverb s={0};
 assert(PW_CLOUD_FLOATS==16744);
 assert(pw_reverb_configure(&s,mem,PW_CLOUD_FLOATS-1,3,70,44100)==PW_CLOUD_FLOATS);
 assert(s.used==0u);
 unsigned used=pw_reverb_configure(&s,mem,MEM,3,70,44100);
 assert(used==PW_CLOUD_FLOATS && used < MEM);
 assert(!pw_reverb_clear(&s,mem,2048));
 for(unsigned z=0;z<8;z++)pw_reverb_clear(&s,mem,2048);
 assert(pw_reverb_clear(&s,mem,2048));
 float maximum=0,peakL=0,peakR=0,peakTail=0;unsigned seed=1;
 for(unsigned j=0;j<44100*60;j++){
  if(j%44100==0){
   unsigned turn=j/44100;
   pw_reverb_controls(&s,turn%3==0?400u:turn%3==1?2000u:30000u,turn%2?0u:100u,turn%2?500u:1u,turn%3?100u:0u);
   pw_reverb_space(&s,turn%2?1:100);
  }
  float x= j<44100*35 ? ((frand01(&seed)-.5f)*.25f) : 0.0f;
  float v[2];pw_reverb_tick(&s,x,v);
  assert(isfinite(v[0])&&isfinite(v[1]));
  float m=fmaxf(fabsf(v[0]),fabsf(v[1]));
  if(m>maximum)maximum=m;
  if(j>44100*35 && m>peakTail)peakTail=m;
  if(j>44100*20 && j<44100*35){peakL=fmaxf(peakL,fabsf(v[0]));peakR=fmaxf(peakR,fabsf(v[1]));}
  if(m>200.0f){fprintf(stderr,"runaway at %u sample output %g\n",j,(double)m);return 5;}
 }
 assert(peakL>0.00001f && peakR>0.00001f);
 printf("PASS Clouds 60 sec / 44.1kHz, 32-bit delay, no nonfinite/output runaway; peak %.5g stereo %.5g/%.5g tail %.5g; shared buffer remaining %u floats\n",maximum,peakL,peakR,peakTail,MEM-used);
}
