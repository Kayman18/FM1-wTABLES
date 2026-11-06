#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main(void){assert(CORE_MAX_VOICES==8 && CORE_DEFAULT_VOICES==4);int32_t*ref=malloc(512*800*sizeof(int32_t)),pcm[512];assert(ref);
 for(unsigned dual=0;dual<2;dual++){
  core_init();core_parallel_set(dual);core_settings c;pw_preset_get(1,&c);c.voice_limit=4;c.unison_count=3;c.bloom_mode=1;c.bloom_length=32;c.bloom_branches=7;c.bloom_path=73;c.chorus_mode=2;c.fx_reverb=1;core_set(&c);c.playing=1;core_set(&c);
  for(unsigned b=0;b<800;b++){
   if(b%20==0){assert(pw_preset_apply((b/20)%PW_PRESET_COUNT,&c));c.bank=(b/20)%PW_TABLE_BANKS;c.bloom_density=(b/20)%3*50;c.bloom_scale=(b/20)%PW_BLOOM_SCALES;c.bloom_offset=(int)(b/20)%49-24;c.bloom_ratchet=(b/20)%6;c.bloom_ratchet_chance=(b/20)%2?100:47;c.delay_sync=(b/20)%2;c.delay_division=(b/20)%4;c.sequencer_bpm=40+(b/20)*5;c.reverb_mix=100;c.bloom_path=(b/20)*3;c.bloom_branches=(b/20)%8;c.trigger_mode=(b/20)%3;c.fx_reverb=(b/160)%2;c.delay_mode=2;c.chorus_mode=2;c.reverb_model=(b/20)%4;c.reverb_size=1+(b/20)*99/39;c.reverb_damping=(b/20)*100/39;c.reverb_speed=500;c.reverb_depth=100;c.reverb_time_ms=30000;c.reverb_mix=100;core_set(&c);}
   if(b%113==0){unsigned step=(b/113)%32;core_bloom_edit(step,-60,c.bloom_scale);core_bloom_edit(step,-1,c.bloom_scale);}if(b%97==0)core_bloom_edit((b/97)%32,3,c.bloom_scale);core_render(pcm,256,4096);assert(!core_worker_fault());if(!dual)memcpy(ref+b*512,pcm,sizeof(pcm));else assert(!memcmp(ref+b*512,pcm,sizeof(pcm)));
  }
 }free(ref);puts("PASS TEST62: all 29 current presets, four voices, 32 steps and live seed edits, chorus, both effects with Clouds reverb: branches/path/Drone: every block identical in single/dual rendering.");}
