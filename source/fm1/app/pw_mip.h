#pragma once
#include <stdint.h>
/* Recovered power-of-two resolution selector, with explicit sample rate. */
unsigned pw_mip_for_bucket(unsigned index,unsigned sample_rate);
unsigned pw_mip_index(float hz);
unsigned pw_mip_select(uint32_t increment,int32_t scale);
