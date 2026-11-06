#pragma once
#include <stdint.h>
/* Amount units are tenths of a semitone: -120..120 = +/-1200 cents. */
int32_t pw_env_pitch_units(float env2,float amount,float lfo_cents);
int32_t pw_env_pitch_scale(int32_t octave_q19);
