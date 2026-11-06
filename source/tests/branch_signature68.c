#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pw_bloom.h"
static uint32_t h(const void *p,unsigned n){const unsigned char *s=p;uint32_t a=2166136261u;for(unsigned i=0;i<n;i++)a=(a^s[i])*16777619u;return a;}
int main(void){
 for(unsigned seed=1;seed<=48;seed++)for(unsigned length=8;length<=32;length+=8)for(unsigned scale=0;scale<PW_BLOOM_SCALES;scale++){
 pw_bloom b;pw_bloom_init(&b);b.rng=seed*0x517cc1b7u;
 for(unsigned i=0;i<PW_BLOOM_STEPS;i++)b.notes[i]=b.seed[i]=(uint8_t)((seed*5+i*11)%61);
 for(unsigned j=0;j<1000&& !b.ready;j++)pw_bloom_service(&b,length,scale);
 if(!b.ready)return 3;
 printf("%u %u %u %08x %08x\n",seed,length,scale,h(b.tree[1],sizeof b.tree[1]),b.tree_rng);
 }
}
