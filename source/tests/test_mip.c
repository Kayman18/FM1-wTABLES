#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
#include "../fm1/app/pw_mip.h"
static int32_t pcm[512];
static void render(unsigned n){while(n--){core_render(pcm,256,4096);for(unsigned j=0;j<512;j++){assert(pcm[j]>=-8388608&&pcm[j]<=8388607);assert(pcm[j]==pcm[j^1]);}}}
int main(void){
 core_settings c={0,48,43,0,75,{0,0,127,0},{0,96,0,100},0,{0,0,127,0}};c.wave_quality=1;core_init();core_set(&c);
 core_midi(0,0x90,24,127);core_midi(2,0x91,83,127);render(5);
 assert(core_note_mip(0,0,24)==1);assert(core_note_mip(2,1,83)==6);uint32_t inc=core_note_increment(2,1,83),phase=core_note_osc_phase(2,1,83);
 c.coarse_tune=12;core_set(&c);render(5);assert(core_note_mip(2,1,83)==7);assert(core_note_increment(2,1,83)==inc*2);assert(core_note_osc_phase(2,1,83)!=phase);
 c.bend_range_offset=22;core_set(&c);core_midi(2,0xe1,0,0);render(20);assert(core_note_mip(2,1,83)==5);assert(core_note_held(0,0,24)&&core_note_held(2,1,83));
 phase=core_note_osc_phase(2,1,83);inc=core_note_increment(2,1,83);c.wave_quality=0;core_set(&c);render(3);assert(!core_note_mip(2,1,83));assert(core_note_osc_phase(2,1,83)==phase+inc*768u);phase=core_note_osc_phase(2,1,83);c.wave_quality=1;core_set(&c);render(3);assert(core_note_mip(2,1,83)==5);assert(core_note_osc_phase(2,1,83)==phase+inc*768u);
 for(unsigned i=0;i<24000;i++){
  c.wave_quality=(uint8_t)(i%3!=0);c.bank=(uint8_t)(i%8);c.position=(uint8_t)(i%101);c.coarse_tune=(int8_t)((int)(i%49)-24);c.fine_tune=(int8_t)((int)(i%201)-100);c.env2_pitch=(int8_t)((int)(i%241)-120);c.env2=(pw_env_params){(uint8_t)(i%128),60,64,60};c.lfo=(pw_lfo_params){(int8_t)((int)(i%201)-100),100,96,100};c.filter.mode=(uint8_t)(i%4);c.filter.resonance=(uint8_t)(i%101);c.amp_attack_shape=(int8_t)((int)(i%128)-64);c.lfo_phase_mode=(uint8_t)(i%4);core_set(&c);
  unsigned p=i%3,ch=i%16,n=24+i%60;core_midi(p,(uint8_t)(0x90|ch),(uint8_t)n,127);core_midi(p,(uint8_t)(0xe0|ch),(uint8_t)(i%128),(uint8_t)((i*7)%128));if(i%3==0)core_midi(p,(uint8_t)(0x80|ch),(uint8_t)n,0);render(1);
  for(unsigned v=0;v<4;v++)assert(core_voice_mip(v)<=10);assert(core_voice_count()<=4);
 }
 core_panic();render(1);for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 puts("PASS: independent low/high-note mip selection, tuning/bend follows actual pitch, Legacy/Auto transitions preserve held notes, all banks/resolutions in 24000 sanitized quality/pitch/modulation/filter/steal blocks; dual mono, bounded PCM and panic silence.");
}
