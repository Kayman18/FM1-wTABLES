#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
static int32_t pcm[512];
static core_settings settings(void){core_settings c={0};c.note=48;c.level=60;c.env=(pw_env_params){0,0,100,0};c.env2=(pw_env_params){0,0,64,0};c.wave_quality=1;c.filter.cutoff=96;c.velocity_amount=100;c.chord_shape=6;c.break_time_ms=100;c.delay_time_ms=500;return c;}
static void render(unsigned n){while(n){unsigned k=n>256?256:n;core_render(pcm,k,4096);n-=k;for(unsigned i=0;i<2*k;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);for(unsigned i=0;i<4;i++)if(slots[i].fade)assert(!slots[i].tail.wait_samples);}}
static unsigned slot_for(unsigned note){for(unsigned i=0;i<4;i++)if(live(&slots[i].main)&&slots[i].main.note==note)return i;assert(0);return 0;}
int main(void){
 assert(sizeof(core_settings)==76);core_settings c=settings();
 for(unsigned mode=0;mode<3;mode++)for(unsigned ms=1;ms<=1000;ms+=37){
  core_init();c=settings();c.break_direction=mode;c.break_time_ms=ms;core_set(&c);core_midi(0,0x90,48,100);
  unsigned down=mode!=0,first=slot_for(down?55:48),second=slot_for(52),third=slot_for(down?48:55);
  unsigned d=pw_break_delay(ms,1),d2=pw_break_delay(ms,2);
  assert(core_pending_count()==2);assert(!core_voice_delay(first));assert(core_voice_delay(second)==d);assert(core_voice_delay(third)==d2);
  render(d);assert(!slots[second].main.osc.phase);assert(slots[second].main.env.level==0.0f);assert(slots[third].main.env2.level==0.0f);
  render(1);assert(slots[second].main.osc.phase);render(d2-d-1);assert(!slots[third].main.osc.phase);render(1);assert(slots[third].main.osc.phase);
  core_midi(0,0x80,48,0);render(4000);assert(!core_voice_count());
 }
 // Alternating follows previous explicit direction; reset starts with Down.
 core_init();c=settings();c.break_direction=2;core_set(&c);
 for(unsigned i=0;i<8;i++){core_midi(0,0x90,48,100);assert(!core_voice_delay(slot_for(i%2?48:55)));core_midi(0,0x80,48,0);render(2000);}
 c.break_direction=1;core_set(&c);core_midi(0,0x90,48,100);c.break_direction=2;core_set(&c);core_midi(0,0x90,48,100);assert(!core_voice_delay(slot_for(48)));
 // Per-strike snapshots: time/direction/chord and panel octave changes cannot alter a queued run.
 core_init();c=settings();core_set(&c);core_midi(0,0x90,48,100);unsigned si=slot_for(52);render(100);c.break_time_ms=1;c.break_direction=1;c.chord_shape=11;c.chord_key=7;core_set(&c);assert(core_voice_delay(si)==4310);render(4310);assert(!slots[si].main.osc.phase);render(1);assert(slots[si].main.osc.phase);
 // Release cancels pending even with sustain or Triggered; already sounded notes retain normal behavior.
 for(unsigned triggered=0;triggered<2;triggered++)for(unsigned pedal=0;pedal<2;pedal++){
  core_init();c=settings();c.trigger_mode=triggered;c.env.attack=80;c.env.decay=80;core_set(&c);core_midi(0,0xb0,64,pedal?127:0);core_midi(0,0x90,48,100);render(50);core_midi(0,0x90,48,0);assert(!core_pending_count());assert(core_note_layers(0,0,48)==1);assert(!core_note_held(0,0,48));render(20000);assert(!core_pending_count());core_midi(0,0xb0,120,0);assert(!core_voice_count());
 }
 // Independent sources, channel all-notes/sound-off, stealing, and re-strike.
 core_init();c=settings();c.chord_shape=2;core_set(&c);core_midi(0,0x90,48,100);core_midi(2,0x91,48,100);assert(core_pending_count()==2);core_midi(0,0xb0,123,0);assert(core_pending_count()==1);assert(core_note_held(2,1,48));core_midi(2,0xb1,120,0);assert(!core_pending_count());
 core_init();c=settings();core_set(&c);core_midi(0,0x90,48,100);render(80);core_midi(0,0x90,50,100);assert(!core_note_held(0,0,48));assert(core_pending_count()==2);render(100);core_midi(0,0x90,50,100);assert(core_voice_delay(slot_for(53))==4410);core_panic();render(1);assert(!core_pending_count());assert(!core_voice_count());
 // Panel and PLAY routes share the timing/cancellation path.
 core_init();c=settings();core_set(&c);core_keys(1);render(1);assert(core_pending_count()==2);core_keys(0);render(1);assert(!core_pending_count());
 core_init();c=settings();c.playing=1;core_set(&c);render(1);assert(core_pending_count()==2);c.playing=0;core_set(&c);render(1);assert(!core_pending_count());
 for(unsigned i=0;i<3000;i++){
  c=settings();c.break_time_ms=i%1001;c.break_direction=i%3;c.chord_shape=i%12;c.chord_scale=i%9;c.chord_inversion=i%4;c.trigger_mode=i%2;c.unison_mode=i%3;core_set(&c);core_midi(i%3,0x90|(i%16),24+i%60,1+i%127);render(1+i%256);
  if(i%3==0)core_midi(i%3,0x80|(i%16),24+i%60,0);
  if(i%31==0)core_midi(i%3,0xb0|(i%16),123,0);
  assert(core_pending_count()<=3);
 }
 core_panic();render(1);assert(!core_pending_count());assert(!core_voice_count());
 puts("PASS: break engine: 84 exact sample-onset scenarios, frozen deferred envelopes/oscillators, direction alternation, settings snapshots, sustain/Triggered cancellation, source/channel isolation, CC120/123, retrigger/steal, panel/PLAY, panic and 3000 sanitized stress blocks.");
}
