#ifndef GP5_REVERB_H
#define GP5_REVERB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Native, allocation-free GP-5 V1.0.3 reverb topology.  Input rate is 44100 Hz;
 * the delay tank runs at 22050 Hz.  All ten named models share this engine.
 * Production delay storage is float.  GP5_REVERB_QUANTIZED_REFERENCE=1 in the
 * .c compilation enables the firmware's int16 quantizers inside float memory
 * for offline validation; it is not the production performance configuration.
 */
#define GP5_REVERB_RATE 44100u
#define GP5_REVERB_DELAY_FLOATS 7830u
#define GP5_REVERB_MODEL_COUNT 10u
#define GP5_REVERB_CONTROL_COUNT 11u

enum gp5_reverb_model {
  GP5_REVERB_AIR, GP5_REVERB_ROOM, GP5_REVERB_HALL, GP5_REVERB_CHURCH,
  GP5_REVERB_PLATE_L, GP5_REVERB_PLATE, GP5_REVERB_SPRING,
  GP5_REVERB_N_STAR, GP5_REVERB_DEEPSEA, GP5_REVERB_SWEET_SPACE
};

typedef struct {
  float *data;
  uint16_t size, position, tap;
} gp5_reverb_delay;

typedef struct {
  float previous_input, previous_output;
} gp5_reverb_allpass;

typedef struct {
  float low[2], band[2];
  float frequency, damping;
  float target_frequency, target_damping;
  float frequency_step, damping_step;
} gp5_reverb_svf;

typedef struct {
  float *memory;
  gp5_reverb_delay feedback[8];
  gp5_reverb_delay diffuser[2];
  float *modulation_buffer;
  float controls[GP5_REVERB_CONTROL_COUNT];
  float feedback_gain[8], active_gain[8];
  float fir_history[8][2];
  float damping_outer, damping_center, active_outer, active_center;
  gp5_reverb_allpass down_even[2], down_odd;
  gp5_reverb_allpass up[2][2][2]; /* channel, output phase, allpass stage */
  gp5_reverb_svf lowpass, highpass;
  float previous_down_odd, pending_wet[2];
  float modulation_phase, modulation_step, modulation_depth;
  float excitation_gain, freeze_mix;
  uint16_t modulation_position;
  uint8_t frame_in_block, enabled, initialized, model;
  uint8_t freeze_enabled;
} gp5_reverb;

/* Initializes state and attaches memory, but DOES NOT write/clear delay memory.
 * The caller must zero GP5_REVERB_DELAY_FLOATS floats before the first tick.
 * This permits the FM1's existing incremental clear outside the audio IRQ.
 * Returns 1 on success, 0 for invalid pointers or insufficient capacity.
 */
int gp5_reverb_init(gp5_reverb *s, float *memory, unsigned capacity);

/* Reset histories/phase/filter ramps while retaining controls and the attached
 * memory.  Does not clear memory.  Caller must clear it before resuming ticks.
 */
void gp5_reverb_reset(gp5_reverb *s);

/* Optional synchronous full clear for startup/offline use, not an audio IRQ.
 * No effect on histories: call reset as well when starting a fresh effect.
 */
void gp5_reverb_clear_memory(gp5_reverb *s);

/* Shared firmware parameter order: Mix, decay control, Damp, LP cutoff,
 * LP resonance, HP cutoff, HP resonance, Trail, Freeze, Mod depth, Mod rate.
 * Percentage-like fields use their original 0..100 scale; rate is the original
 * raw field (actual Hz = raw*0.05 at 44100 Hz). Mix/Trail are retained as metadata:
 * this API returns wet only; the host owns the dry/wet mix and tail routing.
 * Call only at an audio-block/control boundary, never concurrently with tick.
 */
void gp5_reverb_set_parameters(gp5_reverb *s, const float parameters[11]);

/* UI controls Decay/Damp/Mod are 0..100. Model-specific mappings are applied;
 * unused controls have no effect. Mix100/Trail1/Freeze0 are selected.
 */
void gp5_reverb_set_model(gp5_reverb *s, unsigned model,
                          float decay, float damp, float modulation);
const char *gp5_reverb_model_name(unsigned model);

/* Stops or starts excitation with the firmware input fade; existing tail stays
 * available at the wet output. Host decides whether to route that tail.
 */
void gp5_reverb_set_enabled(gp5_reverb *s, int enabled);

/* One mono sample -> normalized stereo wet. Normalization is 0.25, matching
 * the firmware at Mix100 after its output ramp has settled. Call at 44100 Hz.
 * Even input frames run the core and emit the even polyphase output; odd frames
 * update the odd decimator path and emit the saved odd polyphase output.
 * No additional one-frame or block buffering latency is introduced.
 */
void gp5_reverb_tick(gp5_reverb *s, float input, float wet[2]);

/* Convenience wrappers. Sizes count input frames, not floats. Stereo input is
 * folded as (L+R)/2. Output is interleaved wet-only stereo. Buffers may overlap
 * only for process_stereo's exact in-place case; mono->stereo needs disjoint
 * buffers. Chunk size can be arbitrary, including odd, without losing phase.
 */
void gp5_reverb_process(gp5_reverb *s, const float *mono,
                        float *wet_interleaved, unsigned frames);
void gp5_reverb_process_stereo(gp5_reverb *s, const float *input_interleaved,
                               float *wet_interleaved, unsigned frames);

#ifdef __cplusplus
}
#endif
#endif
