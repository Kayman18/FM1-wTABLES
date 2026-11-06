#pragma once
#include <stdint.h>
typedef struct {float ring[256][4];uint32_t phase,write;} pw_chorus;
void pw_chorus_reset(pw_chorus *s);
void pw_chorus_tick(pw_chorus *s,const float input[4],float output[4],unsigned mode,unsigned rate);
/* FM1 production: continuous phase lookup, retaining recovered delay shape. */
void pw_chorus_tick_smooth(pw_chorus *s,const float input[4],float output[4],unsigned mode);
float pw_chorus_offset_smooth(unsigned mode,uint32_t phase);
float pw_chorus_mix_headroom(float mix);

void pw_chorus_tick_stereo(pw_chorus*,const float input[4],float output[4],unsigned mode);
