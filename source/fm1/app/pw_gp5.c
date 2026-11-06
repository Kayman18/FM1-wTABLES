#include "gp5_reverb.h"

#include <string.h>

#ifndef GP5_REVERB_QUANTIZED_REFERENCE
#define GP5_REVERB_QUANTIZED_REFERENCE 0
#endif
#ifndef GP5_REVERB_FLUSH_TINY
#define GP5_REVERB_FLUSH_TINY (!GP5_REVERB_QUANTIZED_REFERENCE)
#endif

/* All constants are single precision. Build with FP contraction disabled when
 * checking the reference path: firmware VMLA uses separate multiply/add rounds.
 */
static const uint16_t feedback_length[8] = {
  241u, 1321u, 701u, 1423u, 223u, 1217u, 857u, 1163u
};
static const uint16_t diffuser_length[2] = {223u, 281u};

/* log(-prototype_gain), evaluated by the original firmware logf. Caching these
 * avoids eight log calls on a control change. The exact shared gain rule is
 * -exp(8*log(-prototype)/T), T=0.1+0.002*decay_control^2.
 */
static const float prototype_log[8] = {
  -0.008095339871942997f, -0.077332302927970886f,
  -0.04293157160282135f, -0.071644552052021027f,
  -0.014235007576644421f, -0.060203436762094498f,
  -0.038094110786914825f, -0.050314702093601227f
};

static const float sine_table[65] = {
  0.0f, 0.0980171412229538f, 0.19509032368659973f,
  0.290284663438797f, 0.3826834261417389f, 0.4713967442512512f,
  0.5555702447891235f, 0.6343932747840881f, 0.7071067690849304f,
  0.7730104327201843f, 0.8314695954322815f, 0.8819212913513184f,
  0.9238795042037964f, 0.9569403529167175f, 0.9807852506637573f,
  0.9951847195625305f, 1.0f, 0.9951847195625305f,
  0.9807852506637573f, 0.9569403529167175f, 0.9238795042037964f,
  0.8819212913513184f, 0.8314695954322815f, 0.7730104327201843f,
  0.7071067690849304f, 0.6343932747840881f, 0.5555702447891235f,
  0.4713967442512512f, 0.3826834261417389f, 0.290284663438797f,
  0.19509032368659973f, 0.0980171412229538f, 0.0f,
  -0.0980171412229538f, -0.19509032368659973f,
  -0.290284663438797f, -0.3826834261417389f, -0.4713967442512512f,
  -0.5555702447891235f, -0.6343932747840881f, -0.7071067690849304f,
  -0.7730104327201843f, -0.8314695954322815f, -0.8819212913513184f,
  -0.9238795042037964f, -0.9569403529167175f, -0.9807852506637573f,
  -0.9951847195625305f, -1.0f, -0.9951847195625305f,
  -0.9807852506637573f, -0.9569403529167175f, -0.9238795042037964f,
  -0.8819212913513184f, -0.8314695954322815f, -0.7730104327201843f,
  -0.7071067690849304f, -0.6343932747840881f, -0.5555702447891235f,
  -0.4713967442512512f, -0.3826834261417389f, -0.290284663438797f,
  -0.19509032368659973f, -0.0980171412229538f, -0.0f
};

typedef struct {
  const char *name;
  float decay_base, decay_scale, damp;
  float lp, lp_resonance, hp, hp_resonance, mod_depth, mod_rate;
} model_parameters;

static const model_parameters models[GP5_REVERB_MODEL_COUNT] = {
  {"Air",         10.0f, 0.1f,   0.0f,100.0f, 0.0f, 0.0f, 0.0f,  0.0f, 10.0f},
  {"Room",        10.0f, 0.2f,   0.0f, 88.0f,40.0f,10.0f,20.0f,  0.0f, 10.0f},
  {"Hall",        20.0f, 0.4f,  70.0f,100.0f, 0.0f,18.0f,25.0f,  0.0f, 10.0f},
  {"Church",      30.0f, 0.5f,  80.0f, 68.0f,20.0f, 0.0f, 0.0f,  0.0f, 10.0f},
  {"Plate L",     25.0f, 0.5f,   0.0f, 80.0f, 0.0f,40.0f,13.0f,100.0f, 13.0f},
  {"Plate",       20.0f, 0.4f,   0.0f, 86.0f,56.0f,40.0f,25.0f,  0.0f, 10.0f},
  {"Spring",      30.0f, 0.4f,  14.0f, 65.0f, 0.0f,35.0f, 7.0f,  7.0f,100.0f},
  {"N-Star",      20.0f, 0.4f,   0.0f,100.0f, 0.0f,35.0f,10.0f,  0.0f, 10.0f},
  {"Deepsea",     50.0f, 0.3f, 100.0f, 46.0f,13.0f, 0.0f, 0.0f,  0.0f, 10.0f},
  {"Sweet Space", 30.0f, 0.4f,   0.0f, 85.0f,20.0f,16.0f,20.0f, 20.0f, 20.0f}
};

