#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
#include "../fm1/app/pw_performance.h"
static int32_t out[512];
static core_settings c;
static void render(unsigned n){while(n--){core_render(out,256,4096);for(unsigned i=0;i<512;i++){assert(out[i]>=-8388608&&out[i]<=8388607);assert(out[i]==out[i^1]);}}}
static void boot(void){c=(core_settings){0,48,0,0,75,{0,0,127,0},{0,96,0,100},0,{0,0,127,0}};core_init();core_set(&c);}
static void msg(unsigned p,unsigned ch,unsigned command,unsigned a,unsigned b){core_midi(p,(uint8_t)(command|ch),(uint8_t)a,(uint8_t)b);}
static void bend(unsigned p,unsigned ch,int v){unsigned raw=(unsigned)(v+8192);msg(p,ch,0xe0,raw&127,raw>>7);}
int main(void){
 boot();msg(0,0,0x90,48,127);msg(2,0,0x90,55,127);render(3);uint32_t root=core_note_increment(0,0,48),peer=core_note_increment(2,0,55);
 bend(0,0,8191);render(20);float b=core_note_performance(0,0,48);assert(b>199&&b<201);assert(core_note_increment(0,0,48)>root);assert(core_note_increment(2,0,55)==peer);
 bend(0,1,-8192);render(4);assert(core_note_increment(0,0,48)>root);bend(0,0,0);render(20);assert(core_note_increment(0,0,48)==root);
 msg(0,0,0xb0,64,127);msg(0,0,0x80,48,0);render(10);assert(!core_note_held(0,0,48)&&core_note_sustained(0,0,48));assert(core_sustain(0,0)&&core_note_env2(0,0,48)==1);
 msg(2,0,0x80,55,0);render(10);assert(core_note_env2(2,0,55)<0&&core_note_sustained(0,0,48));
 msg(0,1,0xb0,64,0);render(2);assert(core_note_sustained(0,0,48));
 msg(0,0,0x90,48,90);render(2);assert(core_note_held(0,0,48)&&!core_note_sustained(0,0,48));
 msg(0,0,0x90,48,0);render(2);assert(core_note_sustained(0,0,48));msg(0,0,0xb0,64,0);render(10);assert(!core_voice_count());
 // CC123 respects pedal; CC121 releases latched notes and centers bend.
 msg(0,0,0xb0,64,127);msg(0,0,0x90,48,127);msg(0,0,0xb0,123,0);render(2);assert(core_note_sustained(0,0,48));msg(0,0,0xb0,121,0);render(10);assert(!core_voice_count()&&!core_sustain(0,0));
 // CC120 kills sound despite pedal, without changing its controller state.
 msg(2,3,0xb0,64,127);msg(2,3,0x90,60,127);msg(2,3,0x80,60,0);render(2);msg(2,3,0xb0,120,0);render(1);assert(!core_voice_count());
 // Pedal-held notes are stolen before physically held peers.
 boot();msg(0,0,0x90,48,127);msg(0,0,0xb0,64,127);
 for(unsigned n=50;n<=54;n+=2){msg(0,0,0x90,n,127);msg(0,0,0x80,n,0);}render(3);
 msg(0,0,0x90,55,127);render(3);assert(core_note_held(0,0,48)&&core_note_held(0,0,55));assert(!core_note_sustained(0,0,50));
 // Key octave is captured at note-on; changing it never breaks key release.
 boot();core_keys(1u<<7);render(3);root=core_note_increment(254,0,60);c.panel_octave=1;core_set(&c);render(3);assert(core_note_increment(254,0,60)==root);core_keys(0);render(10);assert(!core_voice_count());core_keys(1u<<7);render(3);assert(core_note_increment(254,0,60)==root*2);core_keys(0);render(10);assert(!core_voice_count());
 // Tune shifts held notes and combines with bend and independent Env2/LFO.
 boot();msg(0,0,0x90,48,127);render(3);root=core_note_increment(0,0,48);c.coarse_tune=12;c.fine_tune=0;core_set(&c);render(3);assert(core_note_increment(0,0,48)==root*2);
 c.bend_range_offset=10;c.fine_tune=50;c.env2_pitch=60;core_set(&c);bend(0,0,-8192);render(30);assert(fabsf(core_note_performance(0,0,48)-50)<.01);assert(fabsf(core_note_pitch(0,0,48)-600)<.01);
 for(unsigned i=0;i<24000;i++){
  c.coarse_tune=(int8_t)((int)(i%49)-24);c.fine_tune=(int8_t)((int)(i%201)-100);c.bend_range_offset=(int8_t)((int)(i%25)-2);c.panel_octave=(int8_t)((int)(i%5)-2);c.env2_pitch=(int8_t)((int)(i%241)-120);c.lfo=(pw_lfo_params){(int8_t)((int)(i%201)-100),50,48,100};c.filter.mode=(uint8_t)(i%4);c.filter.resonance=(uint8_t)(i%101);c.env2_attack_shape=(int8_t)((int)(i%128)-64);core_set(&c);
  unsigned p=i%3,ch=i%16,n=24+i%60;msg(p,ch,0x90,n,127);bend(p,ch,(int)(i%16384)-8192);msg(p,ch,0xb0,64,i%2?127:0);if(i%3==0)msg(p,ch,0x80,n,0);if(i%19==0)msg(p,ch,0xb0,121,0);if(i%29==0)msg(p,ch,0xb0,120,0);render(1);assert(core_voice_count()<=4);
  uint32_t inc=core_note_increment(p,ch,n);assert(inc<=2147418112u);
 }
 core_panic();render(1);assert(!core_voice_count());for(unsigned p=0;p<3;p++)for(unsigned ch=0;ch<16;ch++)assert(!core_sustain(p,ch));for(unsigned i=0;i<512;i++)assert(!out[i]);
 puts("PASS: bend smoothing/return, USB/TRS/channel isolation, pedal hold/release/retrigger/velocity-zero, CC120/121/123, captured key octaves, held-note tuning and combined pitch routes; 24000 sanitized performance/modulation/steal stress blocks; panic clears sound and controllers.");
}
