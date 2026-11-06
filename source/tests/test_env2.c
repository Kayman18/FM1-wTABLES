#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
static int32_t pcm[512];
static void render(unsigned count){while(count--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++){assert(pcm[i]>=-8388608&&pcm[i]<=8388607);assert(pcm[i]==pcm[i^1]);}}}
int main(void){
 core_settings c={0,48,43,0,75,{0,0,127,90},{PW_FILTER_LP,40,0,100},0,{90,60,20,80},72};
 core_init();core_set(&c);core_midi(0,0x90,48,127);render(20);
 assert(core_env_level()==1.0f);float early=core_note_env2(0,0,48);render(40);float opened=core_note_env2(0,0,48);assert(opened>early+.1f&&core_env_level()==1.0f);
 core_midi(2,0x91,55,127);render(10);
 float older=core_note_env2(0,0,48),younger=core_note_env2(2,1,55);assert(older>younger+.2f);
 float cutold=core_note_cutoff(0,0,48),cutyoung=core_note_cutoff(2,1,55);assert(cutold>cutyoung+15);
 core_midi(2,0x81,55,0);render(10);assert(core_note_env2(2,1,55)<younger);assert(core_note_held(0,0,48));
 core_midi(0,0xb0,123,0);render(6000);assert(!core_voice_count());for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 /* Sum both routes in semitone space, independent sustain shapes. */
 c.env=(pw_env_params){0,0,127,0};c.env2=(pw_env_params){0,0,64,0};c.filter_env_depth=12;c.filter_env2_depth=24;core_init();core_set(&c);core_midi(0,0x90,60,127);render(50);
 assert(fabsf(core_note_cutoff(0,0,60)-(40+12+24*(64.0f/127.0f)))<.001f);
 uint32_t rng=126107;
 for(unsigned i=0;i<16000;i++){
  rng=rng*1664525u+1013904223u;c.env2=(pw_env_params){(uint8_t)(rng%128),(uint8_t)((rng>>7)%128),(uint8_t)((rng>>14)%128),(uint8_t)((rng>>21)%128)};
  c.filter_env_depth=(uint8_t)(i%2?127:0);c.filter_env2_depth=(uint8_t)(i%3?127:0);c.filter.cutoff=(uint8_t)(i%2?0:127);c.filter.resonance=(uint8_t)(i%101);c.filter.mode=(uint8_t)(i%4);c.bank=(uint8_t)(i%8);core_set(&c);
  core_midi(i%2?0:2,(uint8_t)(0x90|(i%16)),(uint8_t)(24+i%60),127);render(1);
  assert(core_voice_count()<=4);float e=core_env2_level();assert(isfinite(e)&&e>=0&&e<=1);
 }
 core_panic();render(1);assert(core_env2_level()==0&&!core_voice_count());for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 printf("PASS: Env2 moves cutoff independently of sustained amp; staggered voice levels %.3f/%.3f give cutoffs %.2f/%.2f; independent gate/release, additive depth mapping, tail/panic silence and 16000 dual-envelope stress blocks.\n",older,younger,cutold,cutyoung);
}
