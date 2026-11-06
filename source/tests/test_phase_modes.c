#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
static int32_t pcm[512];
static void render(unsigned count){while(count--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++){assert(pcm[i]>=-8388608&&pcm[i]<=8388607);assert(pcm[i]==pcm[i^1]);}}}
static void on(unsigned note){core_midi(0,0x90,(uint8_t)note,127);}
static void off(unsigned note){core_midi(0,0x80,(uint8_t)note,0);}
int main(void){
 core_settings c={0,48,50,0,75,{0,0,127,60},{0,72,0,100},0,{60,70,64,50},0,{0,20,24,50},0,0,{0,0,0,0},PW_LFO_POLY,90};
 core_init();core_set(&c);render(1);
 unsigned notes[]={48,52,55,59};for(unsigned i=0;i<4;i++){on(notes[i]);assert(core_note_phase(0,0,notes[i])==pw_lfo_spread_phase(i,90));}
 render(50);for(unsigned i=0;i<4;i++)assert(core_note_phase(0,0,notes[i])==pw_lfo_spread_phase(i,90));
 c.lfo.rate=80;core_set(&c);render(30);uint32_t other=core_note_phase(0,0,48);on(52);assert(core_note_phase(0,0,52)==pw_lfo_spread_phase(1,90));assert(core_note_phase(0,0,48)==other);
 /* Mono resets the entire group only when no primary envelope is held. */
 c.lfo_phase_mode=PW_LFO_MONO;core_init();core_set(&c);render(20);on(48);for(unsigned i=0;i<4;i++)assert(core_voice_phase(i)==pw_lfo_spread_phase(i,90));
 render(20);uint32_t anchor=core_voice_phase(0);on(52);assert(core_note_phase(0,0,52)==anchor+pw_lfo_spread_phase(1,90));assert(core_voice_phase(0)==anchor);
 off(48);off(52);render(10);on(55);for(unsigned i=0;i<4;i++)assert(core_voice_phase(i)==pw_lfo_spread_phase(i,90));
 /* Retrigger Off keeps an idle group clock and joins it on later notes. */
 c.lfo_phase_mode=PW_LFO_OFF;core_init();core_set(&c);render(20);anchor=core_voice_phase(0);assert(anchor!=0);on(48);assert(core_voice_phase(0)==anchor);
 render(20);anchor=core_voice_phase(0);on(52);assert(core_note_phase(0,0,52)==anchor+pw_lfo_spread_phase(1,90));
 off(48);off(52);render(300);assert(core_voice_count()==0);anchor=core_voice_phase(0);render(10);assert(core_voice_phase(0)!=anchor);anchor=core_voice_phase(0);on(48);assert(core_note_phase(0,0,48)==anchor);
 /* Keep retains TEST10 matching-note phase behavior. */
 c.lfo_phase_mode=PW_LFO_KEEP;c.lfo_spread=0;core_init();core_set(&c);on(48);render(30);anchor=core_note_phase(0,0,48);on(48);assert(core_note_phase(0,0,48)==anchor);
 for(unsigned i=0;i<16000;i++){
  c.lfo_phase_mode=(uint8_t)(i%4);c.lfo_spread=(uint8_t)(i%91);c.lfo=(pw_lfo_params){(int8_t)((int)(i%201)-100),(uint8_t)(i%101),(uint8_t)(i%97),(uint8_t)(i%101)};c.filter.mode=(uint8_t)(i%4);c.filter.resonance=(uint8_t)(i%101);c.bank=(uint8_t)(i%8);core_set(&c);
  if(i%5)on(24+i%60);else off(24+i%60);render(1);assert(core_voice_count()<=4);
 }
 core_panic();render(1);assert(!core_voice_count());for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 puts("PASS: Poly reset/90-degree spread, held-note phase isolation, Mono first-note/release-only resets, Off idle-clock continuity, Keep retrigger compatibility, 16000 mode/spread/control/steal stress blocks and panic silence.");
}
