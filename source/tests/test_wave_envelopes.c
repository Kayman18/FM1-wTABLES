#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
#include "../fm1/app/pw_wave_mod.h"
static int32_t pcm[512];
static void render(unsigned blocks){while(blocks--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++){assert(pcm[i]>=-8388608&&pcm[i]<=8388607);assert(pcm[i]==pcm[i^1]);}}}
int main(void){
 assert(pw_wave_mod_position(895,50,1,-50,1,0,0)==895);
 assert(pw_wave_mod_position(895,100,1,100,1,100,1)==1791);
 assert(pw_wave_mod_position(895,-100,1,-100,1,100,-1)==0);
 core_settings c={0,48,0,0,75,{90,60,20,80},{0,96,0,100},0,{90,60,20,80},0,{40,0,0,0},100,0};
 core_init();core_set(&c);core_midi(0,0x90,48,127);render(20);float early=core_note_wave(0,0,48);render(40);float high=core_note_wave(0,0,48);assert(high>early+10);
 core_midi(2,0x91,55,127);render(10);float older=core_note_wave(0,0,48),younger=core_note_wave(2,1,55);assert(older>younger+20);
 core_midi(2,0x81,55,0);render(20);assert(core_note_wave(2,1,55)<younger&&core_note_held(0,0,48));
 /* Env2 sweeps downward while amplitude stays sustained. */
 c.position=100;c.env=(pw_env_params){0,0,127,60};c.wave_env_depth=0;c.wave_env2_depth=-100;
 core_init();core_set(&c);core_midi(0,0x90,60,127);render(20);float down_early=core_note_wave(0,0,60);render(40);float down_late=core_note_wave(0,0,60);assert(down_late<down_early-10&&core_env_level()==1);
 /* Equal opposing routes cancel before clamping, rather than losing range. */
 c.position=50;c.env=(pw_env_params){0,0,127,0};c.env2=c.env;c.wave_env_depth=100;c.wave_env2_depth=-100;
 core_init();core_set(&c);core_midi(0,0x90,60,127);render(40);assert(fabsf(core_note_wave(0,0,60)-50)<.1f);
 for(unsigned i=0;i<18000;i++){
  c.wave_env_depth=(int8_t)((int)(i%201)-100);c.wave_env2_depth=(int8_t)(100-(int)(i%201));c.lfo=(pw_lfo_params){(int8_t)((int)(i%201)-100),(uint8_t)(i%101),(uint8_t)(i%97),(uint8_t)(i%101)};
  c.env=(pw_env_params){(uint8_t)(i%128),(uint8_t)((i*3)%128),(uint8_t)((i*7)%128),(uint8_t)((i*11)%128)};c.env2=(pw_env_params){(uint8_t)(127-i%128),0,(uint8_t)(i%128),127};
  c.filter.mode=(uint8_t)(i%4);c.filter.resonance=(uint8_t)(i%101);c.bank=(uint8_t)(i%8);c.position=(uint8_t)(i%2?100:0);core_set(&c);
  core_midi(i%2?0:2,(uint8_t)(0x90|i%16),(uint8_t)(24+i%60),127);render(1);
  for(unsigned v=0;v<4;v++){float pos=core_voice_wave(v);assert(isfinite(pos)&&pos>=-1&&pos<=100.001f);}
 }
 core_panic();render(1);assert(!core_voice_count());for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 printf("PASS: signed wave sweeps, isolated Amp/Env2 routes, independent staggered notes (%.2f/%.2f), independent release, opposing-route cancellation, clamps and 18000 four-voice combined-modulation stress blocks; panic silent.\n",older,younger);
}
