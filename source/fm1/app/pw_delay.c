/* TEST60 Rikoshet Lite implementation, adapted from SHLabs Rikoshet PingPong.
   Rikoshet is GPL-3.0, source: https://github.com/shlabs-audio/rikoshet
   Full-rate input -> 7-tap binomial 2:1 decimator; 32-bit floating point
   two-line feedback network; 4-point interpolation; low/high cut; stereo
   tap spread and cross-feedback; linear wet interpolation back to 44.1 kHz.
   Shared buffer: no heap or per-sample transcendental functions. */
#include "pw_delay.h"
void pw_delay_route(const float history[4],float wet[4],unsigned mode){
 unsigned swap=mode==2?1u:0u;for(unsigned k=0;k<4;k++)wet[k]=history[k^swap];
}
void pw_delay_frame(const float input[4],const float history[4],float written[4],float output[4],unsigned mode,float feedback,float level){
 float wet[4];pw_delay_route(history,wet,mode);
 for(unsigned k=0;k<4;k++){written[k]=input[k]+feedback*wet[k];output[k]=input[k]+level*wet[k];}
}
static float bound(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
void pw_delay_init(pw_delay *s,float (*memory)[2],unsigned frames){
 *s=(pw_delay){0};s->ring=memory;s->length=frames;
 pw_delay_controls(s,0,100,80,8000);
}
void pw_delay_reset(pw_delay *s,float samples){
 s->write=s->valid=s->fade=s->phase=0;
 s->current=s->next=samples;
 for(unsigned k=0;k<2;k++){
  s->wet[k]=s->previous_wet[k]=s->lp[k]=s->hp[k]=0.0f;
  for(unsigned j=0;j<7;j++)s->in_hist[k][j]=0.0f;
 }
}
void pw_delay_controls(pw_delay *s,unsigned spread,unsigned cross,unsigned lowcut,unsigned highcut){
 if(spread>100)spread=100;if(cross>100)cross=100;
 if(lowcut<20)lowcut=20;if(lowcut>1000)lowcut=1000;
 if(highcut<500)highcut=500;if(highcut>9500)highcut=9500;
 if(s->spread_pct==spread&&s->cross_pct==cross&&s->lohz==lowcut&&s->hihz==highcut)return;
 s->spread_pct=(uint8_t)spread;s->cross_pct=(uint8_t)cross;
 s->lohz=(uint16_t)lowcut;s->hihz=(uint16_t)highcut;
 s->spread=(float)spread*.0025f; /* 0..25% extra right-channel delay */
 s->cross=(float)cross*.01f;
 /* Prewarped one-pole approximations. These values only change at control rate. */
 float omega=(6.2831853f*(float)highcut)/PW_DELAY_DSP_RATE;
 s->lowcoef=bound(omega/(1.0f+omega),0.0f,0.98f);
 omega=(6.2831853f*(float)lowcut)/PW_DELAY_DSP_RATE;
 s->highcoef=bound(omega/(1.0f+omega),0.0f,0.9f);
}
static unsigned sample_index(const pw_delay *s,unsigned age){
 unsigned pos=s->write;return pos>=age?pos-age:pos+s->length-age;
}
/* Four-point Lagrange interpolation for a requested age in ring samples. */
static float tap(const pw_delay *s,float age,unsigned channel){
 if(age<3.0f)age=3.0f;
 if(age>(float)(s->length-4))age=(float)(s->length-4);
 unsigned whole=(unsigned)age;float f=age-(float)whole;
 float y[4];
 for(unsigned i=0;i<4;i++){
  unsigned a=whole+i-1;
  y[i]=a<=s->valid?s->ring[sample_index(s,a)][channel]:0.0f;
 }
 float c0=-f*(f-1.0f)*(f-2.0f)*(1.0f/6.0f);
 float c1=(f+1.0f)*(f-1.0f)*(f-2.0f)*0.5f;
 float c2=-(f+1.0f)*f*(f-2.0f)*0.5f;
 float c3=(f+1.0f)*f*(f-1.0f)*(1.0f/6.0f);
 return c0*y[0]+c1*y[1]+c2*y[2]+c3*y[3];
}
static void half_tick(pw_delay *s,const float input[2],unsigned mode,float samples,float feedback){
 if(s->current<3.0f)s->current=s->next=samples;
 if(!s->fade&&samples!=s->current){s->next=samples;s->fade=1;}
 float delay[2]={s->current,bound(s->current*(1.0f+s->spread),3.0f,(float)(s->length-4))};
 float read[2]={tap(s,delay[0],0),tap(s,delay[1],1)};
 if(s->fade){
  float next_delay[2]={s->next,bound(s->next*(1.0f+s->spread),3.0f,(float)(s->length-4))};
  float x=(float)s->fade*(1.0f/128.0f);
  for(unsigned k=0;k<2;k++)read[k]+=(tap(s,next_delay[k],k)-read[k])*x;
  if(++s->fade>128){s->current=s->next;s->fade=0;}
 }
 for(unsigned k=0;k<2;k++){
  s->lp[k]+=s->lowcoef*(read[k]-s->lp[k]);
  s->hp[k]+=s->highcoef*(s->lp[k]-s->hp[k]);
  if(s->lp[k]<1e-17f&&s->lp[k]>-1e-17f)s->lp[k]=0.0f;
  if(s->hp[k]<1e-17f&&s->hp[k]>-1e-17f)s->hp[k]=0.0f;
 }
 float filtered[2]={s->lp[0]-s->hp[0],s->lp[1]-s->hp[1]};
 float cross=mode==2?s->cross:0.0f;
 s->previous_wet[0]=s->wet[0];s->previous_wet[1]=s->wet[1];
 for(unsigned k=0;k<2;k++){
  float cross_wet=filtered[k]+cross*(filtered[k^1]-filtered[k]);
  /* Safe feedback: the wet output is cross-routed in ping-pong mode. */
  float write=input[k]+cross_wet*feedback;
  if(write<1e-17f&&write>-1e-17f)write=0.0f;
  s->ring[s->write][k]=write;
  s->wet[k]=read[k]+cross*(read[k^1]-read[k]);
 }
 if(++s->write>=s->length)s->write=0;
 if(s->valid<s->length)s->valid++;
}
static void tick(pw_delay *s,const float input[2],float output[2],unsigned mode,float samples,float feedback,float mix,unsigned source){
 if(!mode||mode>2||s->length<8){output[0]=input[0];output[1]=input[1];return;}
 samples=bound(samples,3.0f,(float)(s->length-4));
 feedback=bound(feedback,0.0f,.9f);mix=bound(mix,0.0f,1.0f);
 for(unsigned k=0;k<2;k++){
  for(unsigned j=6;j;j--)s->in_hist[k][j]=s->in_hist[k][j-1];
  s->in_hist[k][0]=input[k];
 }
 if(s->phase){
  float down[2];
  for(unsigned k=0;k<2;k++){
   const float *h=s->in_hist[k];
   down[k]=(h[0]+6.0f*h[1]+15.0f*h[2]+20.0f*h[3]+15.0f*h[4]+6.0f*h[5]+h[6])*(1.0f/64.0f);
  }
  half_tick(s,down,mode,samples,feedback);
 }
 /* Half-rate wet interpolation smooths images and adds no lookahead. */
 for(unsigned k=0;k<2;k++){
  float wet=s->phase?s->wet[k]:0.5f*(s->previous_wet[k]+s->wet[k]);
  output[k]=source?input[k]+wet*mix:input[k]+(wet-input[k])*mix;
 }
 s->phase^=1u;
}
void pw_delay_tick(pw_delay *s,const float input[2],float output[2],unsigned mode,float samples,float feedback,float mix){tick(s,input,output,mode,samples,feedback,mix,0);}
void pw_delay_tick_source(pw_delay *s,const float input[2],float output[2],unsigned mode,float samples,float feedback,float level){tick(s,input,output,mode,samples,feedback,level,1);}
const char *pw_delay_division_name(unsigned d){static const char *n[]={"1/32","1/16","1/8","1/4"};return n[d>3?3:d];}
float pw_delay_sync_ms(unsigned bpm,unsigned d,unsigned *effective){
 if(bpm<40)bpm=40;if(bpm>240)bpm=240;if(d>3)d=3;
 float ms=7500.0f*(float)(1u<<d)/(float)bpm;
 while(ms>(float)PW_DELAY_MAX_MS&&d){ms*=0.5f;--d;}
 if(effective)*effective=d;return ms;
}
