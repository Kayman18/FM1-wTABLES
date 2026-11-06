#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main(void){int32_t *ref=malloc(512*800*sizeof(int32_t)),pcm[512];assert(ref);for(unsigned dual=0;dual<2;dual++){
 core_init();core_parallel_set(dual);core_settings c;pw_preset_get(1,&c);c.voice_limit=6;c.bloom_mode=1;for(unsigned r=0;r<4;r++){c.matrix_source[r]=(unsigned[]){3,6,9,11}[r];c.matrix_dest[r]=(unsigned[]){3,4,5,2}[r];c.matrix_amount[r]=(int[]){100,-100,100,30}[r];}core_set(&c);c.playing=1;core_set(&c);
 for(unsigned b=0;b<800;b++){if(b%20==0){assert(pw_preset_apply(b/20,&c));c.wave_quality=(b/20)%2;core_set(&c);}core_render(pcm,256,4096);assert(pw_tables_ready()&&!core_worker_fault());if(!dual)memcpy(ref+b*512,pcm,sizeof(pcm));else assert(!memcmp(ref+b*512,pcm,sizeof(pcm)));}}
 free(ref);puts("PASS: all 40 slots, both wave-quality modes, four active matrix routes including all three new labels: every block identical in single/dual rendering.");}
