#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int main(void){int32_t *ref=malloc(512*200*sizeof(int32_t)),out[512];assert(ref);
for(unsigned preset=0;preset<PW_PRESET_COUNT;preset++)for(unsigned budget=4;budget<=8;budget+=4){
 for(unsigned dual=0;dual<2;dual++){
 core_init();core_parallel_set(dual);core_settings c;assert(pw_preset_get(preset,&c));c.voice_limit=budget;core_set(&c);
 for(unsigned block=0;block<200;block++){
 if(block%20==0)core_midi(0,0x90,48+(block/20)%12,110);
 if(block==150)core_midi(0,0xb0,123,0);
 core_render(out,256,4096);assert(!core_worker_fault());
 if(!dual)memcpy(ref+block*512,out,sizeof(out));else assert(!memcmp(ref+block*512,out,sizeof(out)));
 }
 assert(dual?core_worker_jobs()>0:core_worker_jobs()==0);
 }
}
free(ref);puts("PASS: all 17 presets at four/eight voices match single-core PCM exactly; CPU1 jobs verified, including Fatt FM1.");return 0;}
