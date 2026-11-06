#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main(void){assert(CORE_MAX_VOICES==8);int32_t*ref=malloc(512*3200*sizeof(int32_t)),pcm[512];assert(ref);
 for(unsigned dual=0;dual<2;dual++){
  core_init();core_parallel_set(dual);core_settings c;pw_preset_get(1,&c);c.voice_limit=6;c.unison_count=6;c.bloom_mode=1;c.bloom_length=32;c.bloom_branches=7;c.bloom_path=73;c.chorus_mode=2;c.fx_reverb=1;core_set(&c);c.playing=1;core_set(&c);
  for(unsigned b=0;b<3200;b++){
   if(b%80==0){assert(pw_preset_apply(b<2240?(b/80)%40:40+(b-2240)/80,&c));if(b<2240)c.bank=b/80;c.bloom_density=(b/80)%3*50;c.bloom_scale=(b/80)%PW_BLOOM_SCALES;c.bloom_offset=(int)(b/80)%49-24;c.bloom_ratchet=(b/80)%6;c.bloom_ratchet_chance=(b/80)%2?100:47;c.reverb_mix=100;c.bloom_path=(b/80)*3;c.bloom_branches=(b/80)%8;c.trigger_mode=(b/80)%3;c.fx_reverb=(b/160)%2;c.delay_mode=2;c.chorus_mode=2;c.reverb_model=(b/80)%4;c.reverb_size=1+(b/80)*99/39;c.reverb_damping=(b/80)*100/39;c.reverb_speed=500;c.reverb_depth=100;c.reverb_time_ms=30000;c.reverb_mix=100;core_set(&c);}
   if(b%113==0){unsigned step=(b/113)%32;core_bloom_edit(step,-60,c.bloom_scale);core_bloom_edit(step,-1,c.bloom_scale);}if(b%97==0)core_bloom_edit((b/97)%32,3,c.bloom_scale);core_render(pcm,256,4096);assert(!core_worker_fault());if(!dual)memcpy(ref+b*512,pcm,sizeof(pcm));else assert(!memcmp(ref+b*512,pcm,sizeof(pcm)));
  }
 }free(ref);puts("PASS: all 28 banks and 12 AV presets, six voices, 32 steps and live seed edits, chorus, both effects and four reverb models: branches/path/Drone: every block identical in single/dual rendering.");}
