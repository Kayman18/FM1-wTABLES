#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int32_t pcm[512];
static void frames(unsigned n){while(n){unsigned m=n>256?256:n;core_render(pcm,m,4096);n-=m;assert(core_primary_count()<=voice_budget());}}
static core_settings config(void){core_settings c={0};c.note=48;c.level=30;c.env=(pw_env_params){0,0,127,0};c.env2=(pw_env_params){0,0,127,0};c.wave_quality=1;c.voice_limit=4;c.bloom_mode=1;c.bloom_length=16;c.bloom_scale=2;c.bloom_octave=3;c.arp_bpm=120;c.arp_gate_percent=50;return c;}
int main(void){assert(sizeof(core_settings)==108);unsigned cases=0;
 for(unsigned scale=0;scale<7;scale++)for(unsigned root=0;root<12;root++)for(unsigned oct=0;oct<5;oct++){
 core_init();core_settings c=config();c.bloom_scale=scale;c.bloom_root=root;c.bloom_octave=oct;c.bloom_mutation=100;core_set(&c);c.playing=1;core_set(&c);
 for(unsigned i=0;i<32;i++){bloom_left=0;frames(1);unsigned note=core_bloom_note();assert(note<=127);unsigned rel=(note+120-root)%12;assert(pw_bloom_quantize(rel,scale)==rel);assert(core_bloom_step()==i%16);++cases;}
 }
 // Exact rational sample timing across lengths, tempos and gate settings.
 for(unsigned length=1;length<=16;length++)for(unsigned bpm=40;bpm<=240;bpm+=40){
 core_init();core_settings c=config();c.bloom_length=length;c.arp_bpm=bpm;core_set(&c);c.playing=1;core_set(&c);uint32_t rem=0;
 for(unsigned i=0;i<length+2;i++){frames(1);assert(core_bloom_step()==i%length);unsigned interval=pw_arp_interval(bpm,&rem);frames(interval-1);assert(core_bloom_step()==i%length);}
 }
 core_init();core_settings c=config();c.sustain_hold=1;c.trigger_mode=1;c.chord_shape=14;c.break_time_ms=1000;c.unison_count=8;c.voice_limit=6;core_set(&c);c.playing=1;core_set(&c);frames(1);assert(core_primary_count()==6&&!core_pending_count());
 c.playing=0;core_set(&c);frames(10000);assert(!core_primary_count());
 // Seed return, reseed, live length/scale changes and normal MIDI coexistence.
 core_init();c=config();core_set(&c);c.playing=1;core_set(&c);frames(1);uint8_t original[16];memcpy(original,bloom.seed,16);
 c.bloom_mutation=100;core_set(&c);for(unsigned i=0;i<64;i++){bloom_left=0;frames(1);}assert(memcmp(original,bloom.notes,16));
 c.bloom_mutation=0;++c.bloom_restore;core_set(&c);frames(1);assert(!memcmp(original,bloom.notes,16)&&core_bloom_step()==0);
 ++c.bloom_reseed;core_set(&c);frames(1);assert(memcmp(original,bloom.seed,16)&&!memcmp(bloom.seed,bloom.notes,16));
 core_midi(0,0x90,72,100);c.playing=0;core_set(&c);frames(10000);assert(core_note_held(0,0,72));core_midi(0,0x80,72,0);frames(10000);assert(!core_primary_count());
 c.playing=1;core_set(&c);frames(10);core_panic();frames(500);assert(!core_primary_count()&&!bloom_running);for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 uint32_t rng=5;for(unsigned i=0;i<4000;i++){unsigned x=pw_bloom_random(&rng);c.bloom_length=x%16+1;c.bloom_scale=(x>>4)%7;c.bloom_mutation=(x>>8)%101;c.bloom_root=(x>>16)%12;c.bloom_octave=(x>>20)%5;c.playing=(x>>24)&1;c.voice_limit=(x>>25)%8+1;if(i%31==0)++c.bloom_reseed;if(i%47==0)++c.bloom_restore;core_set(&c);frames(256);}
 printf("PASS: Bloom engine: %u quantized outputs, 96 length/tempo cases, six-voice gates, seed restore, MIDI ownership, panic and 4000 sanitizer stress blocks.\n",cases);
}
