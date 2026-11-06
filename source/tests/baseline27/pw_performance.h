#pragma once
#include <stdint.h>
/* Recovered exponent table, adapted to a relative 44.1 kHz phase increment. */
int32_t pw_performance_units(float cents);
int32_t pw_performance_increment(uint32_t base,int32_t units);
float pw_performance_bend(int value,unsigned semitones);
