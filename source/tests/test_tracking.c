#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
static int32_t pcm[512];
static void render(unsigned count){while(count--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++){assert(pcm[i]>=-8388608&&pcm[i]<=8388607);assert(pcm[i]==pcm[i^1]);}}}
int main(void){
 core_settings c={0,48,0,0,75,{0,0,127,60},{PW_FILTER_LP,40,0,100},0,{60,70,64,50},0,{40,0,0,0},0,0,{100,0,24,0}};
 core_init();core_set(&c);core_midi(0,0x90,48,32);core_midi(2,0x91,48,96);render(30);
 float soft=core_note_wave(0,0,48),hard=core_note_wave(2,1,48);assert(fabsf(soft-25)<.1f&&fabsf(hard-75)<.1f);
 assert(fabsf(core_note_cutoff(0,0,48)-46)<.01f&&fabsf(core_note_cutoff(2,1,48)-58)<.01f);
 /* Per-note velocity remains attached to its release tail. */
 core_midi(2,0x81,48,0);render(10);assert(fabsf(core_note_wave(2,1,48)-hard)<.01f&&core_note_held(0,0,48));
 c.position=100;c.tracking.velocity_wave=-100;c.tracking.velocity_cutoff=-24;core_set(&c);render(30);
 assert(core_note_wave(0,0,48)>70&&fabsf(core_note_cutoff(0,0,48)-34)<.01f);
 c.position=25;c.tracking=(pw_tracking_params){0,100,0,100};core_init();core_set(&c);core_midi(0,0x90,48,127);core_midi(0,0x90,60,127);render(30);
 assert(fabsf(core_note_wave(0,0,60)-core_note_wave(0,0,48)-9.375f)<.1f);
 assert(fabsf(core_note_cutoff(0,0,60)-core_note_cutoff(0,0,48)-12)<.01f);
 c.tracking.pitch_cutoff=-100;core_set(&c);render(2);assert(core_note_cutoff(0,0,60)<core_note_cutoff(0,0,48));
 for(unsigned i=0;i<18000;i++){
  c.tracking=(pw_tracking_params){(int8_t)((int)(i%201)-100),(int8_t)(100-(int)(i%201)),(int8_t)((int)(i%255)-127),(int8_t)((int)((i*3)%201)-100)};
  c.wave_env_depth=(int8_t)((int)(i%201)-100);c.wave_env2_depth=(int8_t)(100-(int)(i%201));c.filter_env_depth=(uint8_t)(i%128);c.filter_env2_depth=(uint8_t)((i*3)%128);c.lfo=(pw_lfo_params){(int8_t)((int)(i%201)-100),(uint8_t)(i%101),(uint8_t)(i%97),(uint8_t)(i%101)};
  c.filter.mode=(uint8_t)(i%4);c.filter.cutoff=(uint8_t)(i%128);c.filter.resonance=(uint8_t)(i%101);c.bank=(uint8_t)(i%8);c.position=(uint8_t)(i%2?100:0);core_set(&c);
  core_midi(i%2?0:2,(uint8_t)(0x90|i%16),(uint8_t)(24+i%60),(uint8_t)(1+i%127));render(1);
  for(unsigned v=0;v<4;v++){float wave=core_voice_wave(v),cut=core_voice_cutoff(v);assert(isfinite(wave)&&wave>=-1&&wave<=100.001f);assert(isfinite(cut)&&cut>=-1&&cut<=127);}
 }
 core_panic();render(1);for(unsigned i=0;i<512;i++)assert(!pcm[i]);assert(!core_voice_count());
 printf("PASS: independent soft/hard note positions %.2f/%.2f; signed tracking, retained release velocity, MIDI-48 reference and octave mapping; 18000 four-voice combined tracking/modulation/steal stress blocks and panic silence.\n",soft,hard);
}
