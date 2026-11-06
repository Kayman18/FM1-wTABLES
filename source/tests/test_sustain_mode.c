#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int32_t pcm[512],reference[48][512];
static core_settings settings(void){core_settings s={0};s.note=48;s.level=60;s.env=(pw_env_params){60,30,100,20};s.env2=(pw_env_params){50,20,64,20};s.filter.cutoff=96;s.wave_quality=1;s.velocity_amount=100;s.delay_time_ms=500;return s;}
static void render(unsigned blocks){while(blocks--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);}}
int main(void){
 assert(sizeof(core_settings)==76);
 // Synth response is sample-identical to keeping a note held through pedal/re-strikes.
 for(unsigned port=0;port<3;port++)for(unsigned unison=0;unison<3;unison++)for(unsigned chord=0;chord<2;chord++)for(unsigned pass=0;pass<2;pass++){
  core_init();core_settings c=settings();c.sustain_mode=1;c.unison_mode=unison;c.chord_shape=chord?6:0;core_set(&c);core_midi(port,0xb3,64,127);core_midi(port,0x93,48,100);
  for(unsigned b=0;b<48;b++){
   if(pass&&b==8)core_midi(port,0x83,48,0);
   if(pass&&b==16)core_midi(port,0x93,48,40); // Changed velocity does not change existing Synth sound.
   render(1);if(!pass)memcpy(reference[b],pcm,sizeof(pcm));else assert(!memcmp(reference[b],pcm,sizeof(pcm)));
  }
  core_midi(port,0xb3,64,0);assert(core_note_held(port,3,48));core_midi(port,0x83,48,0);render(200);assert(!core_voice_count());
 }
 // Three distinct behaviors on exactly the same pedal-held re-strike.
 for(unsigned mode=0;mode<3;mode++)for(unsigned port=0;port<3;port++)for(unsigned channel=0;channel<16;channel++){
  core_init();core_settings c=settings();c.sustain_mode=mode;c.chord_shape=7;core_set(&c);core_midi(port,0xb0|channel,64,127);core_midi(port,0x90|channel,48,100);render(20);core_midi(port,0x80|channel,48,0);render(4);
  float levels[4];uint32_t phases[4];for(unsigned i=0;i<4;i++){levels[i]=slots[i].main.env.level;phases[i]=slots[i].main.osc.phase;assert(slots[i].main.sustained);}
  core_midi(port,0x90|channel,48,70);
  for(unsigned i=0;i<4;i++){
   assert(slots[i].main.held&&!slots[i].main.sustained);
   if(mode==2){assert(slots[i].main.env.level==0);assert(slots[i].main.osc.phase==0);assert(slots[i].fade);assert(slots[i].main.velocity==70);}
   else{assert(slots[i].main.env.level==levels[i]);assert(slots[i].main.osc.phase==phases[i]);assert(slots[i].main.velocity==(mode==1?100:70));}
  }
  core_midi(port,0xb0|channel,64,0);assert(core_note_held(port,channel,48));core_midi(port,0xb0|channel,123,0);render(200);assert(!core_voice_count());
 }
 // Pedal/channel isolation and mode changes act at the next eligible re-strike.
 core_init();core_settings c=settings();c.sustain_mode=1;c.chord_shape=2;core_set(&c);core_midi(0,0xb0,64,127);core_midi(0,0x90,48,100);core_midi(2,0x91,48,100);render(20);core_midi(0,0x80,48,0);core_midi(2,0x81,48,0);assert(core_note_sustained(0,0,48));assert(!core_note_sustained(2,1,48));c.sustain_mode=2;core_set(&c);core_midi(0,0x90,48,50);assert(core_note_held(0,0,48));core_midi(0,0xb0,120,0);assert(!core_note_held(0,0,48));
 // Strum canceled on release stays canceled in Synth; Piano starts a new strum.
 for(unsigned mode=1;mode<=2;mode++){
  core_init();c=settings();c.chord_shape=6;c.break_time_ms=500;c.sustain_mode=mode;core_set(&c);core_midi(0,0xb0,64,127);core_midi(0,0x90,48,100);render(2);core_midi(0,0x80,48,0);assert(!core_pending_count());core_midi(0,0x90,48,90);assert(core_pending_count()==(mode==2?2:0));assert(core_note_layers(0,0,48)==(mode==2?3:1));
 }
 // Triggered voices never become pedal-sustained, so these modes do not alter them.
 for(unsigned mode=0;mode<3;mode++){
  core_init();c=settings();c.trigger_mode=1;c.sustain_mode=mode;core_set(&c);core_midi(0,0xb0,64,127);core_midi(0,0x90,48,100);render(10);core_midi(0,0x80,48,0);assert(!core_note_sustained(0,0,48));float before=core_env_level();core_midi(0,0x90,48,80);assert(core_env_level()==before);
 }
 for(unsigned i=0;i<4000;i++){
  c=settings();c.sustain_mode=i%3;c.chord_shape=i%12;c.break_time_ms=i%1001;c.trigger_mode=i%2;c.unison_mode=i%3;core_set(&c);unsigned p=i%3,ch=i%16,n=24+i%60;core_midi(p,0xb0|ch,64,127);core_midi(p,0x90|ch,n,100);render(1);core_midi(p,0x80|ch,n,0);core_midi(p,0x90|ch,n,60);render(1);if(i%7==0)core_midi(p,0xb0|ch,121,0);if(i%11==0)core_midi(p,0xb0|ch,120,0);assert(core_voice_count()<=4);
 }
 core_panic();render(1);assert(!core_voice_count()&&!core_pending_count());
 puts("PASS: sustain response: Synth held-note PCM equivalence across all ports/unison/chords, 144 mode/port/channel scenarios, fresh Piano attacks, pedal-up/release/CC120/123, mode switching, strum cancellation/restart, Triggered exclusion and 4000 sanitized stress blocks.");
}
