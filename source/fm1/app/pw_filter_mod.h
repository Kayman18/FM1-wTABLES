#pragma once
/* FM1 modulation integration: primary ADSR adds semitones to cutoff.
 * The filter recurrence itself remains the recovered TEST5 implementation. */
void pw_filter_mod_init(void);
float pw_filter_mod_index(float base,float depth,float envelope);
float pw_filter_mod_g(float index);

float pw_filter_mod_signed(float base,float tracking,float amp_depth,float amp,float env2_depth,float env2,float lfo_depth,float lfo);
