#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "test_engine.h"
#include "pw_presets.h"
static double stamp(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC_RAW,&t);return t.tv_sec+1e-9*t.tv_nsec;}
static int cmp(const void*a,const void*b){double d=*(const double*)a-*(const double*)b;return (d>0)-(d<0);}
static volatile int sink;
static void trial(unsigned branch,unsigned mutate,unsigned ratchet,unsigned bpm,unsigned mode){
 enum {N=2200}; static double records[N],sorted[N]; int32_t pcm[512];
 core_init();core_parallel_set(mode);core_settings s;pw_preset_default(&s);
 s.voice_limit=4;s.unison_count=1;s.playing=0;s.bloom_mode=0;
 s.reverb_mix=100;s.fx_reverb=1;s.reverb_model=3;s.reverb_size=75;s.reverb_time_ms=15000;s.reverb_depth=30;
 s.env=(pw_env_params){0,5,80,55};s.env2=(pw_env_params){0,5,80,55};s.sequencer_bpm=bpm;
 core_set(&s);for(unsigned j=0;j<200;j++)core_render(pcm,256,4096);
 s.playing=1;s.bloom_mode=1;s.bloom_length=32;s.bloom_branches=branch;s.bloom_path=85;s.bloom_mutation=mutate;s.bloom_ratchet=ratchet;s.bloom_ratchet_chance=100;s.bloom_density=100;core_set(&s);
 double peak=0,avg=0;for(unsigned j=0;j<N;j++){
 double a=stamp();core_render(pcm,256,4096);double delta=(stamp()-a)*1e6;records[j]=delta;avg+=delta;if(delta>peak)peak=delta;sink^=pcm[300];
 }
 memcpy(sorted,records,sizeof(sorted));qsort(sorted,N,sizeof(double),cmp);
 printf("branch=%u mutate=%u ratchet=%u bpm=%u dual=%u avg=%.2f p95=%.2f p99=%.2f max=%.2f\n",branch,mutate,ratchet,bpm,mode,avg/N,sorted[(int)(N*.95)],sorted[(int)(N*.99)],peak);
}
int main(void){ for(unsigned d=0;d<2;d++){ trial(0,0,0,180,d);trial(7,0,0,180,d);trial(0,100,0,180,d);trial(7,100,0,180,d);trial(7,100,4,180,d);}fprintf(stderr,"sink %d\n",sink);return 0;}
