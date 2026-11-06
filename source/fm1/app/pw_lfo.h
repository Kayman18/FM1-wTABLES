#pragma once
#include <stdint.h>
typedef struct {uint32_t phase;} pw_lfo;
typedef struct {int8_t rate,wave,cutoff;uint8_t pitch;} pw_lfo_params;
/* Signed rate index -100..100; zero stops phase. Depths are percent,
 * semitones and cents respectively. FM1 routing is documented in LFO.md. */
float pw_lfo_hz(int rate);
int32_t pw_lfo_increment(int rate);
float pw_lfo_tick(pw_lfo *state,int32_t increment);
float pw_lfo_value(const pw_lfo *state);
int32_t pw_lfo_pitch_scale(float cents);
