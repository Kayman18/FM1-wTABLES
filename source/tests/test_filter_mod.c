#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
#include "../fm1/app/pw_filter_mod.h"
static int32_t pcm[512];
static void render(unsigned count){while(count--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++){assert(pcm[i]>=-8388608&&pcm[i]<=8388607);assert(pcm[i]==pcm[i^1]);}}}
int main(void){
 pw_filter_mod_init();float previous=0;
 for(unsigned i=0;i<=12700;i++){
  float index=(float)i/100.0f,g=pw_filter_mod_g(index);
  assert(isfinite(g)&&g>=previous&&g<=1.414122f);previous=g;
  if(i%100==0)assert(g==pw_filter_g(i/100));
 }
 assert(pw_filter_mod_index(40,24,.5f)==52);assert(pw_filter_mod_index(120,127,1)==127);assert(pw_filter_mod_index(40,0,1)==40);
 core_settings c={0,48,43,0,75,{90,60,40,80},{PW_FILTER_LP,40,30,100},72};
 core_init();core_set(&c);core_midi(0,0x90,48,127);render(20);
 float early=core_note_cutoff(0,0,48);render(40);float opened=core_note_cutoff(0,0,48);assert(opened>early+10);
 core_midi(0,0x90,55,127);render(10);
 float old=core_note_cutoff(0,0,48),young=core_note_cutoff(0,0,55);
 assert(old>young+15&&young>=40);assert(core_held_count()==2);
 core_midi(0,0x80,55,0);render(10);assert(core_note_held(0,0,48));assert(core_note_cutoff(0,0,48)>young);
 core_midi(0,0xb0,123,0);render(6000);assert(!core_voice_count());for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 /* Dynamic modulation, not just frozen-coefficient impulse stability. */
 uint32_t rng=126106;
 for(unsigned i=0;i<16000;i++){
  rng=rng*1664525u+1013904223u;
  c.env.attack=(uint8_t)(i%2?0:127);c.env.decay=(uint8_t)(rng%128);c.env.sustain=(uint8_t)((rng>>8)%128);c.env.release=(uint8_t)(i%2?0:127);
  c.filter.mode=(uint8_t)(i%4);c.filter.cutoff=(uint8_t)(i%2?0:127);c.filter.resonance=(uint8_t)(i%101);c.filter.mix=(uint8_t)(i%3?100:50);c.filter_env_depth=(uint8_t)(i%2?127:0);c.bank=(uint8_t)(i%8);core_set(&c);
  core_midi(i%2?0:2,(uint8_t)(0x90|(i%16)),(uint8_t)(24+i%60),127);render(1);
  assert(core_voice_count()<=4);
  for(unsigned v=0;v<4;v++){float index=core_voice_cutoff(v);assert(isfinite(index)&&index>=-1&&index<=127);}
 }
 core_panic();render(1);assert(!core_voice_count());for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 printf("PASS: cutoff table endpoints/monotonicity, depth units/clamps, per-note independent sweeps (%.2f vs %.2f), independent release, exact tail silence and 16000 four-voice extreme modulation blocks under sanitizers.\n",old,young);
}
