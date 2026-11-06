#include "pw_filter.h"
#include "fastmath.h"
/* Recovered Poly Wavetable state-variable filter; see FILTER.md.
 * Source uses fused operations. FM1 separates them; no bit-exact claim. */
float pw_filter_hz(unsigned index){
 if(index>127)index=127;
 float hz=440.0f*fm_expf(((float)index-69.0f)*(FM_LN2/12.0f));
 /* Source's final table entry stops just below Fs/4. */
 float ceiling=44100.0f*(11999.0f/48000.0f);
 return hz<ceiling?hz:ceiling;
}
float pw_filter_g(unsigned index){return 2.0f*fm_sinf((FM_PI/44100.0f)*pw_filter_hz(index));}
float pw_filter_q(unsigned index){if(index>100)index=100;return -1.0f/(0.5099999904632568f+(float)index*0.03200000151991844f);}
/* pw_filter_reset, pw_filter_damping, pw_filter_tick, pw_filter_output and
   pw_filter_live are inlined in pw_filter.h; see the note there. */
