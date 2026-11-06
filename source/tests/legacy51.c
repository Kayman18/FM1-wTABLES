#ifdef REFERENCE50
#include "baseline50/test_engine.c"
#else
#include "../fm1/app/test_engine.c"
#endif
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <stdio.h>
int main(void){core_init();core_settings c;pw_preset_get(0,&c);c.bloom_mode=1;c.voice_limit=6;core_set(&c);c.playing=1;core_set(&c);int32_t pcm[512];for(unsigned b=0;b<1200;b++){if(!(b%30)){assert(pw_preset_apply(b/30,&c));c.chorus_mode=(b/30)%3;c.delay_mode=(b/30)%3;core_set(&c);}core_render(pcm,256,4096);assert(!core_worker_fault());assert(fwrite(pcm,sizeof(pcm),1,stdout)==1);}return 0;}
