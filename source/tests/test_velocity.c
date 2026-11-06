#include <assert.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
#include "../fm1/app/pw_velocity.h"
static int32_t pcm[514];
int main(void){
 assert(pw_velocity_gain(1,0)==1.0f);assert(pw_velocity_gain(127,100)==127.0f/128.0f);
 assert(pw_velocity_gain(255,100)==127.0f/128.0f);assert(pw_velocity_gain(0,-10)==1.0f);
 assert(!pw_velocity_map(127,0,1));assert(pw_velocity_map(0,1,0)==1);
 core_settings c={0,48,30,0,75,{0,0,127,0},{0,96,0,100},0,{0,0,127,0}};
 core_init();c.wave_quality=1;c.velocity_mode=1;c.velocity_amount=100;c.velocity_curve=1;core_set(&c);
 for(unsigned port=0;port<3;port++){core_midi(port,0x90,48,1);assert(core_note_held(port,0,48));core_midi(port,0x90,48,0);assert(!core_note_held(port,0,48));}
 for(unsigned block=0;block<4000;block++){
  c.velocity_mode=(uint8_t)(block%2);c.velocity_amount=(uint8_t)(block%101);c.velocity_curve=(uint8_t)((block/3)%4);c.bank=(uint8_t)(block%8);c.filter.mode=(uint8_t)(block%4);c.unison_mode=(uint8_t)(block%3);c.tracking.velocity_wave=(int8_t)((int)(block%201)-100);c.tracking.velocity_cutoff=(int8_t)((int)(block%255)-127);
  core_set(&c);core_midi(block%3,(uint8_t)(0x90|block%16),(uint8_t)(24+block%60),(uint8_t)(block%128));
  unsigned frames=1+block%257;core_render(pcm,frames,4096);
  for(unsigned i=0;i<frames*2;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);
 }
 core_panic();core_render(pcm,257,4096);for(unsigned i=0;i<514;i++)assert(!pcm[i]);
 puts("PASS: sanitized source/legacy switching, amount and curve sweeps, all MIDI ports, fixed-mode velocity-zero note-off, variable block sizes, tracking/filter/unison/stealing stress and panic silence.");
}
