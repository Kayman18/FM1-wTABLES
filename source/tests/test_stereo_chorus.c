#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
#include "../fm1/app/pw_chorus.h"
#include "../fm1/app/pw_stereo.h"
static int32_t pcm[512];
static void render(unsigned blocks){while(blocks--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);}}
int main(void){
 for(int spread=-100;spread<=100;spread++)for(unsigned mode=1;mode<=3;mode++)for(unsigned v=0;v<4;v++){
  float l,r;pw_stereo_gains(mode,spread,v,4,24+(int)v*19,&l,&r);assert(isfinite(l)&&isfinite(r)&&l>=0&&r>=0&&l<=1&&r<=1);assert(fabsf(l*l+r*r-1)<1e-6f);
 }
 for(unsigned mode=1;mode<=2;mode++)for(unsigned rate=44100;rate<=48000;rate+=3900){
  pw_chorus c;pw_chorus_reset(&c);float in[4],out[4];
  for(unsigned n=0;n<12000;n++){
   for(unsigned k=0;k<4;k++)in[k]=n<6000?sinf((float)n*.05f+(float)k):0.0f;
   pw_chorus_tick(&c,in,out,mode,rate);
   for(unsigned k=0;k<4;k++){assert(isfinite(out[k])&&fabsf(out[k])<1.415f);if(n>6256)assert(out[k]==0.0f);}
  }
  assert(c.write==(12000&255));
 }
 core_settings c={0,48,0,0,75,{0,0,127,0},{0,96,0,100},0,{0,0,127,0}};c.wave_quality=1;c.stereo_mode=1;c.stereo_spread=100;c.chorus_mix=100;
 core_init();core_set(&c);core_midi(0,0x90,48,127);render(5);assert(core_output_level(0)>.01f&&core_output_level(1)==0.0f);
 c.coarse_tune=12;core_set(&c);render(8);assert(core_output_level(0)>.01f&&core_output_level(1)==0.0f);
 c.stereo_spread=-100;core_set(&c);render(20);assert(core_output_level(1)>.01f&&core_output_level(0)==0.0f);
 c.stereo_mode=0;c.coarse_tune=0;c.chorus_mode=1;core_set(&c);render(100);int different=0;for(unsigned i=0;i<256;i++)different|=pcm[2*i]!=pcm[2*i+1];assert(different);
 c.chorus_mode=2;core_set(&c);render(100);different=0;for(unsigned i=0;i<256;i++)different|=pcm[2*i]!=pcm[2*i+1];assert(different);
 c.chorus_mode=0;core_set(&c);render(4);for(unsigned i=0;i<256;i++)assert(pcm[2*i]==pcm[2*i+1]);
 c.chorus_mode=1;core_set(&c);render(10);core_midi(0,0x80,48,0);render(50);for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 for(unsigned i=0;i<24000;i++){
  c.chorus_mode=(uint8_t)(i%3);c.chorus_mix=(uint8_t)(i%101);c.stereo_mode=(uint8_t)(i%4);c.stereo_spread=(int8_t)((int)(i%201)-100);c.wave_quality=(uint8_t)(i%2);c.bank=(uint8_t)(i%8);c.position=(uint8_t)(i%101);c.env2_pitch=(int8_t)((int)(i%241)-120);c.lfo=(pw_lfo_params){(int8_t)((int)(i%201)-100),100,96,100};c.coarse_tune=(int8_t)((int)(i%49)-24);c.filter.mode=(uint8_t)(i%4);c.filter.resonance=(uint8_t)(i%101);core_set(&c);
  unsigned p=i%3,ch=i%16,n=24+i%60;core_midi(p,(uint8_t)(0x90|ch),(uint8_t)n,127);core_midi(p,(uint8_t)(0xe0|ch),(uint8_t)(i%128),(uint8_t)((i*7)%128));if(i%7==0)core_midi(p,(uint8_t)(0xb0|ch),64,127);if(i%11==0)core_midi(p,(uint8_t)(0xb0|ch),121,0);if(i%3==0)core_midi(p,(uint8_t)(0x80|ch),(uint8_t)n,0);render(1);assert(core_voice_count()<=4);assert(core_output_level(0)<=1&&core_output_level(1)<=1);
 }
 core_panic();render(1);for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 puts("PASS: four-slot equal-power spread/limits, both chorus modes at both rates, finite bounded kernel/tail silence, tuning retains pan, smooth pan/effect changes, restored mono, 24000 sanitized stereo/chorus/mip/pitch/filter/steal blocks and panic flush.");
}
