#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
static int32_t pcm[512];
static core_settings config(void){core_settings c={0};c.note=48;c.level=50;c.env=(pw_env_params){0,0,127,0};c.env2=(pw_env_params){0,0,127,0};c.filter.cutoff=96;c.wave_quality=1;c.voice_limit=8;c.delay_time_ms=100;return c;}
static void frames(unsigned n){while(n){unsigned m=n>256?256:n;core_render(pcm,m,4096);n-=m;assert(core_primary_count()<=voice_budget());for(unsigned k=0;k<2*m;k++)assert(pcm[k]>=-8388608&&pcm[k]<=8388607);}}
int main(void){
 assert(sizeof(core_settings)==96);
 for(unsigned note=0;note<128;note++){
  double ideal=440.0*pow(2.0,((double)note-69)/12)*4294967296.0/44100.0;assert(fabs((double)INC[note]-ideal)<=.5);
  for(unsigned port=0;port<3;port++)for(unsigned quality=0;quality<2;quality++){
   core_init();core_settings c=config();c.wave_quality=quality;core_set(&c);core_midi(port,0x95,note,127);frames(2000);
   assert(core_primary_count()==1&&core_note_held(port,5,note)&&core_note_increment(port,5,note)==INC[note]);
   if(quality)assert(core_note_mip(port,5,note)>=1);
   int sound=0;for(unsigned i=0;i<512;i++)sound|=pcm[i]!=0;assert(sound);
   core_midi(port,0xb5,64,127);core_midi(port,0x85,note,0);assert(core_note_sustained(port,5,note));
   core_midi((port+1)%3,0x85,note,0);assert(core_note_sustained(port,5,note));core_midi(port,0xb5,64,0);frames(16000);assert(!core_primary_count());
  }
 }
 // PLAY endpoints and note-zero ownership are independent of the idle display value.
 for(unsigned note=0;note<=127;note+=127){core_init();core_settings c=config();c.note=note;c.playing=1;core_set(&c);frames(1000);assert(core_primary_count()==1&&core_active_note()==note);c.playing=0;core_set(&c);frames(16000);assert(!core_primary_count());}
 // Compare below-tonic chords with the octave-shifted (source-valid) domain.
 unsigned low_cases=0;
 for(unsigned note=0;note<12;note++)for(int key=-12;key<=12;key++)if(note<(unsigned)((key+60)%12))for(unsigned scale=0;scale<9;scale++)for(unsigned shape=0;shape<14;shape++)for(unsigned inv=0;inv<4;inv++){
  uint8_t low[5],high[5],expected[5];unsigned n=pw_chord_notes(note,key,scale,shape,inv,low),m=pw_chord_notes(note+12,key,scale,shape,inv,high),used=0;
  for(unsigned i=0;i<m;i++)if(high[i]>=12)expected[used++]=high[i]-12;
  assert(n==used&&!memcmp(low,expected,n));low_cases++;
 }
 // Every root, shape and inversion remains bounded and MIDI note-off owns all generated members.
 for(unsigned root=0;root<128;root++)for(unsigned shape=1;shape<=14;shape++)for(unsigned inv=0;inv<4;inv++){
  core_init();core_settings c=config();c.chord_shape=shape;c.chord_key=(int)(root%25)-12;c.chord_scale=root%9;c.chord_inversion=inv;core_set(&c);core_midi(2,0x93,root,100);
  uint8_t expected[5];unsigned n=pw_chord_notes(root,c.chord_key,c.chord_scale,pw_chord_shape(shape),inv,expected);assert(core_primary_count()==n);
  unsigned found=0;for(unsigned i=0;i<8;i++)if(live(&slots[i].main)){assert(slots[i].main.note==expected[found++]&&slots[i].main.root==root);}assert(found==n);
  core_midi(2,0x83,root,0);assert(!core_held_count());
 }
 // Upper-note envelope modulation must use the Nyquist-capped wide path.
 for(unsigned note=84;note<128;note++){
  core_init();core_settings c=config();c.env2_pitch=120;core_set(&c);core_midi(0,0x90,note,127);frames(5000);
  assert(core_note_increment(0,0,note)<=2147418112u);if(note>=125)assert(core_note_increment(0,0,note)==2147418112u);
 }
 // Extreme coarse/bend/unison combinations stay finite without phase overflow.
 for(unsigned note=0;note<128;note++)for(unsigned edge=0;edge<2;edge++){
  core_init();core_settings c=config();c.coarse_tune=edge?60:-60;c.bend_range_offset=46;c.unison_count=8;c.unison_detune=100;c.env2_pitch=edge?120:-120;c.lfo_pitch_cents=edge?1200:-1200;c.wave_quality=1;core_set(&c);core_midi(0,0xe0,edge?127:0,edge?127:0);core_midi(0,0x90,note,127);frames(1000);
  for(unsigned i=0;i<8;i++)assert(core_voice_increment(i)<=2147418112u&&isfinite(slots[i].main.performance_cents));
 }
 // Boundary roots, all patterns, all octave ranges, both external clock ports.
 const uint8_t roots[4]={0,12,115,127};
 for(unsigned clock=1;clock<=2;clock++)for(unsigned mode=1;mode<=14;mode++)for(unsigned oct=1;oct<=3;oct++){
  core_init();core_settings c=config();c.arp_mode=mode;c.arp_clock=clock;c.arp_bpm=120;c.arp_octaves=oct;c.arp_gate_percent=50;core_set(&c);
  for(unsigned i=0;i<4;i++)core_midi(2,0x92,roots[i],127);
  unsigned port=clock==1?0:2;core_midi(port,0xfa,0,0);uint8_t step=255;pw_arp_random random={0,{0,0},255};
  for(unsigned k=0;k<50;k++){
   unsigned idx=mode>=11?pw_arp_octave_next(&step,4,mode,oct,0):mode>=8?pw_arp_random_next(&random,4,mode,oct):pw_arp_next(&step,4,mode,oct),pitch=roots[idx%4]+12*(idx/4);
   core_midi(port,0xf8,0,0);frames(1);assert(arp_gate==(pitch<=127));if(pitch<=127)assert(arp_pitch==pitch);
   for(unsigned j=0;j<11;j++){core_midi(port,0xf8,0,0);frames(1);}
  }
  core_midi(port,0xfc,0,0);frames(16000);assert(!core_primary_count());
 }
 // Internal octave steps at the upper edge become rests, never wrap to bass notes.
 core_init();core_settings c=config();c.arp_mode=1;c.arp_bpm=120;c.arp_octaves=3;c.arp_gate_percent=50;core_set(&c);core_midi(0,0x90,127,100);frames(1);assert(arp_gate&&arp_pitch==127);frames(11025);assert(!arp_gate);frames(11025);assert(!arp_gate);frames(11025);assert(arp_gate&&arp_pitch==127);
 core_init();for(unsigned i=0;i<5000;i++){
  c=config();c.note=i%128;c.playing=i%9==0;c.voice_limit=1+i%8;c.unison_count=1+(i/13)%8;c.chord_shape=(i/17)%15;c.chord_inversion=i%4;c.chord_key=(int)(i%25)-12;c.coarse_tune=(int)(i%121)-60;c.env2_pitch=(int)(i%241)-120;c.lfo_pitch_cents=(int)(i%2401)-1200;c.stereo_mode=i%4;c.stereo_spread=(int)(i%201)-100;c.tracking.pitch_wave=(int)(i%201)-100;c.tracking.pitch_cutoff=(int)(i%201)-100;c.filter.mode=i%4;c.filter.mix=100;c.filter.resonance=i%101;c.chorus_mode=(i/23)%3;c.delay_mode=(i/31)%3;c.delay_feedback=i%91;c.delay_mix=35;core_set(&c);
  unsigned port=i%3,ch=i%16,note=i%128;core_midi(port,0x90|ch,note,127);frames(256);core_midi(port,0x80|ch,note,0);if(i%29==0)core_midi(port,0xb0|ch,123,0);
 }
 c.playing=0;core_set(&c);core_panic();frames(1);assert(!core_primary_count());
 printf("PASS: TEST38 range: all 128 pitches on three ports and both wave modes, PLAY endpoints, ownership/pedal stops; %u below-tonic chord cases; 7168 chord root/shape/inversion cases; upper-envelope Nyquist cap, extreme pitch/unison, all external arp patterns/ranges and internal upper rests; 5000 sanitized stress blocks.\n",low_cases);
}
