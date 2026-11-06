#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stddef.h>
static int32_t pcm[512];
static core_settings cfg(void){core_settings c={0};c.note=48;c.level=60;c.env=(pw_env_params){0,0,127,0};c.env2=(pw_env_params){0,0,64,0};c.filter.cutoff=96;c.wave_quality=1;c.voice_limit=8;c.delay_time_ms=100;c.delay_return_db=-3;return c;}
static void frames(unsigned n){while(n){unsigned m=n>256?256:n;core_render(pcm,m,4096);n-=m;assert(core_primary_count()<=voice_budget());for(unsigned i=0;i<m*2;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);}}
static unsigned owned(unsigned root){unsigned n=0;for(unsigned i=0;i<8;i++)if(live(&slots[i].main)&&slots[i].main.root==root)n++;return n;}
int main(void){
 assert(sizeof(core_settings)==96&&offsetof(core_settings,unison_count)==79);
 // Every requested count and limit: clamp new integer-count controls, keep complete groups.
 for(unsigned budget=1;budget<=8;budget++)for(unsigned count=1;count<=8;count++){
  core_init();core_settings c=cfg();c.voice_limit=budget;c.unison_count=count;c.unison_detune=100;core_set(&c);
  unsigned effective=count<budget?count:budget;assert(core_unison_effective(&c)==effective);
  for(unsigned root=36;root<48;root++){
   core_midi(0,0x90,root,100);assert(owned(root)==effective);unsigned members=0;
   for(unsigned i=0;i<8;i++){poly_voice *v=&slots[i].main;if(live(v)&&v->root==root){assert(v->layers==effective&&v->geometry==(budget>4?8:4));assert(v->detune_target==pw_unison_detune(effective,v->member,100));members|=1u<<v->member;}}
   assert(members==(1u<<effective)-1);frames(256);
  }
  core_midi(0,0xb0,64,127);core_midi(0,0x80,47,0);assert(core_note_sustained(0,0,47));core_midi(0,0xb0,64,0);frames(16000);assert(!owned(47));
 }
 // Eight independent notes and bounded oldest stealing; foreign note-off cannot kill new root.
 core_init();core_settings c=cfg();core_set(&c);for(unsigned n=48;n<56;n++)core_midi(0,0x90,n,127);assert(core_primary_count()==8);
 core_midi(0,0x90,60,127);assert(core_primary_count()==8&&!owned(48)&&owned(60));core_midi(0,0x80,48,0);assert(owned(60));frames(4000);assert(voice_headroom==.5f);
 // Live reduction retires groups outside the four-slot layout; no resurrection on expansion.
 c.voice_limit=4;core_set(&c);frames(512);assert(core_primary_count()<=4&&voice_headroom==1);
 for(unsigned i=4;i<8;i++)assert(!live(&slots[i].main)&&!slots[i].fade);
 unsigned before=core_primary_count();c.voice_limit=8;core_set(&c);frames(512);assert(core_primary_count()==before);
 // Captured geometry keeps existing note panning/timbre stable across capacity edits.
 core_init();c=cfg();c.voice_limit=4;c.unison_count=2;c.stereo_mode=1;c.stereo_spread=100;c.unison_wave_spread=100;core_set(&c);core_midi(0,0x90,48,100);assert(slots[0].main.geometry==4&&slots[3].main.geometry==4);
 c.voice_limit=8;core_set(&c);frames(512);assert(slots[3].main.geometry==4&&fabsf(slots[3].main.pan_right-1)<1e-6f);
 // Five-note direct chords and all strum directions retain every generated source member.
 unsigned cases=0;
 for(unsigned shape=12;shape<=14;shape++)for(unsigned scale=0;scale<9;scale++)for(unsigned inv=0;inv<4;inv++)for(unsigned budget=5;budget<=8;budget++){
  core_init();c=cfg();c.voice_limit=budget;c.chord_shape=shape;c.chord_scale=scale;c.chord_inversion=inv;c.unison_count=8;core_set(&c);core_midi(0,0x90,36,100);
  uint8_t expected[5];unsigned n=pw_chord_notes(36,0,scale,pw_chord_shape(shape),inv,expected);assert(n==5&&core_primary_count()==5);
  for(unsigned j=0;j<5;j++){assert(slots[j].main.note==expected[j]&&slots[j].main.layers==1);}
  core_midi(0,0x80,36,0);frames(16000);assert(!core_primary_count());cases++;
 }
 for(unsigned direction=0;direction<3;direction++){
  core_init();c=cfg();c.chord_shape=14;c.break_time_ms=100;c.break_direction=direction;core_set(&c);core_midi(2,0x93,48,100);assert(core_pending_count()==4);
  for(unsigned i=0;i<5;i++)assert(slots[i].main.wait_samples==i*4410);
  frames(4411);assert(core_pending_count()==3);core_midi(2,0x83,48,0);assert(!core_pending_count());frames(16000);assert(!core_primary_count());
 }
 // All clock sources can use an eight-layer step, retaining stop/ownership semantics.
 for(unsigned clock=0;clock<3;clock++){
  core_init();c=cfg();c.unison_count=8;c.arp_mode=1;c.arp_clock=clock;c.arp_bpm=120;c.arp_octaves=1;c.arp_gate_percent=50;core_set(&c);core_midi(2,0x92,48,100);
  if(clock){unsigned port=clock==1?0:2;core_midi(port,0xfa,0,0);core_midi(port,0xf8,0,0);}frames(1);assert(core_primary_count()==8);
  if(clock)core_midi(clock==1?0:2,0xfc,0,0);else core_midi(2,0x82,48,0);frames(16000);assert(!core_primary_count());
 }
 // Aligned maximum voices stay within the previous full-scale range after capacity ramp.
 for(unsigned chorus_mode=0;chorus_mode<=2;chorus_mode++){
  core_init();c=cfg();c.level=100;c.unison_count=8;c.chorus_mode=chorus_mode;c.chorus_mix=50;core_set(&c);frames(512);core_midi(0,0x90,48,127);
  for(unsigned j=0;j<300;j++){frames(256);for(unsigned k=0;k<512;k++)assert(pcm[k]!=8388607&&pcm[k]!=-8388608);}
 }
 // Expanding an already sounding coherent stack cannot briefly double its level.
 core_init();c=cfg();c.voice_limit=4;c.unison_count=4;c.level=100;core_set(&c);core_midi(0,0x90,48,127);frames(6000);
 c.voice_limit=8;c.unison_count=8;core_set(&c);core_midi(0,0x90,48,127);
 for(unsigned j=0;j<1024;j++){frames(1);assert(pcm[0]!=8388607&&pcm[0]!=-8388608);}
 // Mixed voice counts, allocation sources, sustained/chord/arp notes, effects and capacity edits.
 core_init();for(unsigned i=0;i<6000;i++){
  c=cfg();c.voice_limit=1+i%8;c.unison_count=1+(i/7)%8;c.chord_shape=(i/11)%15;c.break_time_ms=(i/19)%1001;c.break_direction=i%3;c.filter.mode=i%4;c.filter.resonance=i%101;c.filter.mix=100;c.chorus_mode=(i/23)%3;c.chorus_mix=i%101;c.delay_style=i%2;c.delay_mode=(i/29)%3;c.delay_feedback=i%91;c.delay_mix=35;c.stereo_mode=i%4;c.stereo_spread=(int)(i%201)-100;c.unison_wave_spread=(int)(i%201)-100;c.sustain_hold=i%2;c.unison_detune=i%101;core_set(&c);
  unsigned port=i%3,ch=i%16,note=24+i%60;core_midi(port,0x90|ch,note,127);frames(256);core_midi(port,0x80|ch,note,0);if(i%31==0)core_midi(port,0xb0|ch,123,0);
 }
 c.playing=0;core_set(&c);core_panic();frames(1);assert(!core_primary_count());for(unsigned i=0;i<2;i++)assert(!pcm[i]);
 printf("PASS: TEST37 polyphony: all 64 unison-count/budget pairs, eight-note allocation/stealing, capacity reductions and captured geometry; %u complete five-note chord cases, five-note strums, eight-layer clocked arpeggios, full-level chorus headroom and 6000 sanitized stress blocks.\n",cases);
}
