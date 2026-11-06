#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){int32_t *ref=malloc(512*400*sizeof(int32_t)),pcm[512];assert(ref);
for(unsigned preset=0;preset<PW_PRESET_COUNT;preset++)for(unsigned budget=4;budget<=6;budget++)for(unsigned dual=0;dual<2;dual++){
 core_init();core_parallel_set(dual);core_settings c;pw_preset_get(preset,&c);c.voice_limit=budget;c.bloom_mode=1;for(unsigned r=0;r<4;r++){c.matrix_source[r]=(unsigned[]){1,6,9,11}[r];c.matrix_dest[r]=1+r%2;c.matrix_amount[r]=(int[]){80,30,-100,-25}[r];}core_set(&c);c.playing=1;core_set(&c);
 for(unsigned b=0;b<400;b++){if(b==200){c.matrix_dest[1]=0;core_set(&c);}core_render(pcm,256,4096);assert(!core_worker_fault());if(!dual)memcpy(ref+b*512,pcm,sizeof(pcm));else assert(!memcmp(ref+b*512,pcm,sizeof(pcm)));}
}
free(ref);puts("PASS: all 15 presets, four/five/six voices, four active Sloth routes and live disconnect: identical single/dual PCM.");}
