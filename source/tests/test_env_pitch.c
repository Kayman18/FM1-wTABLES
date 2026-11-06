#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "../fm1/app/pw_env_pitch.h"
#include "../fm1/app/test_engine.h"
static int32_t pcm[512];
static void render(unsigned count){while(count--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++){assert(pcm[i]>=-8388608&&pcm[i]<=8388607);assert(pcm[i]==pcm[i^1]);}}}
int main(void){
 double worst=0;
 for(int i=-13000;i<=13000;i++){
  float cents=(float)i*.1f;int32_t units=(int32_t)(cents*(524288.0f/1200.0f));int32_t scale=pw_env_pitch_scale(units);
  double actual=1200.0*log2((double)scale/4096.0);double error=fabs(actual-cents);if(error>worst)worst=error;assert(error<2.2);
 }
 assert(pw_env_pitch_scale(0)==4096);assert(pw_env_pitch_scale(524288)==8192);assert(pw_env_pitch_scale(-524288)==2048);
 core_settings c={0,48,0,0,75,{0,0,127,90},{0,72,0,100},0,{90,60,0,80},0,{40,0,0,0},0,0,{0,0,0,0},PW_LFO_KEEP,0,120};
 core_init();core_set(&c);core_midi(0,0x90,48,127);render(20);float early=core_note_pitch(0,0,48);render(40);float later=core_note_pitch(0,0,48);assert(later>early+100&&core_env_level()==1.0f);
 core_midi(2,0x91,55,127);render(10);float older=core_note_pitch(0,0,48),younger=core_note_pitch(2,1,55);assert(older>younger+300);
 core_midi(2,0x81,55,0);render(10);assert(core_note_pitch(2,1,55)<younger&&core_note_held(0,0,48));
 c.env2_pitch=-120;core_set(&c);render(2);assert(core_note_pitch(0,0,48)<0&&core_note_pitch_scale(0,0,48)<4096);
 for(unsigned i=0;i<18000;i++){
  c.amp_attack_shape=(int8_t)((int)(i%128)-64);c.amp_decay_shape=(int8_t)((int)((i*3)%128)-64);c.env2_attack_shape=(int8_t)((int)((i*5)%128)-64);c.env2_decay_shape=(int8_t)((int)((i*7)%128)-64);
  c.env2_pitch=(int8_t)((int)(i%241)-120);c.env2=(pw_env_params){(uint8_t)(i%128),(uint8_t)((i*3)%128),(uint8_t)((i*7)%128),(uint8_t)((i*11)%128)};
  c.lfo=(pw_lfo_params){(int8_t)((int)(i%201)-100),(uint8_t)(i%101),(uint8_t)(i%97),(uint8_t)(i%101)};c.lfo_phase_mode=(uint8_t)(i%4);c.lfo_spread=(uint8_t)(i%91);c.filter.mode=(uint8_t)(i%4);c.filter.resonance=(uint8_t)(i%101);c.bank=(uint8_t)(i%8);core_set(&c);
  core_midi(i%2?0:2,(uint8_t)(0x90|i%16),(uint8_t)(24+i%60),127);render(1);assert(core_voice_count()<=4);
  for(unsigned v=0;v<4;v++){float pitch=core_voice_pitch(v);assert(isfinite(pitch)&&(pitch== -2000.0f||(pitch>=-1300.01f&&pitch<=1300.01f)));}
 }
 core_panic();render(1);assert(!core_voice_count());for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 printf("PASS: original pitch lookup endpoints and curve (<%.3f cents error over exposed range), independent signed Env2 bends (%.1f/%.1f cents), isolated release, 18000 four-voice shape/pitch/LFO/phase/filter/steal stress blocks and panic silence.\n",worst,older,younger);
}
