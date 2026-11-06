#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main(void){int32_t *ref=malloc(512*1200*sizeof(int32_t)),pcm[512];assert(ref);for(unsigned dual=0;dual<2;dual++){
 core_init();core_parallel_set(dual);core_settings c;pw_preset_get(2,&c);c.voice_limit=6;c.bloom_mode=1;core_set(&c);c.playing=1;core_set(&c);
 for(unsigned b=0;b<1200;b++){if(b%40==0){assert(pw_preset_apply((b/40)%29,&c));c.bank=(b/40)%16;c.wave_quality=(b/40)%2;core_set(&c);}core_render(pcm,256,4096);assert(pw_tables_ready()&&!core_worker_fault());if(!dual)memcpy(ref+b*512,pcm,sizeof(pcm));else assert(!memcmp(ref+b*512,pcm,sizeof(pcm)));}}
 free(ref);puts("PASS: compressed bank switches with six-voice Bloom produce identical single/dual PCM.");}
