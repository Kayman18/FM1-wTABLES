#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
int main(void){unsigned cases=0;
 for(unsigned rate=0;rate<3;rate++)for(unsigned count=0;count<6;count++)for(unsigned chance=0;chance<=100;chance+=100){
  core_init();core_settings c;pw_preset_get(0,&c);c.bloom_mode=1;c.bloom_ratchet=count;c.bloom_ratchet_chance=chance;c.arp_bpm=(unsigned[]){40,137,240}[rate];c.voice_limit=6;c.unison_count=6;core_set(&c);c.playing=1;core_set(&c);bloom_sync();
  uint32_t rem=0,interval=pw_arp_interval(c.arp_bpm,&rem);unsigned expected=chance?ratchet_counts[count]:1;unsigned attacks=0;uint32_t first_serial=0;
  for(unsigned t=0;t<interval;t++){unsigned before=ratchet_index;bloom_tick();if(t==0||ratchet_index!=before){assert(t==(uint64_t)attacks*interval/expected);attacks++;if(t==0)first_serial=serial;else assert(serial==first_serial);assert(core_primary_count()<=6);}assert(bloom_index==0);}
  assert(attacks==expected);bloom_tick();assert(bloom_index==1&&bloom_left==pw_arp_interval(c.arp_bpm,&rem)-1);
  c.playing=0;core_set(&c);bloom_sync();assert(!bloom_running&&ratchet_count==1&&!ratchet_left);cases++;
 }
 core_init();core_settings c;pw_preset_get(0,&c);c.bloom_mode=1;c.bloom_ratchet=5;c.bloom_ratchet_chance=100;core_set(&c);c.playing=1;core_set(&c);bloom_sync();bloom_tick();unsigned note=bloom_note;
 // Force MIDI to steal the sequencer voices; subtriggers must not take them back.
 for(unsigned i=0;i<6;i++)voice_note_on(0,0,80+i,100);
 uint32_t ages[6];for(unsigned i=0;i<6;i++)ages[i]=slots[i].main.age;
 ratchet_left=0;bloom_tick();assert(bloom_note==note);
 for(unsigned i=0;i<6;i++)assert(slots[i].main.age==ages[i]&&slots[i].main.port==0);
 assert(pw_preset_apply(40,&c)&&c.bloom_ratchet==5&&c.bloom_ratchet_chance==100&&c.playing);
 puts("PASS: 36 timing/count cases, exact subdivisions at 40/137/240 BPM, chance endpoints, six-voice bound, no repeat allocation, stop, MIDI ownership, preset continuity.");
}
