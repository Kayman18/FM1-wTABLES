#pragma once
#include <stdint.h>
/* FM1 routing for the recovered signed envelope-to-wave controls.
 * Percent amounts span the seven frame intervals of this test bank. */
int32_t pw_wave_mod_position(int32_t base,float amp_depth,float amp_env,float env2_depth,float env2,float lfo_depth,float lfo);