static float clamp(float value, float low, float high) {
  if (!(value >= low)) return low; /* includes NaN */
  return value > high ? high : value;
}

static inline float flush_tiny(float value) {
#if GP5_REVERB_FLUSH_TINY
  /* IEEE-754 magnitude ordering replaces two sign-dependent FP branches.
   * 0x1e3ce508 is exactly the float representation of 1.0e-20f. Strictly
   * smaller magnitudes become +0; either threshold, infinities and NaN
   * payloads/signs are retained. Negative zero becomes +0, as before.
   */
  union { float value; uint32_t bits; } sample;
  uint32_t keep;
  sample.value = value;
  keep = (sample.bits & 0x7fffffffu) >= 0x1e3ce508u;
  sample.bits &= 0u - keep;
  return sample.value;
#else
  return value;
#endif
}

/* Firmware expf's ordinary finite-input branch, restricted to the negative
 * exponent range required by the supported decay controls. No libm or double
 * arithmetic, and called only when controls change, never by the sample loop.
 */
static float decay_exp(float value) {
  static const float hi[4] = {1.0f,1.18896484375f,1.4140625f,1.681640625f};
  static const float lo[4] = {0.0f,0.00024227125686593354f,
    0.00015106236969586462f,0.0001522055099485442f};
  static const float scale[4] = {1.0f,1.1892070770263672f,
    1.4142135381698608f,1.6817928552627563f};
  union { uint32_t bits; float value; } power;
  float rounded = value * 5.770780086517334f;
  int n, quarter, exponent;
  float fn, x, polynomial, result;
  rounded += value < 0.0f ? -0.5f : 0.5f;
  n = (int)rounded;
  quarter = (int)((unsigned)n & 3u);
  exponent = (n - quarter) / 4;
  fn = (float)n;
  x = value - fn * 0.17327880859375f;
  x -= fn * 7.986545824678615e-06f;
  polynomial = 0.16672924160957336f + x * 0.04168231040239334f;
  polynomial = 0.4999999701976776f + x * polynomial;
  polynomial = 0.9999999403953552f + x * polynomial;
  polynomial *= x;
  result = lo[quarter] + scale[quarter] * polynomial;
  result += hi[quarter];
  power.bits = (uint32_t)(exponent + 127) << 23;
  return power.value * result;
}

static float cutoff(float value) {
  float x = clamp(value * 0.01f, 0.0f, 1.0f);
  float frequency = x * 0.75f;
  frequency *= x;
  frequency += 0.25f;
  frequency *= x;
  return frequency < 0.005f ? 0.005f : frequency;
}

static void set_filter(gp5_reverb_svf *filter, float frequency, float resonance) {
  float loss = clamp(1.0f - resonance * 0.01f, 0.01f, 1.0f);
  filter->target_frequency = cutoff(frequency);
  filter->target_damping = loss * (2.0f - filter->target_frequency);
}

void gp5_reverb_set_parameters(gp5_reverb *s, const float parameters[11]) {
  float next[11], decay_time, d, rate;
  unsigned i;
  int changed = 0;
  if (!s || !parameters || !s->initialized) return;
  for (i = 0; i < 11u; ++i) {
    next[i] = clamp(parameters[i], 0.0f, i == 10u ? 1000.0f : 100.0f);
    if (next[i] != s->controls[i]) changed = 1;
  }
  if (!changed) return;
  memcpy(s->controls, next, sizeof(next));
  decay_time = next[1] * next[1];
  decay_time *= 0.0020000000949949026f;
  decay_time += 0.10000000149011612f;
  for (i = 0; i < 8u; ++i) {
    float exponent = prototype_log[i] / decay_time;
    exponent += exponent;
    exponent *= 4.0f;
    s->feedback_gain[i] = -decay_exp(exponent);
  }
  d = next[2] * 0.01f;
  s->damping_outer = (0.5f * d) / (1.0f + d);
  s->damping_center = 1.0f - (s->damping_outer + s->damping_outer);
  set_filter(&s->lowpass, next[3], next[4]);
  set_filter(&s->highpass, next[5], next[6]);
  s->modulation_depth = next[9] * 0.01f;
  rate = clamp(next[10] * 0.1f, 0.001f, 100.0f);
  s->modulation_step = rate * 6.2831854820251465f;
  s->modulation_step *= 2.2675736545352265e-05f;
  s->freeze_enabled = (uint8_t)(next[8] >= 0.5f);
}

