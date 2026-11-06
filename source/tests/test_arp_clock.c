#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
static int32_t pcm[512];
static core_settings config(unsigned source){core_settings c={0};c.note=48;c.level=60;c.env=(pw_env_params){0,0,127,0};c.env2=(pw_env_params){0,0,64,0};c.filter.cutoff=96;c.wave_quality=1;c.delay_time_ms=500;c.arp_mode=1;c.arp_bpm=120;c.arp_octaves=1;c.arp_gate_percent=50;c.arp_clock=source;return c;}
static void frames(unsigned n){while(n){unsigned m=n>256?256:n;core_render(pcm,m,4096);n-=m;assert(core_primary_count()<=4&&arp_count<=16);}}
static void msg(unsigned p,unsigned s){core_midi(p,s,0,0);}
static void notes(void){core_midi(0,0x90,55,100);core_midi(2,0x92,48,95);core_midi(0,0x90,52,90);}
static void tick(unsigned p,unsigned n){for(unsigned i=0;i<n;i++){msg(p,0xf8);frames(450+i%700);}}
int main(void){
 assert(sizeof(core_settings)==96&&offsetof(core_settings,arp_clock)==75);
 for(unsigned source=1;source<=2;source++){
  unsigned port=source==1?0:2,other=source==1?2:0;
  core_init();core_settings c=config(source);core_set(&c);notes();frames(22050);assert(!arp_events&&!arp_gate);
  msg(other,0xfa);tick(other,24);assert(!arp_clock_running&&!arp_events);
  msg(port,0xfa);assert(arp_clock_running&&!arp_events);msg(port,0xf8);assert(arp_events==1&&arp_pitch==48&&arp_port==2&&arp_channel==2);
  tick(port,5);assert(arp_gate);msg(port,0xf8);assert(!arp_gate);tick(port,5);assert(arp_events==1);msg(port,0xf8);assert(arp_events==2&&arp_pitch==52);
  // Stop releases even latched/triggered inputs; clocks while stopped never advance.
  c.sustain_hold=1;c.trigger_mode=1;core_set(&c);msg(port,0xfc);frames(22050);assert(!arp_clock_running&&!arp_gate&&!core_primary_count()&&arp_count==3);
  unsigned previous=arp_events,phase=arp_clock_phase;tick(port,24);assert(arp_events==previous&&arp_clock_phase==phase);
  msg(port,0xfb);tick(port,11);assert(arp_events==previous);msg(port,0xf8);assert(arp_events==previous+1&&arp_pitch==55);
  // Start resets sequence; arrival alone never emits a note.
  msg(port,0xfa);assert(arp_events==previous+1&&!arp_gate);msg(port,0xf8);assert(arp_pitch==48&&arp_gate);
  frames(44099);assert(arp_clock_running);frames(1);assert(!arp_clock_running&&!arp_gate);frames(22050);assert(!core_primary_count());
  tick(port,12);assert(!arp_clock_running);msg(port,0xfb);tick(port,12);assert(arp_clock_running);
  core_panic();frames(1);assert(!arp_clock_running&&!arp_count&&!core_primary_count());tick(port,24);assert(!arp_count&&!core_primary_count());
 }
 // Gate rounding is stable, captured at each step; exact 50% is six MIDI ticks.
 for(unsigned percent=10;percent<=90;percent++){
  core_init();core_settings c=config(1);c.arp_gate_percent=percent;core_set(&c);notes();msg(0,0xfa);msg(0,0xf8);
  unsigned gate=(percent*12+99)/100;
  for(unsigned i=1;i<12;i++){msg(0,0xf8);assert(!!arp_gate==(i<gate));frames(10);}
 }
 // All fourteen pattern types advance once per twelve ticks, irrespective of local BPM.
 for(unsigned mode=1;mode<=14;mode++)for(unsigned oct=1;oct<=3;oct++){
  core_init();core_settings c=config(2);c.arp_mode=mode;c.arp_octaves=oct;c.arp_bpm=40;core_set(&c);notes();msg(2,0xfa);
  uint8_t step=255;pw_arp_random random={0,{0,0},255};unsigned pitches[]={48,52,55},played[]={55,48,52};
  for(unsigned k=0;k<20;k++){
   unsigned idx=mode>=11?pw_arp_octave_next(&step,3,mode,oct,0):mode>=8?pw_arp_random_next(&random,3,mode,oct):pw_arp_next(&step,3,mode,oct);
   msg(2,0xf8);assert(arp_events==k+1&&arp_pitch==(mode==7?played:pitches)[idx%3]+12*(idx/3));tick(2,11);
   c.arp_bpm=k%2?40:240;core_set(&c);
  }
 }
 // Source changes release voices and require fresh transport, but retain input roots.
 core_init();core_settings c=config(1);core_set(&c);notes();msg(0,0xfa);msg(0,0xf8);c.arp_clock=2;core_set(&c);frames(1);assert(!arp_clock_running&&!arp_gate&&arp_count==3);
 tick(0,24);assert(!arp_clock_running);msg(2,0xfa);msg(2,0xf8);assert(arp_clock_running&&arp_gate);
 c.arp_clock=0;core_set(&c);frames(1);assert(arp_gate&&!arp_clock_running);unsigned events=arp_events;msg(0,0xfc);msg(2,0xfc);frames(11025);assert(arp_events==events+1);
 // Removing notes while stopped, channel stops, and note arrival on the next clock grid.
 core_init();c=config(1);core_set(&c);msg(0,0xfa);tick(0,3);core_midi(0,0x90,48,100);assert(!arp_events);tick(0,9);assert(!arp_events);msg(0,0xf8);assert(arp_events==1);
 msg(0,0xfc);core_midi(0,0x80,48,0);assert(!arp_count);msg(0,0xfb);tick(0,24);assert(!arp_count&&!arp_gate);
 c.sustain_hold=1;core_set(&c);core_midi(2,0x93,48,100);core_midi(2,0x83,48,0);tick(0,12);assert(arp_count==1);core_midi(2,0xb3,123,0);assert(!arp_count);tick(0,24);assert(!arp_gate);
 // Bounded stress mixes MIDI ports, transports, patterns, source changes and controllers.
 for(unsigned i=0;i<5000;i++){
  c=config(i%3);c.arp_mode=i%15;c.sustain_hold=i%2;c.arp_octaves=1+i%3;c.chord_shape=i%12;c.unison_mode=i%3;c.voice_limit=1+i%4;core_set(&c);
  unsigned port=i%3;core_midi(port,0x90|(i%16),24+i%60,100);msg(port,(unsigned[]){0xfa,0xf8,0xfc,0xfb,0xf8}[i%5]);frames(256);core_midi(port,0x80|(i%16),24+i%60,0);if(i%17==0)core_panic();
 }
 core_panic();frames(22050);assert(!arp_count&&!core_primary_count()&&!arp_clock_running);
 puts("PASS: external clock: USB/TRS isolation, 12-tick eighth notes, all 81 gate settings, all 14 patterns/3 octaves, Start/Stop/Continue, timeout, panic, Hold/controller stops, clock-source changes and 5000 sanitized stress blocks.");
}
