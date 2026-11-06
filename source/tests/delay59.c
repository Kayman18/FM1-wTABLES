#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
int main(void){
 for(unsigned bpm=40;bpm<=240;bpm++)for(unsigned d=0;d<4;d++){
  unsigned actual=99;float ms=pw_delay_sync_ms(bpm,d,&actual);
  assert(ms>0&&ms<=550&&actual<=d);
  assert(fabsf(ms-7500.0f*(1u<<actual)/bpm)<.001f);
 }
 unsigned actual;assert(pw_delay_sync_ms(120,2,&actual)==250&&actual==2);
 assert(pw_delay_sync_ms(40,3,&actual)==375&&actual==1);
 float (*mem)[2]=calloc(PW_DELAY_FRAMES,sizeof(*mem));assert(mem);
 for(unsigned mode=1;mode<=2;mode++){
  pw_delay delay;pw_delay_init(&delay,mem,PW_DELAY_FRAMES);pw_delay_reset(&delay,11025);
  for(unsigned i=0;i<=11025;i++){float in[2]={i==0?1:0,0},out[2];pw_delay_tick_source(&delay,in,out,mode,11025,0,1);
   if(i>0&&i<11025)assert(out[0]==0&&out[1]==0);
   if(i==11025)assert(out[mode==1?0:1]==1);
  }
 }free(mem);
 core_init();core_settings c;pw_preset_get(0,&c);c.delay_mode=1;c.delay_sync=1;c.delay_division=2;c.arp_bpm=120;core_set(&c);
 int32_t pcm[512];for(unsigned i=0;i<300;i++)core_render(pcm,256,4096);assert(delay.current==11025);
 c.arp_bpm=40;c.delay_division=3;core_set(&c);for(unsigned i=0;i<20;i++)core_render(pcm,256,4096);assert(delay.current==16537.5f);
 assert(pw_preset_apply(1,&c));assert(c.delay_sync==1&&c.delay_division==3&&c.arp_bpm==40);
 c.delay_mode=1;c.delay_sync=0;c.delay_time_ms=123;core_set(&c);for(unsigned i=0;i<300;i++)core_render(pcm,256,4096);assert(fabsf(delay.current-(123*44.1f+1))<.01f);
 puts("PASS: sync divisions at every BPM 40-240, musical capacity fallback, exact stereo/ping-pong impulse timing, live engine tempo changes, free timing and preset persistence.");
}
