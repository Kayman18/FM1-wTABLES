#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static void render(unsigned blocks){int32_t pcm[512];for(unsigned b=0;b<blocks;b++){core_render(pcm,256,4096);assert(!core_worker_fault());for(unsigned i=0;i<512;i++)assert(pcm[i]>=-8388608&&pcm[i]<=8388607);}}
int main(void){
 float *storage=calloc(PW_DELAY_FRAMES*2+8,sizeof(float));assert(storage);unsigned maxbytes=0;
 for(unsigned m=0;m<4;m++)for(unsigned size=1;size<=100;size++){
  pw_reverb r;storage[PW_DELAY_FRAMES*2]=1234;
  unsigned used=pw_reverb_configure(&r,storage,PW_DELAY_FRAMES*2,m,size,44100);assert(used<=PW_DELAY_FRAMES*2);if(used*4>maxbytes)maxbytes=used*4;
  while(!pw_reverb_clear(&r,storage,2048));
  pw_reverb_controls(&r,30000,100,500,100);
  float out[2];for(unsigned i=0;i<9000;i++){pw_reverb_tick(&r,i==0?.01f:0,out);assert(isfinite(out[0])&&isfinite(out[1]));assert(fabsf(out[0])<10&&fabsf(out[1])<10);}
  assert(storage[PW_DELAY_FRAMES*2]==1234);
 }
 free(storage);printf("PASS: all 400 model/size layouts, extreme controls, wrapping and finite impulse responses; max %u bytes\n",maxbytes);
 core_init();core_settings c;pw_preset_get(1,&c);c.voice_limit=6;c.chorus_mode=2;c.delay_mode=2;c.bloom_mode=1;core_set(&c);c.playing=1;core_set(&c);render(100);
 assert(delay_active&&delay_mix>0&&!reverb_active);uint32_t jobs=core_worker_jobs();
 c.fx_reverb=1;core_set(&c);render(60);assert(reverb_active&&reverb_ready&&reverb_mix>0);assert(!delay_active&&delay_mix==0);assert(chorus_active==2);assert(bloom_running);assert(core_worker_jobs()>jobs);
 for(unsigned i=0;i<40;i++){unsigned model=i%4;assert(pw_preset_apply(i,&c));assert(c.fx_reverb==1&&c.playing==1&&c.bloom_mode==1);c.reverb_model=model;c.reverb_size=1+i*99/39;c.chorus_mode=2;core_set(&c);render(50);assert(reverb_ready&&reverb.model==model&&reverb.size==c.reverb_size);assert(bloom_running);}
 c.fx_reverb=0;c.delay_mode=2;core_set(&c);render(60);assert(!reverb_active&&reverb_mix==0&&delay_mix>0&&delay_active==2);
 for(unsigned i=0;i<80;i++){c.fx_reverb=i&1;c.reverb_model=i%4;c.reverb_size=1+i%100;core_set(&c);render(1);}
 c.fx_reverb=1;core_set(&c);render(60);assert(reverb_ready&&reverb_mix>0);core_panic();render(60);assert(!bloom_running);assert(!core_worker_fault());
 puts("PASS: chorus retained; shared-buffer switches, rapid control changes, all presets during sequencing, panic and dual worker.");
 return 0;
}
