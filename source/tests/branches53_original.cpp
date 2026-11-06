#include <array>
#include "bloom_reference/NativeState.h"
#include "bloom_reference/NativeFast.h"
#include <cassert>
extern "C" {
#include "../fm1/app/pw_bloom.h"
}
int main(){BloomState st;uint32_t random=7;unsigned count=0;
 for(unsigned sc=0;sc<7;sc++)for(unsigned len=1;len<=16;len++)for(unsigned trial=0;trial<100;trial++){
  uint8_t src[16],ancestor[16],dst[16];for(unsigned i=0;i<16;i++){src[i]=pw_bloom_random(&random)%61;ancestor[i]=pw_bloom_random(&random)%61;st.write<float>(0xc0000000+12*i,src[i]);st.write<float>(0xc0001000+12*i,ancestor[i]);}
  st.write<uint8_t>(0xc0000304,len);st.write<uint32_t>(0xc0000300,0xc0001000);
  uint32_t rng=pw_bloom_random(&random);st.write<uint32_t>(0x2404b00c,rng);
  st.call(0x240143d0,0xc0002000,0xc0000000,sc);pw_bloom_branch(dst,src,ancestor,len,sc,&rng);
  for(unsigned i=0;i<len;i++)assert(dst[i]==st.read<float>(0xc0002000+12*i));assert(rng==st.read<uint32_t>(0x2404b00c));++count;
 }
 pw_bloom b;pw_bloom_init(&b);for(unsigned i=0;i<254;i++)pw_bloom_service(&b,16,2);assert(b.ready);
 for(unsigned path=0;path<128;path++){b.depth=7;pw_bloom_boundary(&b,7,path);assert(b.node==0);for(unsigned depth=1;depth<=7;depth++){pw_bloom_boundary(&b,7,path);assert(b.node==(1u<<depth)-1+(path>>(7-depth)));}pw_bloom_boundary(&b,7,path);assert(b.node==0);}
 printf("PASS: %u branch transform/output/RNG comparisons; all 128 routes at all seven depths; bounded tree generation.\n",count);
}
