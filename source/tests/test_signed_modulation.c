#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
#include "../fm1/app/pw_filter_mod.h"
static int32_t pcm[512];
static void render(unsigned blocks){while(blocks--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);}}
static float wave[2][150],cut[2][150];
int main(void){
 assert(pw_filter_mod_signed(70,0,100,1,-100,1,0,0)==70);
 assert(pw_filter_mod_signed(70,20,-127,1,0,0,0,0)==0);
 assert(pw_filter_mod_signed(70,20,127,1,0,0,0,0)==127);
 core_settings c={0,48,43,0,65,{0,0,127,0},{PW_FILTER_LP,100,30,100},-40,{0,0,127,0},-20};
 core_init();core_set(&c);core_midi(0,0x90,48,127);render(30);assert(fabsf(core_note_cutoff(0,0,48)-40)<.01f);
 c.filter_env_depth=100;c.filter_env2_depth=-100;c.filter.cutoff=70;core_set(&c);render(30);assert(fabsf(core_note_cutoff(0,0,48)-70)<.01f);
 c.filter_env_depth=-70;c.filter_env2_depth=0;c.env.attack=90;c.filter.cutoff=100;core_init();core_set(&c);core_midi(0,0x90,48,127);render(80);float old=core_note_cutoff(0,0,48);core_midi(0,0x90,55,127);render(8);float young=core_note_cutoff(0,0,55);assert(young>old+20);assert(core_held_count()==2);
 for(unsigned side=0;side<2;side++){
  core_init();c.position=50;c.env=(pw_env_params){0,0,127,0};c.filter.cutoff=64;c.filter_env_depth=c.filter_env2_depth=0;c.lfo=(pw_lfo_params){65,side?-40:40,side?-20:20,0};core_set(&c);core_midi(0,0x90,48,127);
  for(unsigned i=0;i<150;i++){render(1);wave[side][i]=core_note_wave(0,0,48);cut[side][i]=core_note_cutoff(0,0,48);}
 }
 float min=100,max=0;
 for(unsigned i=0;i<150;i++){if(i>=4)assert(fabsf(wave[0][i]+wave[1][i]-100)<.2f);assert(fabsf(cut[0][i]+cut[1][i]-128)<.001f);if(wave[1][i]<min)min=wave[1][i];if(wave[1][i]>max)max=wave[1][i];}
 assert(max-min>30);
 for(unsigned i=0;i<8000;i++){
  c.filter_env_depth=(int8_t)((int)(i%255)-127);c.filter_env2_depth=(int8_t)((int)((i*7)%255)-127);c.lfo.wave=(int8_t)((int)(i%201)-100);c.lfo.cutoff=(int8_t)((int)((i*13)%255)-127);c.tracking.velocity_cutoff=(int8_t)((int)((i*11)%255)-127);c.tracking.pitch_cutoff=(int8_t)((int)((i*17)%201)-100);c.filter.resonance=(uint8_t)(i%101);c.filter.mode=(uint8_t)(i%4);c.bank=(uint8_t)(i%8);c.unison_mode=(uint8_t)(i%3);core_set(&c);core_midi(i%3,(uint8_t)(0x90|i%16),(uint8_t)(24+i%60),(uint8_t)(1+i%127));render(1);
  for(unsigned v=0;v<4;v++){float x=core_voice_cutoff(v);assert(isfinite(x)&&x>=-1&&x<=127);}
 }
 core_panic();render(1);for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 puts("PASS: negative Amp/Env2 cutoff, opposite-route cancellation, independent per-note downward envelopes, mirrored signed LFO wave/filter sweeps, 8000 sanitized signed modulation/filter/tracking/unison/steal blocks and panic silence.");
}
