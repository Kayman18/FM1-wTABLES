#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
static int32_t pcm[512];
static void blocks(unsigned n){while(n--)core_render(pcm,256,4096);}
int main(void){core_init();core_settings c;pw_preset_get(0,&c);c.voice_limit=6;c.env=(pw_env_params){0,0,100,0};c.env2=c.env;c.trigger_mode=2;core_set(&c);blocks(400);
 core_midi(0,0x90,60,100);blocks(20);core_midi(0,0x80,60,0);blocks(50);assert(core_primary_count()==1);assert(slots[0].main.env.stage==PW_ENV_SUSTAIN);assert(slots[0].main.sustained==2);
 core_midi(0,0xb0,64,0);blocks(5);assert(core_primary_count()==1);
 core_midi(0,0xb0,123,0);blocks(50);assert(!core_primary_count());
 core_midi(0,0x90,60,100);core_midi(0,0x80,60,0);blocks(20);c.trigger_mode=0;core_set(&c);blocks(50);assert(!core_primary_count());
 c.trigger_mode=2;core_set(&c);for(unsigned n=40;n<60;n++){core_midi(0,0x90,n,100);core_midi(0,0x80,n,0);blocks(2);assert(core_primary_count()<=6);}core_panic();blocks(2);assert(!core_primary_count());
 c.bloom_mode=1;c.bloom_branches=7;c.bloom_path=127;c.bloom_length=16;c.bloom_mutation=0;core_set(&c);c.playing=1;core_set(&c);blocks(300);assert(bloom.ready||bloom.valid);
 bloom_index=255;bloom.depth=7;bloom_left=0;blocks(1);assert(bloom.node==0);
 for(unsigned pass=1;pass<=8;pass++){for(unsigned i=0;i<16;i++){bloom_left=0;blocks(1);}assert(bloom.depth==pass%8);}
 unsigned ix=bloom_index,node=bloom.node;assert(pw_preset_apply(40,&c));assert(c.bloom_branches==7&&c.bloom_path==127&&c.playing);core_set(&c);blocks(1);assert(bloom_index==ix&&bloom.node==node);
 c.playing=0;core_set(&c);blocks(500);assert(!core_primary_count());
 puts("PASS: Drone sustain, pedal immunity, CC123, mode exit, six-voice stealing, panic; all branch passes, preset continuity, sequencer stop.");
}
