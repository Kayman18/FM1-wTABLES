#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/pw_env.h"
int main(void){
 double worst=0;
 for(unsigned value=0;value<=127;value++){
  pw_env e;pw_env_init(&e);pw_env_set(&e,(pw_env_params){value,value,80,value});pw_env_on(&e);
  for(unsigned pass=0;pass<3;pass++){
   unsigned stage=pass==0?PW_ENV_ATTACK:pass==1?PW_ENV_DECAY:PW_ENV_RELEASE;
   if(pass==2)pw_env_off(&e);
   assert(e.stage==stage);float start=e.level;float target=pass==0?1.0f:pass==1?80.0f/127.0f:0.0f;
   unsigned count=e.remaining,elapsed=0;double ratio=pw_env_ratio(64),asymptote=target+(target<start?-ratio:ratio);
   double q=log((fabs(target-start)+ratio)/ratio)/(pw_env_ms(stage,value)*44.1);
   while(e.stage==stage){
    float old=e.level,v=pw_env_tick(&e);elapsed++;
    assert(isfinite(v)&&v>=0&&v<=1);assert(elapsed<=count);
    if(e.stage==stage){assert(target>start?v>=old:v<=old);double expected=asymptote+(start-asymptote)*exp(-q*elapsed);double err=fabs(v-expected);if(err>worst)worst=err;assert(err<0.00005);}
   }
   assert(elapsed+2>=count);assert(e.level==target);
  }
  assert(e.stage==PW_ENV_IDLE);
 }
 pw_env e;pw_env_init(&e);pw_env_on(&e);for(int i=0;i<100;i++)pw_env_tick(&e);
 pw_env_off(&e);for(int i=0;i<100;i++)pw_env_tick(&e);float prior=e.level;pw_env_on(&e);assert(e.level==prior);
 puts("PASS: all 128 time settings, A/D/R durations, monotonic stages, sustained level, interrupted release and retrigger continuity.");printf("Worst curve deviation versus double analytic reference: %.9g\n",worst);
}
