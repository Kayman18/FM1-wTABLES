#pragma once
#include <stdint.h>
enum { PW_FILTER_OFF, PW_FILTER_LP, PW_FILTER_BP, PW_FILTER_HP };
typedef struct { float low,band,high; } pw_filter;
typedef struct {uint8_t mode,cutoff,resonance,mix;} pw_filter_params;
void pw_filter_reset(pw_filter *state);
float pw_filter_hz(unsigned index);
float pw_filter_g(unsigned index);
float pw_filter_q(unsigned index);
float pw_filter_damping(float g,float q);
void pw_filter_tick(pw_filter *state,float input,float g,float damping);
float pw_filter_output(const pw_filter *state,unsigned mode,float dry);
int pw_filter_live(const pw_filter *state);
