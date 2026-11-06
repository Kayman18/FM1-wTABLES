#define _POSIX_C_SOURCE 200809L
#include "test_engine.h"
#include "pw_presets.h"
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
static volatile unsigned sink;
static double stamp(void){struct timespec t;clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&t);return t.tv_sec+1e-9*t.tv_nsec;}
struct Scenario{const char* name; int chorus,delay,cloud,filter,steals;};
static const struct Scenario scenarios[]={
 {"4voices-effects-off",0,0,0,0,0},
 {"4voices-chorus-mix0",1,0,0,0,0},
 {"4voices-chorus-mix50",1,0,0,0,0},
 {"4voices-delay-mix0",0,1,0,0,0},
 {"4voices-delay-mix50",0,1,0,0,0},
 {"4voices-clouds-mix0",0,0,1,0,0},
 {"4voices-clouds-mix70",0,0,1,0,0},
 {"4voices-steal-ratchet",0,0,0,1,1},
};
static double trial(const struct Scenario *T){
 int32_t pcm[512];core_settings s;core_init();core_parallel_set(0);pw_preset_default(&s);
 s.playing=0;s.bloom_mode=0;s.voice_limit=4;s.unison_count=1;s.env=(pw_env_params){0,0,127,98};s.env2=(pw_env_params){0,0,127,98};
 s.fx_reverb=T->cloud;s.reverb_mix=T->cloud&&__builtin_strstr(T->name,"mix0")?0:70;
 s.reverb_time_ms=15000;s.reverb_size=85;s.reverb_depth=20;
 s.chorus_mode=T->chorus?2:0;s.chorus_mix=T->chorus&&__builtin_strstr(T->name,"mix0")?0:50;
 s.delay_mode=T->delay?2:0;s.delay_sync=0;s.delay_time_ms=300;s.delay_mix=T->delay&&__builtin_strstr(T->name,"mix0")?0:50;
 s.filter.mode=T->filter?PW_FILTER_LP:0;s.filter.mix=T->filter?90:0;
 core_set(&s);
 for(unsigned i=0;i<4;i++)core_midi(0,0x90,48+i*3,100);
 for(unsigned j=0;j<120;j++)core_render(pcm,256,4096);
 const double begin=stamp();
 for(unsigned j=0;j<850;j++){
  if(T->steals)for(unsigned k=0;k<4;k++)core_midi(0,0x90,(unsigned char)(j%2?72+k*3:48+k*3),110);
  core_render(pcm,256,4096);sink^=(unsigned)pcm[(j*11)%512];
 }
 return 1e6*(stamp()-begin)/850;
}
int main(void){puts("scenario,trial,us_cpu_per_256");for(unsigned t=0;t<sizeof(scenarios)/sizeof(scenarios[0]);t++)for(unsigned k=0;k<3;k++)printf("%s,%u,%.4f\n",scenarios[t].name,k,trial(&scenarios[t]));fprintf(stderr,"sink=%u\n",sink);return 0;}
