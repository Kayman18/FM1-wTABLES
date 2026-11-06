#include "pw_delay.h"
void pw_delay_route(const float history[4],float wet[4],unsigned mode){
 unsigned swap=mode==2?1u:0u;for(unsigned k=0;k<4;k++)wet[k]=history[k^swap];
}
void pw_delay_frame(const float input[4],const float history[4],float written[4],float output[4],unsigned mode,float feedback,float level){
 float wet[4];pw_delay_route(history,wet,mode);
 for(unsigned k=0;k<4;k++){written[k]=input[k]+feedback*wet[k];output[k]=input[k]+level*wet[k];}
}
void pw_delay_init(pw_delay *s,float (*memory)[2],unsigned frames){*s=(pw_delay){0};s->ring=memory;s->length=frames;}
void pw_delay_reset(pw_delay *s,float samples){s->write=s->valid=s->fade=0;s->current=s->next=samples;}
static void tap(const pw_delay *s,float delay,float out[2]){
 unsigned whole=(unsigned)delay;float f=delay-(float)whole;
 unsigned a=s->write>=whole?s->write-whole:s->write+s->length-whole;
 unsigned b=a?a-1:s->length-1;
 for(unsigned k=0;k<2;k++){float x=whole<=s->valid?s->ring[a][k]:0.0f,y=whole+1u<=s->valid?s->ring[b][k]:0.0f;out[k]=x+(y-x)*f;}
}
void pw_delay_tick(pw_delay *s,const float input[2],float output[2],unsigned mode,float samples,float feedback,float mix){
 if(!mode||mode>2||s->length<4){output[0]=input[0];output[1]=input[1];return;}
 if(samples<1.0f)samples=1.0f;else if(samples>(float)(s->length-2))samples=(float)(s->length-2);
 if(feedback<0.0f)feedback=0.0f;else if(feedback>.9f)feedback=.9f;if(mix<0.0f)mix=0.0f;else if(mix>1.0f)mix=1.0f;
 if(s->current<1.0f)s->current=s->next=samples;
 if(!s->fade&&samples!=s->current){s->next=samples;s->fade=1;}
 float read[2];tap(s,s->current,read);
 if(s->fade){float next[2];tap(s,s->next,next);float f=(float)s->fade*(1.0f/256.0f);for(unsigned k=0;k<2;k++)read[k]+=(next[k]-read[k])*f;if(++s->fade>256){s->current=s->next;s->fade=0;}}
 float history[4]={read[0],read[1],0.0f,0.0f},routed[4];pw_delay_route(history,routed,mode);
 for(unsigned k=0;k<2;k++){
  float write=input[k]*(1.0f-feedback)+routed[k]*feedback;
  if(write<.000001f&&write> -.000001f)write=0.0f;
  s->ring[s->write][k]=write;output[k]=input[k]+(routed[k]-input[k])*mix;
 }
 if(++s->write==s->length)s->write=0;if(s->valid<s->length)s->valid++;
}
