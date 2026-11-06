#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "pw_bloom.h"
static double stamp(void){struct timespec t;clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t);return t.tv_sec+1e-9*t.tv_nsec;}
static volatile uint32_t sink;
int main(void){const int N=850;double t0=stamp();for(int q=0;q<N;q++){
 pw_bloom b;pw_bloom_init(&b);b.rng=q*0x517cc1b7u +1;
 for(int i=0;i<PW_BLOOM_STEPS;i++)b.notes[i]=(uint8_t)((q*5+i*11)%61);
 for(int j=0;j<256;j++)pw_bloom_service(&b,32,q%PW_BLOOM_SCALES);
 sink+=b.tree[1][44][12]+b.tree_rng;
 }printf("%.3f microsec per tree, %.3f microsec per node, sink=%u\n",1e6*(stamp()-t0)/N,1e6*(stamp()-t0)/(N*255),sink);return 0;}
