#include "pw_filter_mod.h"
#include "pw_filter.h"
static float g_table[128];
void pw_filter_mod_init(void){for(unsigned i=0;i<128;i++)g_table[i]=pw_filter_g(i);}
float pw_filter_mod_index(float base,float depth,float envelope){
 if(depth<0.0f)depth=0.0f;else if(depth>127.0f)depth=127.0f;
 if(envelope<0.0f)envelope=0.0f;else if(envelope>1.0f)envelope=1.0f;
 float index=base+depth*envelope;
 return index<0.0f?0.0f:index>127.0f?127.0f:index;
}
float pw_filter_mod_g(float index){
 if(index<=0.0f)return g_table[0];if(index>=127.0f)return g_table[127];
 unsigned i=(unsigned)index;float fraction=index-(float)i;
 return g_table[i]+(g_table[i+1]-g_table[i])*fraction;
}

/* Source lane 0x9d029c60..0x9d029cdc: sum signed routes before clamping. */
float pw_filter_mod_signed(float base,float tracking,float amp_depth,float amp,float env2_depth,float env2,float lfo_depth,float lfo){
 float index=base+tracking;index+=amp_depth*amp;index+=env2_depth*env2;index+=lfo_depth*lfo;
 return index<0.0f?0.0f:index>127.0f?127.0f:index;
}
