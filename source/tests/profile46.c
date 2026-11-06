#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <time.h>
#include <stdio.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t);return t.tv_sec+t.tv_nsec*1e-9;}
int main(void){int32_t pcm[512];unsigned presets[]={2,7,8};for(unsigned rep=0;rep<3;rep++)for(unsigned p=0;p<3;p++)for(unsigned mode=0;mode<2;mode++){core_init();core_settings c;pw_preset_get(presets[p],&c);c.voice_limit=6;c.bloom_mode=1;if(mode)for(unsigned r=0;r<4;r++){c.matrix_source[r]=(unsigned[]){1,6,9,11}[r];c.matrix_dest[r]=1+r%2;c.matrix_amount[r]=30;}core_set(&c);c.playing=1;core_set(&c);double start=now();unsigned peak=0;for(unsigned b=0;b<1800;b++){core_render(pcm,256,4096);unsigned v=core_primary_count();if(v>peak)peak=v;}printf("%s routes%u %.3fms peak%u\n",pw_preset_name(presets[p]),mode?4:0,(now()-start)*1000,peak);}}
