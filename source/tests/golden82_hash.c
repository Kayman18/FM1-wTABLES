#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
/* Bit-exactness oracle for the wavetable engine: every factory preset at
   several voice limits and unison settings, with note-ons that fill and then
   steal the pool, hashed sample by sample. */
#include <stdio.h>
#include <string.h>
static uint64_t h=1469598103934665603ull;
static void feed(const int32_t *pcm,unsigned n){for(unsigned i=0;i<n*2;i++){uint32_t x=(uint32_t)pcm[i];h^=x;h*=1099511628211ull;}}
int main(void){int32_t pcm[512];core_settings s;
 static const unsigned limits[]={4,6,8};
 for(unsigned pi=0;pi<PW_PRESET_COUNT;pi++)
 for(unsigned li=0;li<3;li++)
 for(unsigned un=0;un<2;un++){
  core_init();core_parallel_set(0);pw_preset_default(&s);pw_preset_get(pi,&s);
  s.playing=0;s.bloom_mode=0;s.voice_limit=limits[li];s.unison_count=un?2:1;
  core_set(&s);
  for(unsigned k=0;k<12;k++){
   if(k==1){core_midi(0,0x90,48,100);core_midi(0,0x90,52,100);core_midi(0,0x90,55,100);core_midi(0,0x90,59,100);}
   if(k==3){core_midi(0,0x90,62,110);core_midi(0,0x90,65,110);core_midi(0,0x90,69,110);core_midi(0,0x90,72,110);}
   if(k==5){core_midi(0,0x90,74,120);core_midi(0,0x90,77,120);}   /* forces stealing */
   if(k==8){core_midi(0,0x80,48,0);core_midi(0,0x80,52,0);}
   if(k==9)core_midi(0,0xB0,74,40);
   if(k==10)core_midi(0,0xE0,0,96);
   core_render(pcm,256,4096);feed(pcm,256);
  }}
 printf("WTGOLDEN %016llx\n",(unsigned long long)h);return 0;}
