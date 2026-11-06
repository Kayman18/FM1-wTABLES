#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/pw_filter.h"
static double tone(unsigned mode,double hz){
 pw_filter s={0};double energy=0;float g=pw_filter_g(72),d=pw_filter_damping(g,pw_filter_q(0));
 for(unsigned i=0;i<88200;i++){float in=(float)sin(6.283185307179586*hz*i/44100.0);pw_filter_tick(&s,in,g,d);float out=pw_filter_output(&s,mode,in);if(i>=44100)energy+=(double)out*out;}
 return sqrt(energy/44100.0);
}
int main(void){
 float worst=0;
 for(unsigned f=0;f<128;f++)for(unsigned q=0;q<=100;q++){
  pw_filter s={0};float g=pw_filter_g(f),d=pw_filter_damping(g,pw_filter_q(q));
  assert(g>0&&g<=1.414122f&&d<0);
  for(unsigned i=0;i<10000;i++){
   pw_filter_tick(&s,i==0?1.0f:0.0f,g,d);
   assert(isfinite(s.low)&&isfinite(s.band)&&isfinite(s.high));
   float peak=fmaxf(fabsf(s.low),fmaxf(fabsf(s.band),fabsf(s.high)));if(peak>worst)worst=peak;assert(peak<10.0f);
  }
 }
 double lp=tone(PW_FILTER_LP,100)/tone(PW_FILTER_LP,5000);
 double hp=tone(PW_FILTER_HP,5000)/tone(PW_FILTER_HP,100);
 double bp=tone(PW_FILTER_BP,523.251)/tone(PW_FILTER_BP,50);
 assert(lp>50&&hp>15&&bp>3);
 pw_filter s={1,2,3};assert(pw_filter_output(&s,PW_FILTER_LP,4)==1);assert(pw_filter_output(&s,PW_FILTER_BP,4)==2);assert(pw_filter_output(&s,PW_FILTER_HP,4)==3);assert(pw_filter_output(&s,PW_FILTER_OFF,4)==4);
 pw_filter_reset(&s);assert(!pw_filter_live(&s));
 printf("PASS: impulse stability for all 128 cutoff / 101 resonance combinations; max impulse state %.5f. Tone ratios LP %.1f, HP %.1f, BP %.1f; all output modes and reset verified.\n",worst,lp,hp,bp);
}
