#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
#include <math.h>
static int32_t pcm[512];
static core_settings settings(void){core_settings c={0};c.note=48;c.level=60;c.env=(pw_env_params){0,0,127,0};c.env2=(pw_env_params){0,0,64,0};c.wave_quality=1;c.filter.cutoff=96;c.velocity_amount=100;c.delay_time_ms=500;return c;}
static void render(unsigned n){while(n--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);assert(core_primary_count()<=voice_budget());for(unsigned i=0;i<4;i++)if(slots[i].fade)assert(!slots[i].tail.wait_samples);}}
int main(void){
 assert(sizeof(core_settings)==76);
 for(unsigned limit=1;limit<=4;limit++)for(unsigned unison=0;unison<3;unison++)for(unsigned shape=0;shape<12;shape++)for(unsigned strum=0;strum<2;strum++){
  core_init();core_settings c=settings();c.voice_limit=limit;c.unison_mode=unison;c.chord_shape=shape;c.break_time_ms=strum?200:0;core_set(&c);
  unsigned expected=1u<<unison;if(shape){uint8_t p[5];expected=pw_chord_notes(48,0,0,pw_chord_shape(shape),0,p);if(expected>limit)expected=limit;}else while(expected>limit)expected>>=1;
  core_midi(0,0x90,48,100);assert(core_primary_count()==expected);assert(core_note_layers(0,0,48)==expected);render(2);
  for(unsigned root=50;root<65;root++){core_midi(0,0x90,root,100);assert(core_primary_count()<=limit);render(1);}
  core_panic();render(1);assert(!core_primary_count()&&!core_pending_count());
 }
 // One-voice mode steals the old root; releasing it cannot stop the new note.
 core_init();core_settings c=settings();c.voice_limit=1;core_set(&c);core_midi(0,0x90,48,100);render(2);core_midi(0,0x90,60,100);assert(!core_note_held(0,0,48)&&core_note_held(0,0,60));core_midi(0,0x80,48,0);assert(core_note_held(0,0,60));
 // Limit reductions remove complete groups and queued strum notes permanently.
 core_init();c=settings();c.chord_shape=7;c.break_time_ms=1000;core_set(&c);core_midi(0,0x90,48,100);render(1);assert(core_pending_count()==3);c.voice_limit=2;core_set(&c);render(1);assert(!core_primary_count()&&!core_pending_count());c.voice_limit=4;core_set(&c);render(600);assert(!core_primary_count());
 // Reductions among independent notes retain the newest held root.
 core_init();c=settings();core_set(&c);for(unsigned i=0;i<4;i++)core_midi(0,0x90,48+i,100);c.voice_limit=1;core_set(&c);render(1);assert(core_note_held(0,0,51));assert(core_primary_count()==1);
 // Full source coarse range and 48-semitone bend, including extreme downward combinations.
 core_init();c=settings();core_set(&c);core_midi(0,0x90,48,127);render(3);uint32_t base=core_note_increment(0,0,48);
 c.coarse_tune=60;core_set(&c);render(3);assert(core_note_increment(0,0,48)==base*32u);
 c.coarse_tune=-60;core_set(&c);render(3);assert(core_note_increment(0,0,48)==base/32u);
 c.coarse_tune=0;c.bend_range_offset=46;core_set(&c);core_midi(0,0xe0,0,0);render(30);assert(fabsf(core_note_performance(0,0,48)+4800)<.02f);assert(core_note_increment(0,0,48)==base/16u);
 c.coarse_tune=-60;c.fine_tune=-100;c.env2_pitch=-120;c.env2.sustain=127;c.panel_octave=-2;core_set(&c);core_keys(1);render(30);assert(core_note_increment(254,0,53)>0);
 for(unsigned i=0;i<5000;i++){
  c=settings();c.voice_limit=1+i%4;c.chord_shape=i%12;c.break_time_ms=i%1001;c.break_direction=i%3;c.sustain_mode=i%3;c.unison_mode=i%3;c.trigger_mode=i%2;c.coarse_tune=(int)(i%121)-60;c.fine_tune=(int)(i%201)-100;c.bend_range_offset=(int)(i%49)-2;c.panel_octave=(int)(i%5)-2;c.env2_pitch=(int)(i%241)-120;c.lfo_pitch_cents=(int)(i%2401)-1200;c.filter.mode=i%4;c.filter.resonance=i%101;core_set(&c);
  unsigned p=i%3,ch=i%16,n=24+i%60;core_midi(p,0xb0|ch,64,127);core_midi(p,0xe0|ch,i&127,(i>>7)&127);core_midi(p,0x90|ch,n,100);render(1);core_midi(p,0x80|ch,n,0);if(i%5==0)core_midi(p,0x90|ch,n,60);if(i%11==0)core_midi(p,0xb0|ch,120,0);
 }
 core_panic();render(1);assert(!core_primary_count()&&!core_pending_count());
 puts("PASS: voice budget: 288 limit/unison/chord/strum combinations, last-note stealing, old note-off isolation, live limit reduction, no queued-note resurrection, full coarse/bend endpoints and 5000 sanitized extreme-pitch/allocator/control stress blocks.");
}
