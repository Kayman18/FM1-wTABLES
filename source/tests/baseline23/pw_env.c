#include "pw_env.h"
#include "fastmath.h"
#define RATE 44100.0f
float pw_env_ms(unsigned stage,unsigned value){
 if(value>127)value=127;
 if(stage==PW_ENV_ATTACK)return fm_expf((float)value*0.07571500539779663f);
 if(stage==PW_ENV_DECAY)return 20.0f*fm_expf((float)value*0.05212656036019325f);
 return 10.0f*fm_expf((float)value*0.06304226815700531f);
}
float pw_env_ratio(unsigned shape){if(shape>127)shape=127;return fm_expf((float)shape*0.03779527544975281f-5.0f);}
/* Direct translation of 9D0E8058 for reference comparisons. */
void pw_env_coeff(float current,float target,float inverse_samples,float ratio,float *coefficient,float *offset){
 float c=fm_expf(-inverse_samples*fm_logf((fm_fabsf(target-current)+ratio)/ratio));
 *coefficient=c;*offset=(target+(target<current?-ratio:ratio))*(1.0f-c);
}
static void stage(pw_env *e,unsigned which){
 e->stage=(uint8_t)which;e->roundoff=0.0f;
 if(which==PW_ENV_IDLE){e->level=0.0f;e->remaining=0;return;}
 if(which==PW_ENV_SUSTAIN){e->level=(float)e->params.sustain/127.0f;e->remaining=0;return;}
 unsigned value=which==PW_ENV_ATTACK?e->params.attack:which==PW_ENV_DECAY?e->params.decay:e->params.release;
 float samples=pw_env_ms(which,value)*(RATE/1000.0f);
 e->remaining=(uint32_t)(samples+0.5f);if(!e->remaining)e->remaining=1;
 e->target=which==PW_ENV_ATTACK?1.0f:which==PW_ENV_DECAY?(float)e->params.sustain/127.0f:0.0f;
 float ratio=pw_env_ratio(which==PW_ENV_ATTACK?e->attack_shape:e->decay_shape);
 float distance=fm_fabsf(e->target-e->level);
 float q=fm_logf((distance+ratio)/ratio)/samples;
 /* 1-exp(-q), without cancellation for long times. q <= .114 across all shapes.
  * Fifth order remainder < 3e-9 across this parameter range. */
 e->alpha=q*(1.0f-q*(0.5f-q*(1.0f/6.0f-q*(1.0f/24.0f-q/120.0f))));
 e->asymptote=e->target+(e->target<e->level?-ratio:ratio);
}
void pw_env_init(pw_env *e){*e=(pw_env){0};e->params=(pw_env_params){50,60,80,60};e->attack_shape=e->decay_shape=64;}
void pw_env_set(pw_env *e,pw_env_params p){
 if(p.attack>127)p.attack=127;if(p.decay>127)p.decay=127;if(p.sustain< -127)p.sustain= -127;if(p.release>127)p.release=127;
 int changed=p.sustain!=e->params.sustain;e->params=p;
 if(changed&&e->stage==PW_ENV_SUSTAIN)stage(e,PW_ENV_DECAY);
}
void pw_env_shapes(pw_env *e,unsigned attack,unsigned decay){e->attack_shape=attack>127?127:(uint8_t)attack;e->decay_shape=decay>127?127:(uint8_t)decay;}
void pw_env_on(pw_env *e){stage(e,PW_ENV_ATTACK);}
void pw_env_off(pw_env *e){if(e->stage!=PW_ENV_IDLE)stage(e,PW_ENV_RELEASE);}
float pw_env_tick(pw_env *e){
 if(e->stage==PW_ENV_IDLE||e->stage==PW_ENV_SUSTAIN)return e->level;
 /* Algebraically the source's offset + coefficient*level. Compensated
  * increments preserve slow movement near sustain on the float-only FM1. */
 float step=(e->asymptote-e->level)*e->alpha-e->roundoff;
 float next=e->level+step;e->roundoff=(next-e->level)-step;e->level=next;
 int reached=e->asymptote>e->target?next>=e->target:next<=e->target;
 if(!--e->remaining||reached){
  unsigned old=e->stage;e->level=e->target;
  stage(e,old==PW_ENV_ATTACK?PW_ENV_DECAY:old==PW_ENV_DECAY?PW_ENV_SUSTAIN:PW_ENV_IDLE);
 }
 return e->level;
}
