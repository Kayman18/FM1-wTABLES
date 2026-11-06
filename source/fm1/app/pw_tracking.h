#pragma once
#include <stdint.h>
typedef struct {int8_t velocity_wave,pitch_wave,velocity_cutoff,pitch_cutoff;} pw_tracking_params;
/* Source tracking reference: MIDI 48, velocity normalized by 128. */
float pw_tracking_wave_offset(unsigned note,unsigned velocity,float velocity_depth,float pitch_depth);
float pw_tracking_cutoff_offset(unsigned note,unsigned velocity,float velocity_depth,float pitch_depth);
