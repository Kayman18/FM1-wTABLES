/* TEST60 Rikoshet Lite delay: source-adapted stereo and 750 ms capacity. */
#include "../fm1/app/pw_delay.h"
#include "../fm1/app/pw_reverb.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static float buffer[PW_DELAY_FRAMES][2];
static pw_delay d;
static void reset(unsigned spread,unsigned cross,unsigned lo,unsigned hi,float ms){
 memset(buffer,0,sizeof buffer);pw_delay_init(&d,buffer,PW_DELAY_FRAMES);
 pw_delay_controls(&d,spread,cross,lo,hi);
 pw_delay_reset(&d,ms*22.05f);
}
static void impulse(unsigned mode,float delay_ms,float feedback,unsigned cross,unsigned spread,unsigned *first_channel,unsigned *peak_sample,float *peak){
 reset(spread,cross,20,9500,delay_ms);
 float channel_peaks[2]={0,0};unsigned channel_samples[2]={0,0};
 unsigned end=(unsigned)(44.1f*delay_ms*1.17f)+320;
 for(unsigned i=0;i<end;i++){
  float input[2]={i==0?1.0f:0.0f,0.0f},output[2];
  pw_delay_tick_source(&d,input,output,mode,22.05f*delay_ms,feedback,1.0f);
  for(unsigned k=0;k<2;k++){
   float v=fabsf(output[k]-(i==0?input[k]:0.0f));
   if(v>channel_peaks[k]){channel_peaks[k]=v;channel_samples[k]=i;}
   assert(isfinite(output[k]));
  }
 }
 *first_channel=channel_peaks[0]>=channel_peaks[1]?0:1;
 *peak_sample=channel_samples[*first_channel];*peak=channel_peaks[*first_channel];
 printf("impulse mode=%u ms=%.1f: L %.5f@%u R %.5f@%u\n",mode,delay_ms,channel_peaks[0],channel_samples[0],channel_peaks[1],channel_samples[1]);
 if(mode==1&&spread==0)assert(channel_peaks[1]<1e-8f);
 if(mode==2&&cross==100&&feedback==0)assert(channel_peaks[0]<1e-8f);
}
int main(void){
 assert(sizeof buffer==141312); assert(PW_DELAY_FRAMES*8==141312);
 for(unsigned bpm=40;bpm<=240;bpm++)for(unsigned div=0;div<4;div++){
  unsigned actual=99;float ms=pw_delay_sync_ms(bpm,div,&actual);
  assert(ms>0.0f&&ms<=750.0f&&actual<=div);
  assert(fabsf(ms-7500.0f*(float)(1u<<actual)/(float)bpm)<.001f);
 }
 unsigned actual=0;assert(pw_delay_sync_ms(120,2,&actual)==250.0f&&actual==2);
 assert(pw_delay_sync_ms(40,3,&actual)==750.0f&&actual==2);
 for(unsigned mode=1;mode<=2;mode++){
  unsigned first,peak_at;float peak;
  impulse(mode,250.0f,0.0f,100,0,&first,&peak_at,&peak);
  assert(first==(mode==1?0u:1u));
  assert(peak>.04f);assert(peak_at>10990&&peak_at<11060);
 }
 {unsigned first,peak_at;float peak;impulse(2,750.0f,0.0f,100,0,&first,&peak_at,&peak);
  assert(first==1&&peak>.04f&&peak_at>33040&&peak_at<33120);}
 /* Confirm cross-feedback creates a second bounce on alternate channels. */
 reset(0,100,20,9500,130.0f);
 float pkL=0,pkR=0;
 for(unsigned i=0;i<14000;i++){
  float input[2]={i==0?1.0f:0.0f,0},out[2];
  pw_delay_tick_source(&d,input,out,2,130.0f*22.05f,0.8f,1.0f);
  assert(isfinite(out[0])&&isfinite(out[1]));
  if(i>10400&&i<11800&&fabsf(out[0])>pkL)pkL=fabsf(out[0]);
  if(i>5400&&i<6600&&fabsf(out[1])>pkR)pkR=fabsf(out[1]);
 }
 assert(pkR>.03f&&pkL>.015f);printf("ping-pong first=%.5f second=%.5f\n",pkR,pkL);
 /* Sweep controls while feeding alternating input, check finiteness and limiting. */
 reset(100,100,80,8000,750.0f);
 for(unsigned i=0;i<100000;i++){
  if(i%401==0)pw_delay_controls(&d,(i/401)%101,(i/201)%101,20+(i%49)*20,500+(i%37)*250);
  float input[2]={i%20<10?0.11f:-0.11f,i%13<7?0.1f:-0.1f},out[2];
  float delaySamples=3.0f+(float)((i*757u)%16500u);
  pw_delay_tick_source(&d,input,out,1+i%2,delaySamples,0.90f,0.6f);
  assert(isfinite(out[0])&&isfinite(out[1]));
  assert(fabsf(out[0])<6&&fabsf(out[1])<6);
 }
 /* Reverb remains within the same, now smaller, shared effects buffer. */
 pw_reverb rv;unsigned largest=0;unsigned model=0,size=0;
 for(unsigned m=0;m<4;m++)for(unsigned sz=1;sz<=100;sz++){
  unsigned required=pw_reverb_configure(&rv,&buffer[0][0],PW_DELAY_FRAMES*2,m,sz,44100);
  if(required>largest){largest=required;model=m;size=sz;}
  assert(required<=PW_DELAY_FRAMES*2);assert(rv.used==required);
 }
 printf("max reverb samples %u (%u bytes), model %u, size %u; within shared %u bytes\n",largest,largest*4u,model,size,(unsigned)sizeof buffer);
 puts("PASS TEST60: 750ms clock, stereo and pingpong impulses, alternating feedback, numeric stability, control sweeps and all reverb memory models.");
}
