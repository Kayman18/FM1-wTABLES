#pragma once
#include "pw_sloth.h"
#include "test_engine.h"
typedef struct {pw_sloth sloth[3];float volts[12],value[2],step[2],target[2];unsigned left,remainder;} pw_matrix;
void pw_matrix_init(pw_matrix*);
void pw_matrix_tick(pw_matrix*,const core_settings*);
void pw_matrix_advance(pw_matrix*,const core_settings*,unsigned frames);
unsigned pw_matrix_active(const pw_matrix*,const core_settings*);
float pw_matrix_resonance(float index);
const char *pw_matrix_source_name(unsigned);
const char *pw_matrix_source_rate(unsigned);
