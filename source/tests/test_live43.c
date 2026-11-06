#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int32_t pcm[512];
static void frames(unsigned n){while(n){unsigned m=n>256?256:n;core_render(pcm,m,4096);n-=m;assert(!core_worker_fault());}}
int main(void){core_settings c;pw_preset_default(&c);core_init();c.bloom_mode=1;c.bloom_mutation=35;c.bloom_length=7;c.arp_bpm=137;c.bloom_reseed=3;c.bloom_restore=2;c.panel_octave=1;core_set(&c);c.playing=1;core_set(&c);frames(37000);
for(unsigned i=0;i<17;i++){
 if(bloom_left<10)frames(20);
 unsigned left=bloom_left,gate=bloom_gate_left,index=bloom_index;pw_bloom before=bloom;
 assert(pw_preset_apply(i,&c));assert(c.playing&&c.bloom_mode&&c.bloom_length==7&&c.arp_bpm==137&&c.panel_octave==1&&c.bloom_reseed==3&&c.bloom_restore==2&&c.voice_limit==4);
 core_set(&c);frames(1);assert(!panic_requested&&bloom_running&&bloom_index==index&&bloom_left==left-1&&bloom_gate_left==(gate?gate-1:0)&&!memcmp(&before,&bloom,sizeof(bloom)));
 frames(129);
}
// Every root/scale, base octave, and panel octave: in range and scale preserved.
for(unsigned scale=0;scale<7;scale++)for(unsigned root=0;root<12;root++)for(unsigned oct=0;oct<5;oct++)for(int shift=-2;shift<=2;shift++){
 c.bloom_scale=scale;c.bloom_root=root;c.bloom_octave=oct;c.panel_octave=shift;c.bloom_mutation=0;core_set(&c);bloom_left=0;frames(1);unsigned note=core_bloom_note();assert(note<=127);unsigned rel=(note+132-root)%12;assert(pw_bloom_quantize(rel,scale)==rel);
}
// One-step pattern transposes by exactly 12 at ordinary pitches, without reseeding.
c.bloom_length=1;c.bloom_scale=2;c.bloom_root=0;c.bloom_octave=3;c.panel_octave=0;core_set(&c);bloom_left=0;frames(1);unsigned base=bloom_note;c.panel_octave=1;core_set(&c);bloom_left=0;frames(1);assert(bloom_note==base+12);c.panel_octave=-1;core_set(&c);bloom_left=0;frames(1);assert(bloom_note==base-12);
// External-clock arp keeps held roots, clock phase and pattern when instruments change.
core_init();pw_preset_default(&c);c.arp_mode=1;c.arp_clock=1;core_set(&c);core_midi(0,0x90,60,100);core_midi(0,0xfa,0,0);core_midi(0,0xf8,0,0);frames(1);
for(unsigned i=0;i<17;i++){unsigned phase=arp_clock_phase,index=arp_step_index,count=arp_count;assert(pw_preset_apply(i,&c));core_set(&c);frames(1);assert(arp_clock_running&&arp_clock_phase==phase&&arp_step_index==index&&arp_count==count&&!panic_requested);}
// MIDI-fed arp transposes its generated pitch, preserves source note-off ownership.
c.panel_octave=1;core_set(&c);for(unsigned i=0;i<12;i++)core_midi(0,0xf8,0,0);assert(core_arp_note()==72);core_midi(0,0x80,60,0);assert(!core_arp_count());
// Panel-fed arp does not apply octave a second time.
core_init();pw_preset_default(&c);c.arp_mode=1;c.panel_octave=1;core_set(&c);core_keys(1);frames(1);assert(core_arp_note()==65);for(unsigned i=0;i<8;i++)if(live(&slots[i].main))assert(slots[i].main.key_octave==0);core_keys(0);frames(1);assert(!core_arp_count());
puts("PASS: 17 live Bloom and 17 external-clock arp instrument changes preserve transport; 2100 octave/scale cases, exact octave shifts, MIDI ownership and no double panel transposition.");return 0;}
