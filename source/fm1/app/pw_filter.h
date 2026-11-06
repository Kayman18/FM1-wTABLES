#pragma once
#include <stdint.h>
enum { PW_FILTER_OFF, PW_FILTER_LP, PW_FILTER_BP, PW_FILTER_HP };
typedef struct { float low,band,high; } pw_filter;
typedef struct {uint8_t mode,cutoff,resonance,mix;} pw_filter_params;
float pw_filter_hz(unsigned index);
float pw_filter_g(unsigned index);
float pw_filter_q(unsigned index);
/* Per-sample, per-voice: inlined so the state stays in registers across the
   recurrence and its two accessors.  Arithmetic and order are unchanged. */
static inline __attribute__((always_inline)) void pw_filter_reset(pw_filter *s){*s=(pw_filter){0};}
static inline __attribute__((always_inline)) float pw_filter_damping(float g,float q){float bound=g*1.4142135381698608f-2.700000047683716f;return q>bound?q:bound;}
static inline __attribute__((always_inline)) void pw_filter_tick(pw_filter *s,float input,float g,float damping){
 float high=input+(s->band*damping-s->low);
 float band=s->band+high*g;
 float low=s->low+g*band;
 s->high=high;s->band=band;s->low=low;
}
static inline __attribute__((always_inline)) float pw_filter_output(const pw_filter *s,unsigned mode,float dry){return mode==PW_FILTER_LP?s->low:mode==PW_FILTER_BP?s->band:mode==PW_FILTER_HP?s->high:dry;}
static inline __attribute__((always_inline)) int pw_filter_live(const pw_filter *s){
 float l=s->low,b=s->band;
 if(l<0.0f)l=-l;if(b<0.0f)b=-b;
 return l>0.000001f||b>0.000001f;
}
