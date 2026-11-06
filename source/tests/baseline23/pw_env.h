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
 uint8_t attack_shape,decay_shape;
} pw_env;
float pw_env_ms(unsigned stage,unsigned value);
float pw_env_ratio(unsigned shape);
void pw_env_coeff(float current,float target,float inverse_samples,float ratio,float *coefficient,float *offset);
void pw_env_init(pw_env *e);
void pw_env_set(pw_env *e,pw_env_params params);
void pw_env_shapes(pw_env *e,unsigned attack,unsigned decay);
void pw_env_on(pw_env *e);
void pw_env_off(pw_env *e);
float pw_env_tick(pw_env *e);
