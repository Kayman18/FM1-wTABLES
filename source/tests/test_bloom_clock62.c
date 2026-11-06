#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "../fm1/app/pw_bloom.h"
static uint32_t reference(unsigned bpm,uint32_t *rem){if(bpm<40)bpm=40;if(bpm>240)bpm=240;unsigned d=bpm*2;uint32_t sum=2646000u+*rem;*rem=sum%d;return sum/d;}
int main(void){
 for(unsigned bpm=40;bpm<=240;bpm++){
  uint32_t x=0,y=0,total=0;
  for(unsigned j=0;j<300;j++){uint32_t a=pw_bloom_eighth_interval(bpm,&x),b=reference(bpm,&y);assert(a==b&&x==y);total+=a;}
  assert(total>0);
 }
 puts("PASS TEST62: Bloom eighth-note intervals unchanged for all 40-240 BPM");
}
