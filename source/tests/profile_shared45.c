#include "test_engine.c"
#include "pw_presets.h"
#include <time.h>
#include <stdio.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t);return t.tv_sec+t.tv_nsec*1e-9;}
int main(void){int32_t pcm[512];unsigned presets[]={0,2,8,10,0};for(unsigned p=0;p<5;p++)for(unsigned mode=0;mode<4;mode++){core_init();core_settings c;pw_preset_get(presets[p],&c);c.voice_limit=8;if(p==4){c.chorus_mode=2;c.chorus_mix=100;c.delay_mode=2;c.delay_style=1;c.delay_return_db=-6;c.delay_feedback=60;}core_set(&c);if(mode==1)core_midi(0,0x90,60,100);if(mode==2)for(unsigned n=0;n<8;n++)core_midi(0,0x90,48+n*4,100);if(mode==3){c.bloom_mode=1;core_set(&c);c.playing=1;core_set(&c);}double start=now();unsigned sum=0,max=0;for(unsigned b=0;b<1800;b++){core_render(pcm,256,4096);unsigned v=core_primary_count();sum+=v;if(v>max)max=v;}printf("%s mode%u %.2fms mean voices %.2f peak %u\n",p==4?"Both FX":pw_preset_name(presets[p]),mode,(now()-start)*1000,(double)sum/1800,max);}}
