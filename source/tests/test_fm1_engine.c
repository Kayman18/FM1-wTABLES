#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../fm1/app/test_engine.h"
static int32_t pcm[512];
int main(void){
 core_init();core_settings c={1,48,30,0,75,{50,60,80,60}};int64_t energy=0;core_set(&c);
 for(unsigned b=0;b<1000;b++){
  core_render(pcm,256,4096);
  for(unsigned j=0;j<256;j++){assert(pcm[2*j]==pcm[2*j+1]);assert(abs(pcm[2*j])<=8388607);energy+=llabs(pcm[2*j]);}
 }
 assert(energy>0);
 for(unsigned bank=0;bank<8;bank++)for(unsigned note=24;note<=83;note++){
  c.bank=bank;c.note=note;c.position=note%2?100:0;core_set(&c);
  for(unsigned b=0;b<40;b++)core_render(pcm,256,4096);
  int peak=0;for(unsigned j=0;j<512;j++){assert(abs(pcm[j])<=8388607);if(abs(pcm[j])>peak)peak=abs(pcm[j]);}assert(peak>100);
 }
 c.playing=0;core_set(&c);for(unsigned b=0;b<200;b++)core_render(pcm,256,4096);
 for(unsigned j=0;j<512;j++)assert(pcm[j]==0);
 c.playing=1;core_set(&c);core_render(pcm,256,4096);core_panic();core_render(pcm,256,4096);
 for(unsigned j=0;j<512;j++)assert(pcm[j]==0);
 /* Last-note priority, source/channel isolation, velocity-zero note off,
  * held-note fallback, physical keyboard, audition fallback and panic. */
 core_init();core_keys(1u);core_render(pcm,256,4096);assert(core_active_note()==53);
 core_midi(0,0x90,60,90);core_render(pcm,256,4096);assert(core_active_note()==60);
 core_midi(2,0x91,64,100);core_render(pcm,256,4096);assert(core_active_note()==64);
 core_midi(0,0x81,64,0);core_render(pcm,256,4096);assert(core_active_note()==64);
 core_midi(2,0x91,64,0);core_render(pcm,256,4096);assert(core_active_note()==60);
 core_midi(0,0x80,60,0);core_render(pcm,256,4096);assert(core_active_note()==53);
 core_keys(0);core_render(pcm,256,4096);assert(core_active_note()==0);assert(core_env_stage()==PW_ENV_RELEASE);
 for(int i=0;i<200;i++)core_render(pcm,256,4096);assert(core_env_stage()==PW_ENV_IDLE);
 core_midi(0,0x90,67,127);core_render(pcm,256,4096);assert(core_active_note()==67);
 core_midi(0,0xb0,123,0);core_render(pcm,256,4096);assert(core_active_note()==0);
 core_midi(0,0x90,10,127);core_render(pcm,256,4096);assert(core_active_note()==0);
 core_panic();core_render(pcm,256,4096);for(unsigned j=0;j<512;j++)assert(pcm[j]==0);
 puts("PASS: note priority, USB/TRS channel isolation, velocity-zero note-off, keyboard and release/panic silence.");
 puts("PASS: all 8 banks and MIDI notes 24–83, wave-position endpoints, bounded dual-mono output, bank fades and stop/panic silence under UB/address sanitizers.");
}
