#include "pw_filter.h"
#include "fastmath.h"
/* Recovered Poly Wavetable state-variable filter; see FILTER.md.
 * Source uses fused operations. FM1 separates them; no bit-exact claim. */
void pw_filter_reset(pw_filter *s){*s=(pw_filter){0};}
float pw_filter_hz(unsigned index){
 if(index>127)index=127;
 float hz=440.0f*fm_expf(((float)index-69.0f)*(FM_LN2/12.0f));
 /* Source's final table entry stops just below Fs/4. */
 float ceiling=44100.0f*(11999.0f/48000.0f);
 return hz<ceiling?hz:ceiling;
}
float pw_filter_g(unsigned index){return 2.0f*fm_sinf((FM_PI/44100.0f)*pw_filter_hz(index));}
float pw_filter_q(unsigned index){if(index>100)index=100;return -1.0f/(0.5099999904632568f+(float)index*0.03200000151991844f);}
float pw_filter_damping(float g,float q){float bound=g*1.4142135381698608f-2.700000047683716f;return q>bound?q:bound;}
void pw_filter_tick(pw_filter *s,float input,float g,float damping){
 float high=input+(s->band*damping-s->low);
 float band=s->band+high*g;
 float low=s->low+g*band;
 s->high=high;s->band=band;s->low=low;
}
float pw_filter_output(const pw_filter *s,unsigned mode,float dry){return mode==PW_FILTER_LP?s->low:mode==PW_FILTER_BP?s->band:mode==PW_FILTER_HP?s->high:dry;}
int pw_filter_live(const pw_filter *s){return fm_fabsf(s->low)>0.000001f||fm_fabsf(s->band)>0.000001f;}