const char *gp5_reverb_model_name(unsigned model) {
  return models[model < GP5_REVERB_MODEL_COUNT ? model : 0u].name;
}

void gp5_reverb_set_model(gp5_reverb *s, unsigned model,
                          float decay, float damp, float modulation) {
  float p[11];
  const model_parameters *m;
  if (!s) return;
  if (model >= GP5_REVERB_MODEL_COUNT) model = 0u;
  m = &models[model];
  decay = clamp(decay, 0.0f, 100.0f);
  damp = clamp(damp, 0.0f, 100.0f);
  modulation = clamp(modulation, 0.0f, 100.0f);
  p[0] = 100.0f;
  p[1] = m->decay_base + m->decay_scale * decay;
  p[2] = m->damp;
  if (model == GP5_REVERB_AIR || model == GP5_REVERB_SWEET_SPACE) p[2] = damp;
  else if (model == GP5_REVERB_PLATE) p[2] = 0.5f * damp;
  p[3] = m->lp; p[4] = m->lp_resonance;
  p[5] = m->hp; p[6] = m->hp_resonance;
  p[7] = 1.0f; p[8] = 0.0f;
  p[9] = m->mod_depth;
  if (model == GP5_REVERB_SWEET_SPACE) p[9] += 0.3f * modulation;
  p[10] = m->mod_rate;
  s->model = (uint8_t)model;
  gp5_reverb_set_parameters(s, p);
}

static void attach_delays(gp5_reverb *s) {
  unsigned offset = 0u, i;
  for (i = 0; i < 8u; ++i) {
    gp5_reverb_delay *line = &s->feedback[i];
    line->data = s->memory + offset;
    line->size = (uint16_t)(feedback_length[i] + 2u);
    /* Firmware reads nominal D-2, then applies its causal three-tap FIR. */
    line->tap = (uint16_t)(feedback_length[i] - 2u);
    offset += line->size;
  }
  for (i = 0; i < 2u; ++i) {
    gp5_reverb_delay *line = &s->diffuser[i];
    line->data = s->memory + offset;
    line->size = (uint16_t)(diffuser_length[i] + 2u);
    line->tap = diffuser_length[i];
    offset += line->size;
  }
  s->modulation_buffer = s->memory + offset;
}

void gp5_reverb_reset(gp5_reverb *s) {
  float controls[11];
  float *memory;
  unsigned model;
  if (!s || !s->initialized) return;
  memory = s->memory;
  model = s->model;
  memcpy(controls, s->controls, sizeof(controls));
  memset(s, 0, sizeof(*s));
  s->memory = memory;
  s->initialized = 1u;
  s->enabled = 1u;
  s->model = (uint8_t)model;
  s->excitation_gain = 1.0f;
  s->lowpass.frequency = s->lowpass.damping = 1.0f;
  s->highpass.frequency = s->highpass.damping = 1.0f;
  attach_delays(s);
  s->controls[0] = -1.0f; /* force derived-coefficient reconstruction */
  gp5_reverb_set_parameters(s, controls);
}

int gp5_reverb_init(gp5_reverb *s, float *memory, unsigned capacity) {
  if (!s || !memory || capacity < GP5_REVERB_DELAY_FLOATS) return 0;
  memset(s, 0, sizeof(*s));
  s->memory = memory;
  s->initialized = 1u;
  gp5_reverb_set_model(s, GP5_REVERB_AIR, 50.0f, 0.0f, 50.0f);
  gp5_reverb_reset(s);
  return 1;
}

void gp5_reverb_clear_memory(gp5_reverb *s) {
  if (s && s->initialized)
    memset(s->memory, 0, GP5_REVERB_DELAY_FLOATS * sizeof(float));
}

void gp5_reverb_set_enabled(gp5_reverb *s, int enabled) {
  if (s) s->enabled = (uint8_t)(enabled != 0);
}

static inline float stored_sample(float value, int rounded) {
#if GP5_REVERB_QUANTIZED_REFERENCE
  value *= 1638.4000244140625f;
  if (rounded) value += 0.5f;
  if (value >= 32767.0f) return 32767.0f;
  if (value <= -32768.0f) return -32768.0f;
  return (float)(int32_t)value;
#else
  (void)rounded;
  return flush_tiny(value);
#endif
}

