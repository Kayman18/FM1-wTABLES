#pragma once
#include <stdint.h>
#include "gp5_reverb.h"
/* TEST63 experimental: clean Clouds-style Griesinger/Dattorro reverb as used in
   genAMB RC2 Stretch ST3; MI Rings FxEngine topology with 32-bit float storage.
   Caller-owned RAM shared with Rikoshet Lite delay. No allocation in audio path.
   Source of adaptation: genAMB app/rings_port/rings_port.cc, MIT/MI Rings. */
#define PW_CLOUD_TANK_FLOATS 16384u
#define PW_CLOUD_EXTRA_FLOATS (149u+211u)
#define PW_CLOUD_FLOATS (PW_CLOUD_TANK_FLOATS+PW_CLOUD_EXTRA_FLOATS)
#define PW_REVERB_CLOUDS 3u
#define PW_REVERB_MODELS 11u
/* Model 3 remains Clouds so existing preset IDs retain their meaning.
   GP5 uses Decay/Tone/Damp/Depth 0..100 and Rate in hundredths of a Hz.
   Clouds keeps its existing millisecond decay and motion-rate units. */
typedef struct {unsigned time,size,damping,speed,depth;} pw_reverb_settings;
pw_reverb_settings pw_reverb_defaults(unsigned model);
const char *pw_reverb_model_name(unsigned model);
typedef struct {float y0,y1,coefficient;} pw_cloud_lfo;
typedef struct {
 float *mem;
 uint32_t write_ptr,used,clear_left;
 unsigned model,size,time,damping,speed,moddepth;
 unsigned rev_ap_pos[2];
 pw_cloud_lfo lfo[2];
 float lfo_value[2];
 float hp[2],lp[2];
 float diffusion,lowpass,regen,depth;
 unsigned controls_dirty;
 gp5_reverb gp5;
} pw_reverb;
unsigned pw_reverb_configure(pw_reverb*,float*,unsigned,unsigned,unsigned,unsigned);
void pw_reverb_space(pw_reverb*,unsigned);
void pw_reverb_controls(pw_reverb*,unsigned,unsigned,unsigned,unsigned);
int pw_reverb_clear(pw_reverb*,float*,unsigned);
void pw_reverb_tick(pw_reverb*,float,float[2]);
