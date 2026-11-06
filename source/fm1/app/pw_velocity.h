/* Recovered Disting EX 1.26.1 Poly Wavetable velocity helpers. */
#pragma once
#include <stdint.h>
float pw_velocity_gain(unsigned velocity,float amount);
unsigned pw_velocity_map(unsigned velocity,unsigned gate,unsigned curve);
