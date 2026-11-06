#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
static core_settings c;static int32_t pcm[512];
static void render(unsigned n){while(n--){core_render(pcm,256,4096);for(unsigned j=0;j<512;j++)assert(pcm[j]>=-8388608&&pcm[j]<=8388607);assert(core_voice_count()<=4);}}
static void on(unsigned p,unsigned ch,unsigned n){core_midi(p,(uint8_t)(0x90|ch),(uint8_t)n,127);}
static void off(unsigned p,unsigned ch,unsigned n){core_midi(p,(uint8_t)(0x80|ch),(uint8_t)n,0);}
static void boot(unsigned mode){c=(core_settings){0,48,0,0,75,{0,0,127,0},{0,96,0,100},0,{0,0,127,0}};c.wave_quality=1;c.unison_mode=(uint8_t)mode;c.unison_detune=50;core_init();core_set(&c);}
int main(void){
 boot(1);on(0,0,48);render(3);assert(core_note_layers(0,0,48)==2);assert(core_voice_note(0)==48&&core_voice_note(3)==48);assert(core_voice_detune(0)<-49&&core_voice_detune(3)>49);
 on(0,0,55);render(3);assert(core_note_layers(0,0,55)==2&&core_held_count()==4);on(0,0,48);render(3);assert(core_note_layers(0,0,48)==2&&core_note_layers(0,0,55)==2);
 on(0,0,60);render(3);assert(!core_note_layers(0,0,55)&&core_note_layers(0,0,48)==2&&core_note_layers(0,0,60)==2);off(0,0,55);render(3);assert(core_held_count()==4);
 core_midi(0,0xb0,64,127);off(0,0,48);render(3);assert(core_note_sustained(0,0,48));on(0,0,65);render(3);assert(!core_note_layers(0,0,48)&&core_note_layers(0,0,60)==2&&core_note_layers(0,0,65)==2);
 on(2,0,60);render(3);assert(!core_note_layers(0,0,60)&&core_note_layers(2,0,60)==2);off(0,0,60);render(3);assert(core_note_held(2,0,60));
 c.unison_mode=2;core_set(&c);render(3);assert(core_note_layers(2,0,60)==2);on(0,0,67);render(3);assert(core_note_layers(0,0,67)==4&&!core_note_layers(2,0,60));
 c.unison_mode=0;core_set(&c);render(3);assert(core_note_layers(0,0,67)==4);on(0,0,48);render(3);assert(core_note_layers(0,0,48)==1&&!core_note_layers(0,0,67));
 boot(2);c.position=50;c.unison_detune=0;c.unison_wave_spread=80;core_set(&c);on(0,0,60);render(20);
 for(unsigned i=0;i<4;i++){assert(fabsf(core_voice_wave(i)-(10.f+(float)i*80.f/3.f))<.2f);assert(core_voice_detune(i)==0);}
 c.unison_wave_spread=-80;core_set(&c);render(20);for(unsigned i=0;i<4;i++)assert(fabsf(core_voice_wave(i)-(90.f-(float)i*80.f/3.f))<.2f);
 c.unison_detune=100;core_set(&c);render(20);assert(core_voice_detune(0)<-99&&core_voice_detune(3)>99);uint32_t low=core_voice_increment(0),high=core_voice_increment(3);assert(high>low);
 core_midi(0,0xb0,123,0);render(30);assert(!core_voice_count());
 boot(2);c.level=100;c.unison_detune=0;c.chorus_mode=2;c.chorus_mix=100;core_set(&c);on(0,0,48);render(200);for(unsigned i=0;i<512;i++)assert(pcm[i]!=8388607&&pcm[i]!=-8388608);
 for(unsigned k=0;k<18000;k++){
  c.unison_mode=(uint8_t)(k%3);c.unison_detune=(uint8_t)(k%101);c.unison_wave_spread=(int8_t)((int)(k%201)-100);c.wave_quality=(uint8_t)(k%2);c.bank=(uint8_t)(k%8);c.position=(uint8_t)(k%101);c.coarse_tune=(int8_t)((int)(k%49)-24);c.chorus_mode=(uint8_t)(k%3);c.chorus_mix=(uint8_t)(k%101);c.stereo_mode=(uint8_t)(k%4);c.stereo_spread=(int8_t)((int)(k%201)-100);c.filter.mode=(uint8_t)(k%4);c.filter.resonance=(uint8_t)(k%101);c.lfo=(pw_lfo_params){50,100,96,100};core_set(&c);
  unsigned p=k%3,ch=k%16,n=24+k%60;on(p,ch,n);if(k%2)off(p,ch,n);if(k%7==0)core_midi(p,(uint8_t)(0xb0|ch),64,127);if(k%11==0)core_midi(p,(uint8_t)(0xb0|ch),121,0);if(k%23==0)core_midi(p,(uint8_t)(0xb0|ch),120,0);render(1);
  if(core_note_held(p,ch,n))assert(core_note_layers(p,ch,n)==(1u<<c.unison_mode));
 }
 core_panic();render(1);for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 puts("PASS: 2/4-layer allocation, balanced stereo pairs, whole-group stealing/retrigger, pedal/source isolation, next-note layer changes, live detune/signed wave positions, corrected chorus headroom, 18000 sanitized mixed-mode allocation/DSP blocks, four-slot bound and panic silence.");
}