static inline float ring_read(const gp5_reverb_delay *line) {
  unsigned position = line->position;
  position = position >= line->tap ? position - line->tap :
             position + line->size - line->tap;
  return line->data[position];
}

static inline void ring_write(gp5_reverb_delay *line, float value) {
  unsigned position = line->position;
  line->data[position] = stored_sample(value, 1);
  ++position;
  if (position == line->size) position = 0u;
  line->position = (uint16_t)position;
}

static inline float resample_allpass(gp5_reverb_allpass *state,
                                     float input, float coefficient) {
  float output = input - state->previous_output;
  output *= coefficient;
  output += state->previous_input;
  state->previous_input = input;
  state->previous_output = flush_tiny(output);
  return state->previous_output;
}

static void filter_ramp(gp5_reverb_svf *filter) {
  filter->frequency_step = clamp((filter->target_frequency - filter->frequency)
                                 * 0.03125f, -0.001f, 0.001f);
  filter->damping_step = clamp((filter->target_damping - filter->damping)
                               * 0.03125f, -0.001f, 0.001f);
  /* If float rounding makes an increment ineffective, retain the actual settled
   * value instead of snapping to a subtly different target. */
  if (filter->frequency + filter->frequency_step == filter->frequency)
    filter->frequency_step = 0.0f;
  if (filter->damping + filter->damping_step == filter->damping)
    filter->damping_step = 0.0f;
}

static void begin_block(gp5_reverb *s) {
  unsigned i;
  filter_ramp(&s->lowpass);
  filter_ramp(&s->highpass);
  if (s->freeze_enabled) {
    s->freeze_mix += 0.01f;
    if (s->freeze_mix > 1.0f) s->freeze_mix = 1.0f;
  } else {
    s->freeze_mix *= 0.99f;
  }
  for (i = 0; i < 8u; ++i) {
    float g = s->feedback_gain[i];
    g += (-1.0f - g) * s->freeze_mix;
    s->active_gain[i] = g * 0.7071067690849304f;
  }
  s->active_outer = s->freeze_enabled ? 0.0f : s->damping_outer;
  s->active_center = 1.0f - (s->active_outer + s->active_outer);
}

static float modulated_delay(gp5_reverb *s, float input) {
  float table_position = s->modulation_phase * 10.18591594696045f;
  int table_index = (int)table_position;
  float fraction = table_position - (float)table_index;
  unsigned k = (unsigned)table_index & 63u;
  float sine = sine_table[k] * (1.0f - fraction);
  float delay, result;
  unsigned write = s->modulation_position;
  int read, integral;
  sine += sine_table[k + 1u] * fraction;
  s->modulation_buffer[write] = stored_sample(input, 0);
  ++write;
  /* Exact firmware layout:159 live positions and a duplicated guard sample0.
   * The first startup write goes to0; subsequent wraps copy159 to0, then use1.
   */
  if (write >= 160u) {
    s->modulation_buffer[0] = s->modulation_buffer[159];
    write = 1u;
  }
  s->modulation_position = (uint16_t)write;
  delay = sine * 0.5f;
  delay *= s->modulation_depth;
  delay += 0.5f;
  delay *= 156.0f;
  delay += 2.0f;
  integral = (int)delay;
  fraction = delay - (float)integral;
  read = (int)write - integral;
  if (read <= 0) read += 159;
  result = (1.0f - fraction) * s->modulation_buffer[read];
  result += fraction * s->modulation_buffer[read - 1];
#if GP5_REVERB_QUANTIZED_REFERENCE
  result /= 1638.4000244140625f;
#endif
  s->modulation_phase += s->modulation_step;
  if (s->frame_in_block == 62u) {
    while (s->modulation_phase > 6.2831854820251465f)
      s->modulation_phase += -6.2831854820251465f;
  }
  return result;
}

static inline float diffuse(gp5_reverb_delay *line, float input) {
  float delayed = ring_read(line);
  float output, feedback;
#if GP5_REVERB_QUANTIZED_REFERENCE
  delayed /= 1638.4000244140625f;
#endif
  output = input * 0.699999988079071f;
  output += delayed;
  feedback = output * 0.699999988079071f;
  feedback -= input;
  ring_write(line, feedback);
  return output;
}

static inline float damped_read(gp5_reverb *s, unsigned index) {
  float input = ring_read(&s->feedback[index]);
  float outer, output;
#if GP5_REVERB_QUANTIZED_REFERENCE
  input *= 0.0006103515625f;
#endif
  outer = (s->fir_history[index][1] + input) * s->active_outer;
  output = s->active_center * s->fir_history[index][0];
  output += outer;
  s->fir_history[index][1] = s->fir_history[index][0];
  s->fir_history[index][0] = input;
  return output;
}

