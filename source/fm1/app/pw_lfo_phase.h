#pragma once
#include <stdint.h>
enum {PW_LFO_KEEP,PW_LFO_POLY,PW_LFO_MONO,PW_LFO_OFF};
uint32_t pw_lfo_spread_phase(unsigned slot,unsigned degrees);
void pw_lfo_assign_phase(uint32_t *phases,unsigned count,unsigned slot,unsigned mode,unsigned degrees,int any_held,int fresh);
