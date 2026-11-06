#pragma once
#include <stdint.h>
#define CORE_MAX_VOICES 8u
#define CORE_DEFAULT_VOICES 4u
#define CORE_MAX_UNISON 3u
#include "pw_env.h"
#include "pw_filter.h"
#include "pw_lfo.h"
#include "pw_tracking.h"
#include "pw_lfo_phase.h"
typedef struct {uint8_t playing,note,position,bank,level;pw_env_params env;pw_filter_params filter;int8_t filter_env_depth;pw_env_params env2;int8_t filter_env2_depth;pw_lfo_params lfo;int8_t wave_env_depth,wave_env2_depth;pw_tracking_params tracking;uint8_t lfo_phase_mode,lfo_spread;int8_t env2_pitch;
 int8_t amp_attack_shape,amp_decay_shape,env2_attack_shape,env2_decay_shape; /* offsets from original default 64 */int8_t coarse_tune,fine_tune,panel_octave,bend_range_offset; /* bend range = 2 + offset */uint8_t wave_quality; /* 0 legacy256, 1 automatic mip */uint8_t chorus_mode,chorus_mix,stereo_mode;int8_t stereo_spread;uint8_t unison_mode,unison_detune;int8_t unison_wave_spread;uint8_t delay_mode,delay_feedback,delay_mix;uint16_t delay_time_ms;uint8_t velocity_mode,velocity_amount,velocity_curve,trigger_mode;int16_t lfo_pitch_cents;uint8_t sustain_mode;uint8_t voice_limit,sustain_hold;uint16_t sequencer_bpm;uint8_t bloom_gate_percent;int8_t gain_db,delay_return_db;uint8_t delay_style,unison_count;uint8_t bloom_mode,bloom_length,bloom_scale,bloom_root,bloom_octave,bloom_mutation,bloom_branches,bloom_path,bloom_ratchet,bloom_ratchet_chance;uint8_t bloom_density;int8_t bloom_offset;uint32_t bloom_reseed,bloom_restore;uint8_t matrix_source[4],matrix_dest[4];int8_t matrix_amount[4];uint8_t fx_reverb,reverb_model,reverb_size,reverb_damping,reverb_depth,reverb_mix;uint16_t reverb_time_ms,reverb_speed;uint8_t delay_sync,delay_division;uint8_t delay_spread,delay_cross;uint16_t delay_lowcut_hz,delay_highcut_hz;} core_settings;
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

unsigned core_primary_count(void);


/* Zero retains the older 1/2/4 enum for existing engine callers. UI uses count. */
static inline unsigned core_unison_count(const volatile core_settings *c){unsigned legacy=c->unison_mode>2u?2u:c->unison_mode;unsigned n=c->unison_count?c->unison_count:(1u<<legacy);return n>CORE_MAX_UNISON?CORE_MAX_UNISON:n;}
static inline unsigned core_unison_effective(const volatile core_settings *c){unsigned n=core_unison_count(c),b=c->voice_limit?c->voice_limit:CORE_DEFAULT_VOICES;if(c->unison_count)return n>b?b:n;while(n>b)n>>=1;return n;}

unsigned core_bloom_step(void);
unsigned core_bloom_seed(unsigned index);
unsigned core_bloom_enabled(unsigned index);
unsigned core_bloom_edit(unsigned index,int delta,unsigned scale);
unsigned core_bloom_branch(void);
unsigned core_bloom_note(void);
unsigned core_bloom_pattern(unsigned index);

unsigned core_worker_fault(void);
unsigned core_worker_jobs(void);
unsigned core_worker_status(void);

void core_parallel_set(unsigned enabled);
unsigned core_parallel_enabled(void);

float core_matrix_voltage(unsigned source);
