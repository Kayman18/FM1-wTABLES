#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/pw_delay.h"
#include "../fm1/app/test_engine.h"
static float memory[PW_DELAY_FRAMES][2];static int32_t pcm[512];
int main(void){
 for(unsigned mode=1;mode<=2;mode++){
  pw_delay d;pw_delay_init(&d,memory,512);pw_delay_reset(&d,10);float in[2],out[2];
  for(unsigned n=0;n<500;n++){
   in[0]=n==0?1:0;in[1]=0;pw_delay_tick(&d,in,out,mode,10,.5f,1);
   if(n==10){assert(out[mode==2?1:0]==.5f);assert(out[mode==2?0:1]==0);}
   if(n==20){assert(out[0]==.25f&&out[1]==0);}
   if(n>450)assert(out[0]==0&&out[1]==0);
  }
 }
 pw_delay d;pw_delay_init(&d,memory,PW_DELAY_FRAMES);pw_delay_reset(&d,24256);float in[2],out[2];
 for(unsigned n=0;n<50000;n++){in[0]=n==0?1:0;in[1]=0;pw_delay_tick(&d,in,out,1,24256,0,1);assert(out[0]==(n==24256?1:0)&&out[1]==0);}
 pw_delay_reset(&d,10.5f);
 for(unsigned n=0;n<50;n++){in[0]=n==0?1:0;in[1]=0;pw_delay_tick(&d,in,out,1,10.5f,0,1);assert(out[0]==(n==10||n==11?.5f:0));}
 for(unsigned n=0;n<100000;n++){
  in[0]=sinf((float)n*.03f);in[1]=cosf((float)n*.05f);float time=50+(float)((n/270)%24000);float fb=(float)(n%91)*.01f,mix=(float)(n%101)*.01f;
  pw_delay_tick(&d,in,out,n%2+1,time,fb,mix);assert(isfinite(out[0])&&isfinite(out[1])&&fabsf(out[0])<=1.000001f&&fabsf(out[1])<=1.000001f);assert(d.write<PW_DELAY_FRAMES);
 }
 pw_delay_reset(&d,1);in[0]=in[1]=0;for(unsigned n=0;n<30000;n++){pw_delay_tick(&d,in,out,2,1,.9f,1);assert(out[0]==0&&out[1]==0);}
 core_settings c={0,48,0,0,100,{0,0,127,0},{0,96,0,100},0,{0,0,127,0}};c.wave_quality=1;c.delay_mode=1;c.delay_time_ms=100;c.delay_feedback=90;c.delay_mix=100;
 core_init();core_set(&c);for(unsigned ch=0;ch<4;ch++)core_midi(0,(uint8_t)(0x90|ch),48,127);
 for(unsigned n=0;n<1200;n++){core_render(pcm,256,4096);for(unsigned j=0;j<512;j++)assert(pcm[j]!=8388607&&pcm[j]!=-8388608);}
 for(unsigned n=0;n<12000;n++){
  c.delay_mode=(uint8_t)(n%3);c.delay_time_ms=(uint16_t)(1+n%550);c.delay_feedback=(uint8_t)(n%91);c.delay_mix=(uint8_t)(n%101);c.unison_mode=(uint8_t)(n%3);c.unison_detune=(uint8_t)(n%101);c.chorus_mode=(uint8_t)(n%3);c.chorus_mix=(uint8_t)(n%101);c.stereo_mode=(uint8_t)(n%4);c.filter.mode=(uint8_t)(n%4);c.bank=(uint8_t)(n%8);core_set(&c);core_midi(n%3,(uint8_t)(0x90|n%16),(uint8_t)(24+n%60),127);core_render(pcm,256,4096);for(unsigned j=0;j<512;j++)assert(pcm[j]>=-8388608&&pcm[j]<=8388607);
 }
 core_panic();core_render(pcm,256,4096);for(unsigned j=0;j<512;j++)assert(!pcm[j]);
 puts("PASS: exact Stereo/Ping-pong impulse routing/feedback, maximum time and ring wrap, fractional tap, bounded variable-time/feedback/mix, reset without stale replay, full-level four-voice headroom, 12000 sanitized delay/unison/chorus/filter/bank blocks and immediate panic silence.");
}
