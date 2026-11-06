#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../fm1/app/pw_env.h"
int main(void){
 double worst=0;unsigned cases=0;
 for(unsigned shape=0;shape<128;shape++)for(unsigned k=0;k<3;k++){
  unsigned time=k==0?0:k==1?60:127;
  pw_env e;pw_env_init(&e);pw_env_set(&e,(pw_env_params){time,time,80,time});pw_env_shapes(&e,shape,127-shape);pw_env_on(&e);
  for(unsigned pass=0;pass<3;pass++){
   unsigned stage=pass==0?PW_ENV_ATTACK:pass==1?PW_ENV_DECAY:PW_ENV_RELEASE;
   if(pass==2)pw_env_off(&e);
   assert(e.stage==stage);unsigned count=e.remaining,n=0;
   double start=e.level,target=pass==0?1.:pass==1?80.f/127.f:0.;
   double ratio=pw_env_ratio(pass==0?shape:127-shape),asym=target+(target<start?-ratio:ratio);
   double q=log((fabs(target-start)+ratio)/ratio)/(pw_env_ms(stage,time)*44.1);
   while(e.stage==stage){float old=e.level,v=pw_env_tick(&e);n++;
    assert(isfinite(v)&&v>=0&&v<=1);assert(n<=count);
    if(e.stage==stage){assert(target>start?v>=old:v<=old);double ref=asym+(start-asym)*exp(-q*n),err=fabs(v-ref);if(err>worst)worst=err;assert(err<.000005);}
   }
   assert(n+2>=count);assert(e.level==target);cases++;
  }
 }
 pw_env e;pw_env_init(&e);pw_env_set(&e,(pw_env_params){90,60,0,90});pw_env_on(&e);for(int i=0;i<100;i++)pw_env_tick(&e);
 float alpha=e.alpha,level=e.level;unsigned remaining=e.remaining;pw_env_shapes(&e,0,127);
 assert(e.alpha==alpha&&e.level==level&&e.remaining==remaining);pw_env_off(&e);assert(e.stage==PW_ENV_RELEASE);float previous=e.level;pw_env_on(&e);assert(e.level==previous);
 pw_env_shapes(&e,999,999);assert(e.attack_shape==127&&e.decay_shape==127);
 printf("PASS: all 128 attack/decay shapes at short/mid/maximum times, %u A/D/R stages, finite monotonic output, exact endpoints/durations, next-stage shape changes and interrupted release/retrigger. Max analytic error %.9g.\n",cases,worst);
}
