#define _POSIX_C_SOURCE 200809L
#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <time.h>
#include <stdio.h>
#include <assert.h>
static double stamp(void){struct timespec t;clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t);return t.tv_sec+t.tv_nsec*1e-9;}
int main(void){int32_t pcm[512];volatile int32_t check=0;
 puts("preset,unison_voices,trial,condition,model,mix,cpu_us_per_block,active_voice_mean");
 for(unsigned p=0;p<2;p++)for(unsigned voices=4;voices<=4;voices+=3)for(unsigned trial=0;trial<9;trial++)for(unsigned order=0;order<4;order++){
  unsigned condition=(unsigned[]){0,4,5,6}[trial%2?3-order:order];
  core_init();core_parallel_set(0);core_settings c;pw_preset_get(p?6:1,&c);c.voice_limit=4;c.unison_count=voices;c.bloom_mode=1;c.bloom_length=32;c.bloom_branches=0;c.bloom_mutation=0;c.bloom_ratchet=0;c.chorus_mode=0;c.delay_mode=0;c.arp_bpm=120;c.fx_reverb=condition!=0;c.reverb_model=condition<=4?(condition?condition-1:3):3;c.reverb_mix=condition==5?0:condition==6?100:60;c.reverb_time_ms=2000;c.reverb_size=70;c.reverb_damping=50;c.reverb_depth=20;c.reverb_speed=25;core_set(&c);c.playing=1;core_set(&c);
  for(unsigned i=0;i<400;i++)core_render(pcm,256,4096);
  unsigned active=0;double begin=stamp();
  for(unsigned i=0;i<4000;i++){core_render(pcm,256,4096);active+=core_primary_count();check^=pcm[i%512];}
  double elapsed=stamp()-begin;
  printf("%s,%u,%u,%u,%u,%u,%.6f,%.3f\n",p?"PlainPad":"Koto",voices,trial,condition,c.reverb_model,c.reverb_mix,elapsed*1e6/4000,(double)active/4000);fflush(stdout);
 }
 return check==123456789?1:0;
}
