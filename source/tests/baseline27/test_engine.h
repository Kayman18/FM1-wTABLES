#pragma once
#include <stdint.h>
#include "pw_env.h"
#include "pw_filter.h"
#include "pw_lfo.h"
#include "pw_tracking.h"
#include "pw_lfo_phase.h"
typedef struct {uint8_t playing,note,position,bank,level;pw_env_params env;pw_filter_params filter;int8_t filter_env_depth;pw_env_params env2;int8_t filter_env2_depth;pw_lfo_params lfo;int8_t wave_env_depth,wave_env2_depth;pw_tracking_params tracking;uint8_t lfo_phase_mode,lfo_spread;int8_t env2_pitch;
 int8_t amp_attack_shape,amp_decay_shape,env2_attack_shape,env2_decay_shape; /* offsets from original default 64 */int8_t coarse_tune,fine_tune,panel_octave,bend_range_offset; /* bend range = 2 + offset */uint8_t wave_quality; /* 0 legacy256, 1 automatic mip */uint8_t chorus_mode,chorus_mix,stereo_mode;int8_t stereo_spread;uint8_t unison_mode,unison_detune;int8_t unison_wave_spread;uint8_t delay_mode,delay_feedback,delay_mix;uint16_t delay_time_ms;uint8_t velocity_mode,velocity_amount,velocity_curve,trigger_mode;int16_t lfo_pitch_cents;uint8_t chord_shape,chord_scale;int8_t chord_key;uint8_t chord_inversion;uint16_t break_time_ms;uint8_t break_direction,sustain_mode;} core_settings;
void core_init(void);
void core_set(const core_settings*);
void core_keys(uint32_t keys);
/* Called by the audio owner, before core_render. */
void core_midi(unsigned port,uint8_t status,uint8_t note,uint8_t velocity);
void core_panic(void);
void core_render(int32_t*,uint32_t,uint32_t);
int32_t core_wave_sample(uint32_t index);
unsigned core_active_note(void);
unsigned core_env_stage(void);
float core_env_level(void);
unsigned core_voice_count(void);
unsigned core_held_count(void);
unsigned core_note_held(unsigned port,unsigned channel,unsigned note);
float core_note_cutoff(unsigned port,unsigned channel,unsigned note);
float core_voice_cutoff(unsigned slot);
float core_env2_level(void);
float core_note_env2(unsigned port,unsigned channel,unsigned note);
float core_lfo_level(void);
uint32_t core_lfo_phase(void);
float core_note_lfo(unsigned port,unsigned channel,unsigned note);
float core_voice_wave(unsigned slot);
float core_note_wave(unsigned port,unsigned channel,unsigned note);
uint32_t core_voice_phase(unsigned slot);
uint32_t core_note_phase(unsigned port,unsigned channel,unsigned note);
float core_voice_pitch(unsigned slot);
float core_note_pitch(unsigned port,unsigned channel,unsigned note);
int32_t core_note_pitch_scale(unsigned port,unsigned channel,unsigned note);

float core_note_performance(unsigned port,unsigned channel,unsigned note);
uint32_t core_note_increment(unsigned port,unsigned channel,unsigned note);
unsigned core_note_sustained(unsigned port,unsigned channel,unsigned note);
unsigned core_sustain(unsigned port,unsigned channel);

unsigned core_voice_mip(unsigned slot);
unsigned core_note_mip(unsigned port,unsigned channel,unsigned note);

uint32_t core_note_osc_phase(unsigned port,unsigned channel,unsigned note);

float core_output_level(unsigned channel);

unsigned core_note_layers(unsigned port,unsigned channel,unsigned note);
unsigned core_voice_note(unsigned slot);
float core_voice_detune(unsigned slot);
uint32_t core_voice_increment(unsigned slot);

unsigned core_pending_count(void);
uint32_t core_voice_delay(unsigned slot);
