#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
#include <math.h>
static int32_t pcm[512];
static core_settings cfg(void){core_settings c={0};c.note=48;c.level=60;c.env=(pw_env_params){0,0,127,0};c.env2=(pw_env_params){0,0,64,0};c.filter.cutoff=96;c.wave_quality=1;c.delay_time_ms=500;c.arp_mode=1;c.arp_bpm=120;c.arp_octaves=1;c.arp_gate_percent=50;return c;}
static void frames(unsigned n){while(n){unsigned m=n>256?256:n;core_render(pcm,m,4096);n-=m;assert(core_primary_count()<=voice_budget());assert(arp_count<=16);for(unsigned k=0;k<2*m;k++)assert(pcm[k]>=-8388608&&pcm[k]<=8388607);}}
static void on(unsigned n){core_midi(0,0x90,n,100);}
static void off(unsigned n){core_midi(0,0x80,n,0);}
int main(void){
 assert(sizeof(core_settings)==76);
 // Exact first-onset, step and gate boundary, with sorted held input.
 core_init();core_settings c=cfg();core_set(&c);on(55);on(48);on(52);frames(1);assert(arp_pitch==48&&arp_events==1&&arp_gate);
 frames(5511);assert(arp_gate);frames(1);assert(!arp_gate);frames(5512);assert(arp_events==1);frames(1);assert(arp_pitch==52&&arp_events==2);
 frames(11025);assert(arp_pitch==55&&arp_events==3);frames(11025);assert(arp_pitch==48&&arp_events==4);
 off(48);off(52);off(55);assert(!arp_count);frames(22050);assert(!core_primary_count());unsigned events=arp_events;frames(22050);assert(arp_events==events);
 // Rational clock has no cumulative rounding drift at every allowed BPM.
 for(unsigned bpm=40;bpm<=240;bpm++){uint32_t rem=0;uint64_t total=0;for(unsigned k=1;k<=1000;k++){total+=pw_arp_interval(bpm,&rem);assert(total==(uint64_t)k*2646000/(bpm*2));}}
 // All patterns/octaves and one-note special case; octave notes outside range are rests.
 for(unsigned mode=1;mode<=7;mode++)for(unsigned oct=1;oct<=3;oct++){
  core_init();c=cfg();c.arp_mode=mode;c.arp_octaves=oct;core_set(&c);on(48);on(52);on(55);uint8_t step=255;
  for(unsigned k=0;k<20;k++){unsigned idx=pw_arp_next(&step,3,mode,oct),notes[]={48,52,55};frames(k?11025:1);assert(arp_pitch==notes[idx%3]+12*(idx/3));}
 }
 core_init();c=cfg();c.arp_octaves=3;core_set(&c);on(83);frames(1);assert(arp_events==1);frames(11025*2);assert(arp_events==1&&!arp_gate);frames(11025);assert(arp_events==2);
 // Explicit new-pattern order independent of helper, including unsorted As Played.
 const unsigned expected[3][8]={{48,52,48,55,48,52,48,55},{55,52,55,48,55,52,55,48},{55,48,52,55,48,52,55,48}};
 for(unsigned mode=5;mode<=7;mode++){
  core_init();c=cfg();c.arp_mode=mode;core_set(&c);on(55);on(48);on(52);on(55);assert(arp_count==3);
  for(unsigned k=0;k<8;k++){frames(k?11025:1);assert(arp_pitch==expected[mode-5][k]);}
  off(55);off(48);off(52);frames(22050);assert(!arp_count&&!core_primary_count());
 }
 // Random family: bounded pitches, no-repeat and full-deck shuffle over two octaves.
 for(unsigned mode=8;mode<=10;mode++){
  core_init();c=cfg();c.arp_mode=mode;c.arp_octaves=2;core_set(&c);on(48);on(52);on(55);
  unsigned previous=255,seen=0;
  for(unsigned k=0;k<60;k++){
   frames(k?11025:1);unsigned n=arp_pitch,idx=n==48?0:n==52?1:n==55?2:n==60?3:n==64?4:n==67?5:99;assert(idx<6);
   if(mode==9)assert(idx!=previous);
   if(mode==10){assert(!(seen&(1u<<idx)));seen|=1u<<idx;if(k%6==5){assert(seen==63);seen=0;}}
   previous=idx;
  }
  off(48);off(52);off(55);frames(22050);assert(!arp_count&&!core_primary_count());
  on(48);frames(1);assert(arp_events&&arp_gate);core_panic();frames(22050);assert(!arp_count&&!core_primary_count());
 }
 // Source -8ve removes the repeated octave-root at the join of flagged chords.
 const unsigned omitted[4][14]={
 {48,52,55,60,64,67,72,48,52,55,60,64,67,72},
 {72,67,64,60,55,52,48,72,67,64,60,55,52,48},
 {48,52,55,60,64,67,72,67,64,60,55,52,48,52},
 {48,52,55,60,64,67,72,72,67,64,60,55,52,48}};
 for(unsigned mode=11;mode<=14;mode++){
  core_init();c=cfg();c.chord_shape=7;c.arp_mode=mode;c.arp_octaves=2;core_set(&c);on(48);
  for(unsigned k=0;k<14;k++){frames(k?11025:1);assert(arp_pitch==omitted[mode-11][k]);}
  off(48);frames(22050);assert(!arp_count&&!core_primary_count());
 }
 // All supported flagged shapes and inversions, original helper selection, channel ownership.
 for(unsigned shape=5;shape<=9;shape+=2)for(unsigned inv=0;inv<4;inv++)for(unsigned mode=11;mode<=14;mode++){
  core_init();c=cfg();c.chord_shape=shape;c.chord_inversion=inv;c.arp_mode=mode;c.arp_octaves=3;core_set(&c);
  core_midi(2,0x93,24,96);uint8_t notes[5],step=255;unsigned n=pw_chord_notes(24,0,0,pw_chord_shape(shape),inv,notes);
  for(unsigned k=0;k<12;k++){unsigned idx=pw_arp_octave_next(&step,n,mode,3,1);frames(k?11025:1);assert(arp_pitch==notes[idx%n]+12*(idx/n)&&arp_port==2&&arp_channel==3);}
  core_midi(2,0xb3,123,0);frames(22050);assert(!arp_count&&!core_primary_count());
 }
 // Clipping a flagged chord down to one candidate must not divide by zero.
 for(unsigned mode=11;mode<=14;mode++){
  core_init();c=cfg();c.chord_shape=7;c.arp_mode=mode;c.arp_octaves=3;core_set(&c);on(83);frames(100000);assert(arp_count==1);
  core_panic();frames(22050);assert(!arp_count&&!core_primary_count());
 }
 // Generated chord members feed steps; unison stays bounded by voice budget.
 core_init();c=cfg();c.chord_shape=6;c.break_time_ms=1000;c.unison_mode=2;c.voice_limit=2;core_set(&c);on(48);frames(1);assert(arp_pitch==48&&core_primary_count()==2&&!core_pending_count());frames(11025);assert(arp_pitch==52);frames(11025);assert(arp_pitch==55);
 // Hold/pedal latch the input, never the individual step gate. Port/channel ownership.
 core_init();c=cfg();c.sustain_hold=1;core_set(&c);on(48);frames(1);off(48);frames(6000);assert(arp_count==1&&!arp_gate);c.sustain_hold=0;core_set(&c);frames(5000);assert(!arp_count&&!core_primary_count());
 core_init();c=cfg();core_set(&c);core_midi(1,0xb2,64,127);core_midi(1,0x92,48,100);core_midi(2,0x93,55,80);frames(1);core_midi(1,0x82,48,0);core_midi(2,0x83,55,0);assert(arp_count==1);core_midi(1,0xb2,121,0);frames(5000);assert(!arp_count&&!core_primary_count());
 // Repeated note-ons update, overflow remains bounded, source-scoped stops work even with Hold.
 core_init();c=cfg();c.sustain_hold=1;core_set(&c);for(unsigned i=0;i<60;i++)on(24+i);assert(arp_count==16);on(83);assert(arp_count==16);core_midi(2,0x91,48,70);frames(1);core_midi(0,0xb0,123,0);assert(arp_count==1&&arp_roots[0].port==2);core_midi(2,0xb1,120,0);assert(!arp_count);
 // Off transition stops one-shots as well; re-enable does not resurrect MIDI roots.
 core_init();c=cfg();c.trigger_mode=1;c.env.release=100;core_set(&c);on(48);frames(11026);c.arp_mode=0;core_set(&c);frames(1);assert(!arp_count);for(unsigned i=0;i<4;i++)assert(!slots[i].main.held);c.arp_mode=2;core_set(&c);frames(11026);assert(!arp_count);
 // Panel and PLAY share the lane; PLAY stop works, panic never restarts held keys.
 core_init();c=cfg();core_set(&c);core_keys(1);frames(1);assert(arp_count==1&&arp_pitch==53);core_keys(0);frames(5000);assert(!arp_count);
 c.playing=1;core_set(&c);frames(1);assert(arp_count==1);c.playing=0;core_set(&c);frames(5000);assert(!arp_count);
 c.playing=1;core_set(&c);frames(1);assert(arp_count);c.arp_mode=0;core_set(&c);frames(5000);assert(!arp_count&&!core_primary_count()&&!st.playing);
 for(unsigned i=0;i<4000;i++){
  c=cfg();c.arp_mode=i%15;c.arp_bpm=40+i%201;c.arp_octaves=1+i%3;c.arp_gate_percent=10+i%81;c.sustain_hold=i%2;c.unison_mode=i%3;c.voice_limit=1+i%4;c.chord_shape=i%12;c.chord_scale=i%9;c.trigger_mode=i%2;c.filter.mode=i%4;c.filter.resonance=i%101;core_set(&c);
  unsigned p=i%3,ch=i%16,n=24+i%60;core_midi(p,0xb0|ch,64,i%5?0:127);core_midi(p,0x90|ch,n,100);frames(256);core_midi(p,0x80|ch,n,0);if(i%17==0)core_midi(p,0xb0|ch,123,0);
 }
 core_panic();frames(1);assert(!arp_count&&!core_primary_count());frames(22050);assert(!arp_count&&!core_primary_count());
 puts("PASS: arpeggiator engine: exact onset/step/gate samples, 201 BPM rational-clock sweeps, all 14 patterns and 3 octave ranges, boundary rests, chord/unison integration, Hold/pedals, channel stops, pool overflow, panel/PLAY/panic and 4000 sanitized stress blocks.");
}
