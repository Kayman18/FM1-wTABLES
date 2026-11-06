#include "pw_chorus.h"
#include "pw_chorus_tables.h"
void pw_chorus_reset(pw_chorus *s){*s=(pw_chorus){0};}
void pw_chorus_tick(pw_chorus *s,const float input[4],float output[4],unsigned mode,unsigned rate){
 if(!mode||mode>2){for(unsigned k=0;k<4;k++)output[k]=input[k];return;}
 unsigned row=mode-1;s->write=(s->write+1u)&255u;
 for(unsigned k=0;k<4;k++)s->ring[s->write][k]=input[k];
 s->phase+=(rate==48000?pw_chorus_increment48:pw_chorus_increment44)[row];
 static const uint32_t offsets[4]={0,0x80000000u,0x40000000u,0xc0000000u};
 for(unsigned k=0;k<4;k++){
  unsigned phase=(s->phase+offsets[k])>>22;
  unsigned idx=(s->write+(rate==48000?pw_chorus_index48:pw_chorus_index44)[row][phase])&255u;
  float fraction=(rate==48000?pw_chorus_fraction48:pw_chorus_fraction44)[row][phase];
  float a=s->ring[idx][k],b=s->ring[(idx+1u)&255u][k];
  float wet=a+(b-a)*fraction;
  output[k]=mode==1?wet:(input[k]+wet)*0.7070000171661377f;
 }
}
float pw_chorus_offset_smooth(unsigned mode,uint32_t phase){
 unsigned row=mode==2?1:0,i=phase>>22,j=(i+1u)&1023u;
 float fraction=(float)(phase&0x3fffffu)*(1.0f/4194304.0f);
 float a=(float)pw_chorus_index44[row][i]+pw_chorus_fraction44[row][i];
 float b=(float)pw_chorus_index44[row][j]+pw_chorus_fraction44[row][j];
 return a+(b-a)*fraction;
}
float pw_chorus_mix_headroom(float mix){
 if(mix<0.0f)mix=0.0f;else if(mix>1.0f)mix=1.0f;
 /* Bound dry+(0.707*(dry+wet)-dry)*mix to its input peak. */
 return 1.0f/(1.0f+0.4140000343322754f*mix);
}
void pw_chorus_tick_smooth(pw_chorus *s,const float input[4],float output[4],unsigned mode){
 if(!mode||mode>2){for(unsigned k=0;k<4;k++)output[k]=input[k];return;}
 s->write=(s->write+1u)&255u;
 for(unsigned k=0;k<4;k++)s->ring[s->write][k]=input[k];
 s->phase+=pw_chorus_increment44[mode-1];
 static const uint32_t offsets[4]={0,0x80000000u,0x40000000u,0xc0000000u};
 for(unsigned k=0;k<4;k++){
  float position=pw_chorus_offset_smooth(mode,s->phase+offsets[k]);
  unsigned whole=(unsigned)position,idx=(s->write+whole)&255u;
  float fraction=position-(float)whole;
  float a=s->ring[idx][k],b=s->ring[(idx+1u)&255u][k];
  float wet=a+(b-a)*fraction;
  output[k]=mode==1?wet:(input[k]+wet)*0.7070000171661377f;
 }
}

/* The FM1 consumes two lanes; unused lanes remain zero after reset. */
void pw_chorus_tick_stereo(pw_chorus *s,const float input[4],float output[4],unsigned mode){
 if(!mode||mode>2){for(unsigned k=0;k<2;k++)output[k]=input[k];return;}
 s->write=(s->write+1u)&255u;
 for(unsigned k=0;k<2;k++)s->ring[s->write][k]=input[k];
 s->phase+=pw_chorus_increment44[mode-1];
 static const uint32_t offsets[4]={0,0x80000000u,0x40000000u,0xc0000000u};
 for(unsigned k=0;k<2;k++){
  float position=pw_chorus_offset_smooth(mode,s->phase+offsets[k]);
  unsigned whole=(unsigned)position,idx=(s->write+whole)&255u;
  float fraction=position-(float)whole;
  float a=s->ring[idx][k],b=s->ring[(idx+1u)&255u][k];
  float wet=a+(b-a)*fraction;
  output[k]=mode==1?wet:(input[k]+wet)*0.7070000171661377f;
 }
}
