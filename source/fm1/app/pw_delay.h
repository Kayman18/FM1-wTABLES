#pragma once
#include <stdint.h>
/* TEST60 Rikoshet Lite: stereo 44.1 kHz I/O, 22.05 kHz float processing.
   17664 stereo frames = 141312 bytes = 138 KiB shared with reverb.
   One 750 ms tap plus ~51 ms of timing/modulation headroom. */
#define PW_DELAY_FRAMES 17664u
#define PW_DELAY_DSP_RATE 22050.0f
#define PW_DELAY_MAX_MS 750u
typedef struct {
 float (*ring)[2]; uint32_t length,write,valid;
 float current,next;unsigned fade,phase;
 float in_hist[2][7],wet[2],previous_wet[2];
 float lp[2],hp[2],lowcoef,highcoef,spread,cross;
 uint16_t lohz,hihz;uint8_t spread_pct,cross_pct;
} pw_delay;
void pw_delay_route(const float history[4],float wet[4],unsigned mode);
void pw_delay_frame(const float input[4],const float history[4],float written[4],float output[4],unsigned mode,float feedback,float level);
void pw_delay_init(pw_delay *s,float (*memory)[2],unsigned frames);
void pw_delay_reset(pw_delay *s,float samples);
void pw_delay_controls(pw_delay *s,unsigned spread,unsigned cross,unsigned lowcut,unsigned highcut);
void pw_delay_tick(pw_delay *s,const float input[2],float output[2],unsigned mode,float samples,float feedback,float mix);
void pw_delay_tick_source(pw_delay *s,const float input[2],float output[2],unsigned mode,float samples,float feedback,float level);
/* Internal sequencer tempo shared by Bloom and delay, not external MIDI clock. */
float pw_delay_sync_ms(unsigned bpm,unsigned division,unsigned *effective);
const char *pw_delay_division_name(unsigned division);
