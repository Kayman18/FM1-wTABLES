#pragma once
#include "test_engine.h"
#define PW_PRESET_COUNT 29u
void pw_preset_default(core_settings *out);
int pw_preset_get(unsigned index,core_settings *out);
const char *pw_preset_name(unsigned index);
const char *pw_preset_category(unsigned index);
int pw_preset_apply(unsigned index,core_settings *current);
