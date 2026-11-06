#include "../fm1/app/test_engine.c"
#include <assert.h>
#include <stdio.h>
static int16_t table[PW_FRAME_COUNT*2048];
static uint32_t rng=731u;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(void){
 for(unsigned i=0;i<PW_FRAME_COUNT*2048;i++)table[i]=(int16_t)random32();
 for(unsigned mip=0;mip<=10;mip++)for(unsigned i=0;i<12000;i++){
  pw_core_voice a={random32(),(int32_t)random32(),(int32_t)(random32()%5000)-500,4096},b=a;
  if(i%9==0)a.position_q8=b.position_q8=PW_FRAME_POS_MAX;
  const int16_t *frames=i%7?table:0;
  int32_t reference=pw_core_tick(&a,frames,PW_FRAME_COUNT,mip),actual=wave_tick_unscaled(&b,frames,mip);
  assert(actual==reference&&a.phase==b.phase&&a.position_q8==b.position_q8);
 }
 puts("PASS TEST72: 132000 exact oscillator/state comparisons, all 11 mip sizes, clipping edges, wrap and missing tables.");
}