static void filter_stereo(gp5_reverb_svf *filter, float wet[2], int highpass) {
  unsigned channel;
  if (filter->frequency_step != 0.0f) filter->frequency += filter->frequency_step;
  if (filter->damping_step != 0.0f) filter->damping += filter->damping_step;
  for (channel = 0; channel < 2u; ++channel) {
    float band = filter->band[channel];
    float low = filter->low[channel] + filter->frequency * band;
    float high = wet[channel] - low;
    high -= band * filter->damping;
    band += filter->frequency * high;
    filter->low[channel] = flush_tiny(low);
    filter->band[channel] = flush_tiny(band);
    wet[channel] = highpass ? flush_tiny(high) : filter->low[channel];
  }
}

static void core_tick(gp5_reverb *s, float input, float wet[2]) {
  unsigned pair;
  input = modulated_delay(s, input);
  input = diffuse(&s->diffuser[0], input);
  input = diffuse(&s->diffuser[1], input);
  for (pair = 0; pair < 4u; ++pair) {
    unsigned a = pair * 2u, b = a + 1u;
    float u = damped_read(s, a) * s->active_gain[a];
    float v = damped_read(s, b) * s->active_gain[b];
    float left = u + v, right = v - u;
    ring_write(&s->feedback[a], input + left);
    ring_write(&s->feedback[b], input + right);
    if (pair == 0u) { wet[0] = left; wet[1] = right; }
    else { wet[0] += left; wet[1] += right; }
  }
  filter_stereo(&s->lowpass, wet, 0);
  filter_stereo(&s->highpass, wet, 1);
}

void gp5_reverb_tick(gp5_reverb *s, float input, float wet[2]) {
  float excited;
  if (!wet) return;
  if (!s || !s->initialized) { wet[0] = wet[1] = 0.0f; return; }
  if (s->frame_in_block == 0u) begin_block(s);
  input = flush_tiny(input);
  if (s->enabled && s->controls[8] == 0.0f) {
    if (s->excitation_gain < 1.0f) {
      excited = input * s->excitation_gain;
      s->excitation_gain += 0.01f;
      if (s->excitation_gain > 1.0f) s->excitation_gain = 1.0f;
    } else excited = input;
  } else {
    excited = input * s->excitation_gain;
    s->excitation_gain = flush_tiny(s->excitation_gain * 0.99f);
  }
  if ((s->frame_in_block & 1u) == 0u) {
    float down, tank[2];
    unsigned channel;
    down = resample_allpass(&s->down_even[0], excited, 0.11447494477033615f);
    down = resample_allpass(&s->down_even[1], down, 0.7699431777000427f);
    down = (down + s->previous_down_odd) * 0.5f;
    core_tick(s, down, tank);
    for (channel = 0; channel < 2u; ++channel) {
      float even = resample_allpass(&s->up[channel][0][0], tank[channel],
                                     0.079866424202919f);
      float odd = resample_allpass(&s->up[channel][1][0], tank[channel],
                                    0.28382933139801025f);
      even = resample_allpass(&s->up[channel][0][1], even, 0.5453236699104309f);
      odd = resample_allpass(&s->up[channel][1][1], odd, 0.8344119191169739f);
      wet[channel] = even * 0.25f;
      s->pending_wet[channel] = odd * 0.25f;
    }
  } else {
    s->previous_down_odd = resample_allpass(&s->down_odd, excited,
                                            0.39783668518066406f);
    wet[0] = s->pending_wet[0];
    wet[1] = s->pending_wet[1];
  }
  s->frame_in_block = (uint8_t)((s->frame_in_block + 1u) & 63u);
}

void gp5_reverb_process(gp5_reverb *s, const float *mono,
                        float *wet_interleaved, unsigned frames) {
  unsigned i;
  if (!mono || !wet_interleaved) return;
  for (i = 0; i < frames; ++i)
    gp5_reverb_tick(s, mono[i], wet_interleaved + 2u * i);
}

void gp5_reverb_process_stereo(gp5_reverb *s, const float *input_interleaved,
                               float *wet_interleaved, unsigned frames) {
  unsigned i;
  if (!input_interleaved || !wet_interleaved) return;
  for (i = 0; i < frames; ++i) {
    float input = (input_interleaved[2u*i] + input_interleaved[2u*i+1u]) * 0.5f;
    gp5_reverb_tick(s, input, wet_interleaved + 2u*i);
  }
}
