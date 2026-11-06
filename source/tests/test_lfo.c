#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/pw_lfo.h"
#include "../fm1/app/test_engine.h"
static int32_t pcm[512];
static void render(unsigned count){while(count--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++){assert(pcm[i]>=-8388608&&pcm[i]<=8388607);assert(pcm[i]==pcm[i^1]);}}}
int main(void){
 double worst=0;
 for(int cents=-10000;cents<=10000;cents++){
  float c=(float)cents/100.0f;int32_t scale=pw_lfo_pitch_scale(c);double ref=4096.0*pow(2.0,(double)c/1200.0);double err=fabs(scale-ref);if(err>worst)worst=err;assert(err<.502);
 }
 for(int rate=1;rate<=100;rate++){
  double actual=(double)pw_lfo_increment(rate)*44100.0/4294967296.0;assert(fabs(actual-pw_lfo_hz(rate))<0.000011);
 }
 pw_lfo a={0},b={0};for(unsigned i=0;i<40000;i++){float x=pw_lfo_tick(&a,pw_lfo_increment(80)),y=pw_lfo_tick(&b,pw_lfo_increment(-80));assert(isfinite(x)&&fabsf(x)<=1&&fabsf(x+y)<.0062f);}
 uint32_t frozen=a.phase;float value=pw_lfo_value(&a);for(unsigned i=0;i<100;i++)assert(pw_lfo_tick(&a,0)==value&&a.phase==frozen);
 core_settings c={0,48,50,0,75,{0,0,127,60},{PW_FILTER_LP,60,20,100},0,{60,70,64,50},0,{80,30,24,50}};
 core_init();core_set(&c);core_midi(0,0x90,48,127);render(30);core_midi(2,0x91,55,127);render(10);
 float x=core_note_lfo(0,0,48),y=core_note_lfo(2,1,55);assert(fabsf(x-y)>.3f);assert(core_held_count()==2);
 c.lfo.rate=0;core_set(&c);render(1);uint32_t phase=core_lfo_phase();render(100);assert(core_lfo_phase()==phase);
 for(unsigned i=0;i<18000;i++){
  c.lfo.rate=(int8_t)(i%201-100);c.lfo.wave=(uint8_t)(i%2?100:0);c.lfo.cutoff=(uint8_t)(i%2?96:0);c.lfo.pitch=(uint8_t)(i%2?100:0);c.filter.mode=(uint8_t)(i%4);c.filter.cutoff=(uint8_t)(i%128);c.filter.resonance=(uint8_t)(i%101);c.bank=(uint8_t)(i%8);c.filter_env_depth=(uint8_t)(i%128);c.filter_env2_depth=(uint8_t)((i*3)%128);core_set(&c);
  core_midi(i%2?0:2,(uint8_t)(0x90|i%16),(uint8_t)(24+i%60),127);render(1);assert(core_voice_count()<=4);assert(isfinite(core_lfo_level())&&fabsf(core_lfo_level())<=1.00001f);
 }
 core_panic();render(1);assert(!core_voice_count());for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 printf("PASS: signed rate accuracy, stop/reverse, pitch scaling (<%.5f Q12 error), independent note phases, rate-zero freeze, 18000 four-voice modulation/steal/extreme-control stress blocks and panic silence.\n",worst);
}
