#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
static int32_t pcm[512];
int main(void){assert(VOICES==4&&CORE_MAX_VOICES==4);pw_bloom b;
 for(unsigned sc=0;sc<7;sc++)for(unsigned ix=0;ix<32;ix++){
  pw_bloom_init(&b);for(unsigned j=0;j<70;j++){unsigned n=pw_bloom_edit(&b,ix,1,sc);assert(n<=60&&pw_bloom_quantize(n,sc)==n);assert(b.seed[ix]==n&&b.notes[ix]==n);}for(unsigned j=0;j<70;j++){unsigned n=pw_bloom_edit(&b,ix,-1,sc);assert(pw_bloom_quantize(n,sc)==n);}assert(b.seed[ix]==0);
 }
 core_init();core_settings c;pw_preset_get(1,&c);c.voice_limit=8;c.unison_count=8;c.bloom_mode=1;c.bloom_length=32;c.bloom_branches=7;c.bloom_path=127;c.bloom_ratchet=5;c.bloom_ratchet_chance=100;core_set(&c);assert(st.voice_limit==4&&st.unison_count==4);c.playing=1;core_set(&c);
 for(unsigned i=0;i<700;i++)core_render(pcm,256,4096);assert(bloom.ready||bloom.valid);
 bloom_index=255;bloom.depth=7;bloom_left=0;
 for(unsigned i=0;i<257;i++){bloom_left=0;core_render(pcm,1,4096);assert(bloom_index==i%32);assert(bloom.depth==(i/32)%8);assert(core_primary_count()<=4);}
 unsigned ix=bloom_index,depth=bloom.depth;core_bloom_edit(31,10,st.bloom_scale);assert(bloom_index==ix&&bloom.depth==depth);assert(bloom.seed[31]==bloom.notes[31]);
 assert(pw_preset_apply(40,&c));assert(c.playing&&c.bloom_length==32&&c.bloom_branches==7);core_set(&c);assert(st.voice_limit<=4);
 c.bloom_length=1;core_set(&c);bloom_left=0;core_render(pcm,256,4096);assert(bloom_index==0);
 c.playing=0;core_set(&c);for(unsigned i=0;i<500;i++)core_render(pcm,256,4096);assert(!bloom_running);
 printf("PASS: 31360 scale-locked edits across 32 steps and seven scales; four-voice cap; 256-event branch cycle; live edit/preset continuity; length reduction and stop. Bloom storage %zu bytes.\n",sizeof(pw_bloom));
}
