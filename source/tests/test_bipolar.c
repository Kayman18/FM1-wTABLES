#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
static int32_t pcm[512];
static void run(unsigned n){while(n--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);}}
int main(void){
 pw_env e;pw_env_init(&e);pw_env_set(&e,(pw_env_params){0,0,-127,0});pw_env_on(&e);
 for(unsigned i=0;i<2000;i++)pw_env_tick(&e);assert(e.stage==PW_ENV_SUSTAIN&&e.level==-1);
 pw_env_off(&e);float last=e.level;for(unsigned i=0;i<1000;i++){float v=pw_env_tick(&e);assert(v>=last&&v<=0);last=v;}assert(e.stage==PW_ENV_IDLE&&e.level==0);
 core_settings c={0,48,50,0,65,{0,0,127,0},{PW_FILTER_LP,80,0,100},0,{0,0,-127,0},30};c.wave_env2_depth=30;c.env2_pitch=60;
 core_init();core_set(&c);core_midi(0,0x90,48,127);run(30);assert(core_note_env2(0,0,48)==-1);assert(fabsf(core_note_cutoff(0,0,48)-50)<.01);assert(core_note_wave(0,0,48)<21);assert(fabsf(core_note_pitch(0,0,48)+600)<.01);
 c.env2.sustain=127;core_set(&c);run(30);assert(core_note_env2(0,0,48)==1);assert(fabsf(core_note_cutoff(0,0,48)-110)<.01);assert(core_note_wave(0,0,48)>79);
 c.env2_pitch=0;c.wave_env2_depth=0;c.filter_env2_depth=0;c.lfo.rate=83;c.lfo_pitch_cents=-1200;core_init();core_set(&c);core_midi(0,0x90,48,127);float lo=2000,hi=-2000;
 for(unsigned i=0;i<200;i++){run(1);float p=core_note_pitch(0,0,48);if(p<lo)lo=p;if(p>hi)hi=p;}assert(lo< -1100&&hi>1100);
 c.lfo.pitch=255;c.lfo_pitch_cents=32767;core_set(&c);for(unsigned i=0;i<50;i++){run(1);assert(fabsf(core_note_pitch(0,0,48))<=1200.01f);}c.lfo.pitch=0;
 for(unsigned i=0;i<8000;i++){c.env2.sustain=(int8_t)((int)(i%255)-127);c.env2_pitch=(int8_t)((int)(i%241)-120);c.lfo_pitch_cents=(int16_t)((int)(i%2401)-1200);c.filter_env2_depth=(int8_t)((int)(i%255)-127);c.wave_env2_depth=(int8_t)((int)(i%201)-100);c.filter.mode=(uint8_t)(i%4);c.unison_mode=(uint8_t)(i%3);core_set(&c);core_midi(i%3,(uint8_t)(0x90|i%16),(uint8_t)(24+i%60),100);run(1);}
 core_panic();run(1);for(unsigned i=0;i<512;i++)assert(!pcm[i]);puts("PASS: bipolar sustain, upward release to zero, live sustain polarity changes, signed wave/filter/pitch routes, full signed LFO pitch sweep, 8000 sanitized combined stress blocks and panic silence.");
}
