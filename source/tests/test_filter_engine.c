#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../fm1/app/test_engine.h"
static int32_t pcm[512];
static void render(unsigned blocks){while(blocks--){core_render(pcm,256,4096);for(unsigned i=0;i<512;i++){assert(pcm[i]>=-8388608&&pcm[i]<=8388607);assert(pcm[i]==pcm[i^1]);}}}
int main(void){
 core_settings c={0,48,100,0,100,{0,0,127,0},{1,0,100,100}};
 core_init();core_set(&c);
 for(unsigned i=0;i<4;i++)core_midi(0,0x90,(uint8_t)(48+i*7),127);
 render(400);core_midi(0,0xb0,123,0);render(5000);assert(!core_voice_count());for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 /* Abrupt control targets are smoothed by the production audio path. */
 for(unsigned i=0;i<12000;i++){
  c.filter.mode=(uint8_t)(i%4);c.filter.cutoff=(uint8_t)(i%2?127:0);c.filter.resonance=(uint8_t)(i%101);c.filter.mix=(uint8_t)(i%3?100:50);c.bank=(uint8_t)(i%8);core_set(&c);
  core_midi((i%2)?0:2,(uint8_t)(0x90|(i%16)),(uint8_t)(24+i%60),127);render(1);
 }
 core_panic();render(1);assert(!core_voice_count());for(unsigned i=0;i<512;i++)assert(!pcm[i]);
 puts("PASS: filter tails decay to exact silence; 12000 full-gain blocks of four voices, steals, bank changes and cutoff/resonance/mode/mix extremes remain bounded and finite; panic is silent.");
}
