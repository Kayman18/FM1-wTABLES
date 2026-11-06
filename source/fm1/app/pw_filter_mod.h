#pragma once
/* FM1 modulation integration: primary ADSR adds semitones to cutoff.
 * The filter recurrence itself remains the recovered TEST5 implementation. */
void pw_filter_mod_init(void);
/* Built by pw_filter_mod_init(); read-only afterwards.  Exposed so the
   interpolation below can be inlined into the per-sample voice path. */
extern float pw_filter_mod_g_table[128];
static inline __attribute__((always_inline)) float pw_filter_mod_index(float base,float depth,float envelope){
 if(depth<0.0f)depth=0.0f;else if(depth>127.0f)depth=127.0f;
 if(envelope<0.0f)envelope=0.0f;else if(envelope>1.0f)envelope=1.0f;
 float index=base+depth*envelope;
 return index<0.0f?0.0f:index>127.0f?127.0f:index;
}
static inline __attribute__((always_inline)) float pw_filter_mod_g(float index){
 if(index<=0.0f)return pw_filter_mod_g_table[0];if(index>=127.0f)return pw_filter_mod_g_table[127];
 unsigned i=(unsigned)index;float fraction=index-(float)i;
 return pw_filter_mod_g_table[i]+(pw_filter_mod_g_table[i+1]-pw_filter_mod_g_table[i])*fraction;
}

float pw_filter_mod_signed(float base,float tracking,float amp_depth,float amp,float env2_depth,float env2,float lfo_depth,float lfo);
