#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
int main(void){core_init();core_settings c;pw_preset_get(1,&c);assert(c.bloom_density==100);
for(unsigned i=0;i<32;i++){core_bloom_edit(i,-60,1);core_bloom_edit(i,-1,1);assert(!core_bloom_enabled(i));core_bloom_edit(i,1,1);assert(core_bloom_enabled(i)&&core_bloom_seed(i)==0);core_bloom_edit(i,1,1);assert(core_bloom_seed(i)==2);}
c.bloom_mode=c.playing=1;c.bloom_length=32;c.bloom_ratchet=5;c.bloom_ratchet_chance=100;c.bloom_branches=7;c.bloom_mutation=100;core_set(&c);core_set(&c);bloom_sync();
for(unsigned density=0;density<=100;density+=50){c.bloom_density=density;core_set(&c);unsigned hits=0;
for(unsigned i=0;i<2048;i++){unsigned prev=bloom_index;bloom_left=0;bloom_tick();assert(bloom_index==(prev==255?0:(prev+1)%32));hits+=bloom_gate;if(!bloom_gate){assert(ratchet_count==1);ratchet_left=0;bloom_tick();assert(!bloom_gate);}}
if(!density)assert(!hits);else if(density==100)assert(hits==2048);else assert(hits>800&&hits<1250);
}
core_bloom_edit(31,-60,1);core_bloom_edit(31,-1,1);assert(!core_bloom_enabled(31));
for(unsigned i=0;i<128;i++){bloom_index=30;bloom_left=0;bloom_tick();assert(bloom_index==31&&!bloom_gate&&ratchet_count==1);}
c.bloom_density=37;assert(pw_preset_apply(40,&c));assert(c.bloom_density==37&&!core_bloom_enabled(31));pw_bloom_restore(&bloom);pw_bloom_reseed(&bloom);assert(!core_bloom_enabled(31));
puts("PASS: 32 OFF slots, re-enable and scale editing, density 0/50/100, ratchet suppression, clock advance, branch/mutation and preset/restore/reseed persistence.");}
