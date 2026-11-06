#include "../fm1/app/test_engine.h"
#include "../fm1/app/pw_chord.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int32_t pcm[512];
static core_settings settings(void){core_settings s={0};s.note=48;s.level=60;s.env=(pw_env_params){0,0,100,0};s.env2=(pw_env_params){0,0,64,0};s.filter.cutoff=96;s.wave_quality=1;s.velocity_amount=100;s.delay_time_ms=500;s.chord_shape=6;return s;}
static void render(unsigned n){while(n--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);}}
static void notes(unsigned root,unsigned selection,unsigned scale,int key,unsigned inversion){
 uint8_t pitches[5];unsigned n=pw_chord_notes(root,key,scale,pw_chord_shape(selection),inversion,pitches),count=0;
 for(unsigned j=0;j<n;j++)if(pitches[j]>=24&&pitches[j]<=83){unsigned found=0;for(unsigned i=0;i<4;i++)found+=core_voice_note(i)==pitches[j];assert(found);count++;}
 assert(core_held_count()==count);
}
int main(void){
 assert(sizeof(core_settings)==76);
 core_settings s=settings();
 for(unsigned shape=1;shape<=11;shape++)for(unsigned scale=0;scale<9;scale++)for(unsigned inv=0;inv<4;inv++)for(unsigned root=24;root<=83;root++){
  core_init();s.chord_shape=shape;s.chord_scale=scale;s.chord_inversion=inv;s.chord_key=(int)(root%25)-12;s.unison_mode=root%3;core_set(&s);core_midi(0,0x90,root,100);notes(root,shape,scale,s.chord_key,inv);core_midi(0,0x80,root,0);assert(!core_held_count());
 }
 /* Two overlapping octave chords must retain independent owners, even sharing pitch 60. */
 core_init();s=settings();s.chord_shape=2;core_set(&s);core_midi(0,0x90,48,100);core_midi(0,0x90,60,100);assert(core_held_count()==4);core_midi(0,0x80,48,0);assert(core_note_held(0,0,60));assert(!core_note_held(0,0,48));assert(core_held_count()==2);render(10);
 /* Changes affect the next strike; release uses the captured root. */
 s.chord_shape=11;s.chord_key=7;s.chord_inversion=3;core_set(&s);core_midi(0,0x80,60,0);assert(!core_held_count());render(20);assert(!core_voice_count());
 /* Group stealing, sustain, port/channel separation and zero velocity. */
 core_init();s=settings();s.chord_shape=2;core_set(&s);core_midi(0,0x90,48,100);core_midi(2,0x91,48,100);core_midi(0,0xb0,64,127);core_midi(0,0x90,48,0);assert(core_note_sustained(0,0,48));assert(core_note_held(2,1,48));core_midi(0,0xb0,64,0);render(20);assert(!core_note_sustained(0,0,48));assert(core_note_held(2,1,48));core_midi(2,0xb1,123,0);render(20);assert(!core_voice_count());
 core_init();s=settings();core_set(&s);core_midi(0,0x90,48,127);render(2);core_midi(0,0x90,50,127);assert(!core_note_held(0,0,48));assert(core_note_layers(0,0,50)==3);core_midi(0,0x80,48,0);assert(core_note_layers(0,0,50)==3);
 /* Re-strike changed shapes, including zero playable pitches at high inversion. */
 s.chord_inversion=3;s.chord_shape=3;core_set(&s);core_midi(0,0x90,83,100);assert(!core_note_held(0,0,83));
 for(unsigned j=0;j<3000;j++){
  s.chord_shape=j%12;s.chord_scale=j%9;s.chord_key=(int)(j%25)-12;s.chord_inversion=j%4;s.unison_mode=j%3;s.trigger_mode=j%2;core_set(&s);
  core_midi(j%3,0x90|(j%16),24+j%60,1+j%127);render(1);
  if(j%3==0)core_midi(j%3,0x80|(j%16),24+j%60,0);
  if(j%31==0)core_midi(j%3,0xb0|(j%16),120,0);
  assert(core_held_count()<=4);
 }
 core_panic();render(10);assert(!core_voice_count());
 puts("PASS: chord engine: 23760 pitch/ownership cases, overlapping roots, settings changes, sustain, ports/channels, group stealing, zero-velocity off, panic and 3000 sanitized stress blocks.");
}
