#pragma once
#include <stdint.h>
/* Primary envelope recovered from disting EX 1.26.1 Poly Wavetable.
 * Numerically stable FM1 adaptation; see ENVELOPE.md for exact scope. */
enum { PW_ENV_IDLE, PW_ENV_SUSTAIN, PW_ENV_ATTACK, PW_ENV_DECAY, PW_ENV_RELEASE };
typedef struct { uint8_t attack,decay; int8_t sustain; uint8_t release; } pw_env_params;
typedef struct {
 float level,alpha,asymptote,target,roundoff;
 uint32_t remaining;
 uint8_t stage;
 pw_env_params params;
 uint8_t attack_shape,decay_shape,triggered;
} pw_env;
float pw_env_ms(unsigned stage,unsigned value);
float pw_env_ratio(unsigned shape);
void pw_env_coeff(float current,float target,float inverse_samples,float ratio,float *coefficient,float *offset);
void pw_env_init(pw_env *e);
void pw_env_set(pw_env *e,pw_env_params params);
void pw_env_shapes(pw_env *e,unsigned attack,unsigned decay);
void pw_env_on(pw_env *e);
void pw_env_trigger_mode(pw_env *e,unsigned triggered);
void pw_env_off(pw_env *e);
/* The per-sample tick is inlined: at eight voices it runs 44,100 times a
   second per voice, and the call alone cost more than its arithmetic.  The
   stage transition it reaches a few times per note stays out of line. */
void pw_env_advance(pw_env *e);
extern void (* volatile const pw_env_advance_ptr)(pw_env *);
static inline __attribute__((always_inline)) float pw_env_tick(pw_env *e){
 if(e->stage==PW_ENV_IDLE||e->stage==PW_ENV_SUSTAIN)return e->level;
 /* Algebraically the source's offset + coefficient*level. Compensated
  * increments preserve slow movement near sustain on the float-only FM1. */
 float step=(e->asymptote-e->level)*e->alpha-e->roundoff;
 float next=e->level+step;e->roundoff=(next-e->level)-step;e->level=next;
 int reached=e->asymptote>e->target?next>=e->target:next<=e->target;
 if(!--e->remaining||reached)pw_env_advance_ptr(e);
 return e->level;
}
