#include "pw_filter_mod.h"
#include "plat.h"
#include "pw_filter.h"
float pw_filter_mod_g_table[128];
void pw_filter_mod_init(void){for(unsigned i=0;i<128;i++)pw_filter_mod_g_table[i]=pw_filter_g(i);}
/* pw_filter_mod_index and pw_filter_mod_g are inlined in pw_filter_mod.h. */

/* Source lane 0x9d029c60..0x9d029cdc: sum signed routes before clamping. */
PLAT_HOT float pw_filter_mod_signed(float base,float tracking,float amp_depth,float amp,float env2_depth,float env2,float lfo_depth,float lfo){
 float index=base+tracking;index+=amp_depth*amp;index+=env2_depth*env2;index+=lfo_depth*lfo;
 return index<0.0f?0.0f:index>127.0f?127.0f:index;
}
