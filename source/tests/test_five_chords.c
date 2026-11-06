#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int32_t pcm[512];
static core_settings config(unsigned shape){core_settings c={0};c.note=48;c.level=60;c.env=(pw_env_params){0,0,127,0};c.env2=(pw_env_params){0,0,64,0};c.filter.cutoff=96;c.wave_quality=1;c.delay_time_ms=500;c.chord_shape=shape;c.arp_bpm=120;c.arp_octaves=1;c.arp_gate_percent=50;return c;}
static void frames(unsigned n){while(n){unsigned m=n>256?256:n;core_render(pcm,m,4096);n-=m;assert(core_primary_count()<=voice_budget()&&arp_count<=16);}}
static void sort(uint8_t *a,unsigned n){for(unsigned i=1;i<n;i++){uint8_t v=a[i];unsigned j=i;while(j&&a[j-1]>v){a[j]=a[j-1];j--;}a[j]=v;}}
int main(void){
 assert(sizeof(core_settings)==96);
 const unsigned maps[3]={10,12,13};for(unsigned i=0;i<3;i++)assert(pw_chord_shape(12+i)==maps[i]);
 const unsigned member[4][4]={{0},{0,4},{0,3,4},{0,1,3,4}};unsigned cases=0;
 for(unsigned shape=12;shape<=14;shape++)for(unsigned scale=0;scale<9;scale++)for(unsigned inv=0;inv<4;inv++)for(unsigned root=24;root<=83;root++)for(unsigned budget=1;budget<=4;budget++){
  core_init();core_settings c=config(shape);c.chord_scale=scale;c.chord_inversion=inv;c.chord_key=(int)(root%25)-12;c.voice_limit=budget;c.unison_mode=2;core_set(&c);
  unsigned port=root%3,ch=root%16;uint8_t all[5],filtered[5],expected[4],actual[4];unsigned n=pw_chord_notes(root,c.chord_key,scale,maps[shape-12],inv,all),used=0;
  for(unsigned i=0;i<n;i++)if(all[i]<=127)filtered[used++]=all[i];
  unsigned count=used<budget?used:budget;
  for(unsigned i=0;i<count;i++)expected[i]=filtered[used==5?member[budget-1][i]:i];
  core_midi(port,0x90|ch,root,100);unsigned found=0;
  for(unsigned i=0;i<4;i++)if(slots[i].main.held){poly_voice *v=&slots[i].main;assert(v->root==root&&v->port==port&&v->channel==ch&&v->layers==1);actual[found++]=v->note;}
  assert(found==count);sort(expected,count);sort(actual,found);assert(!memcmp(expected,actual,count));
  core_midi(port,0x80|ch,root,0);assert(!core_held_count());cases++;
 }
 assert(cases==25920);
 // Audible root-position reductions preserve the sixth/seventh/octave or ninth extension.
 const uint8_t direct[3][4]={{48,52,57,60},{48,52,59,60},{48,52,59,62}};
 for(unsigned shape=12;shape<=14;shape++){
  core_init();core_settings c=config(shape);core_set(&c);core_midi(0,0x90,48,100);uint8_t got[4];for(unsigned i=0;i<4;i++)got[i]=slots[i].main.note;sort(got,4);assert(!memcmp(got,direct[shape-12],4));
 }
 // Internal clock consumes all five notes even with a one-voice budget.
 const unsigned full[3][5]={{48,52,55,57,60},{48,52,55,59,60},{48,52,55,59,62}};
 for(unsigned shape=12;shape<=14;shape++){
  core_init();core_settings c=config(shape);c.arp_mode=1;c.voice_limit=1;core_set(&c);core_midi(0,0x90,48,100);
  for(unsigned k=0;k<10;k++){frames(k?11025:1);assert(arp_pitch==full[shape-12][k%5]);}
  core_midi(0,0x80,48,0);frames(22050);assert(!arp_count&&!core_primary_count());
 }
 // Both clock ports, every pattern and octave range use the complete five-member source chord.
 for(unsigned source=1;source<=2;source++)for(unsigned shape=12;shape<=14;shape++)for(unsigned mode=1;mode<=14;mode++)for(unsigned oct=1;oct<=3;oct++){
  core_init();core_settings c=config(shape);c.arp_clock=source;c.arp_mode=mode;c.arp_octaves=oct;c.unison_mode=2;c.voice_limit=2;core_set(&c);
  unsigned port=source==1?0:2;core_midi(2,0x93,36,100);core_midi(port,0xfa,0,0);
  uint8_t pitches[5],step=255;unsigned n=pw_chord_notes(36,0,0,maps[shape-12],0,pitches);assert(n==5);pw_arp_random random={0,{0,0},255};
  for(unsigned k=0;k<20;k++){
   unsigned idx=mode>=11?pw_arp_octave_next(&step,n,mode,oct,shape!=14):mode>=8?pw_arp_random_next(&random,n,mode,oct):pw_arp_next(&step,n,mode,oct);
   core_midi(port,0xf8,0,0);frames(1);assert(arp_pitch==pitches[idx%n]+12*(idx/n)&&arp_port==2&&arp_channel==3&&core_primary_count()<=2);
   for(unsigned j=0;j<11;j++){core_midi(port,0xf8,0,0);frames(1);}
  }
  core_midi(port,0xfc,0,0);frames(22050);assert(!arp_gate&&!core_primary_count());
 }
 // Strum fits the reduced group, respects direction and cancels pending notes on release.
 for(unsigned direction=0;direction<=2;direction++){
  core_init();core_settings c=config(14);c.break_time_ms=100;c.break_direction=direction;core_set(&c);core_midi(0,0x90,48,100);assert(core_pending_count()==3);
  for(unsigned i=0;i<4;i++)assert(slots[i].main.wait_samples==4410u*i);
  frames(4411);assert(core_pending_count()==2);core_midi(0,0x80,48,0);assert(!core_pending_count());frames(22050);assert(!core_primary_count());
 }
 // New shapes preserve global Hold/pedal ownership across edits and stop messages.
 core_init();core_settings c=config(14);c.sustain_hold=1;core_set(&c);core_midi(2,0x92,48,100);core_midi(2,0x82,48,0);assert(core_note_sustained(2,2,48));c.chord_shape=12;c.chord_inversion=3;core_set(&c);core_midi(2,0xb2,120,0);frames(1);assert(!core_primary_count());
 for(unsigned i=0;i<4000;i++){
  c=config(i%15);c.chord_scale=i%9;c.chord_inversion=i%4;c.chord_key=(int)(i%25)-12;c.voice_limit=1+i%4;c.unison_mode=i%3;c.break_time_ms=i%1001;c.break_direction=i%3;c.sustain_hold=i%2;c.arp_mode=i%15;c.arp_clock=i%3;c.arp_octaves=1+i%3;core_set(&c);
  unsigned port=i%3,ch=i%16,root=24+i%60;core_midi(port,0x90|ch,root,100);if(i%7==0)core_midi(port,0xfa,0,0);core_midi(port,0xf8,0,0);frames(256);core_midi(port,0x80|ch,root,0);if(i%17==0)core_midi(port,0xb0|ch,123,0);
 }
 core_panic();frames(22050);assert(!arp_count&&!core_primary_count());
 puts("PASS: five-note chords: 25920 direct-voicing/ownership cases; all 3 shapes, 9 scales, 4 inversions, 60 roots and 4 voice budgets; full five-note internal/external arpeggios in all 14 patterns, strum, Hold/controller stops and 4000 sanitized stress blocks.");
}
