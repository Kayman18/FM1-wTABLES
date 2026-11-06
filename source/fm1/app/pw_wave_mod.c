#include "pw_wave_mod.h"
#include "pw_table_layout.h"
int32_t pw_wave_mod_position(int32_t base,float amp_depth,float amp_env,float env2_depth,float env2,float lfo_depth,float lfo){
 float position=(float)base+(amp_depth*amp_env+env2_depth*env2+lfo_depth*lfo)*((float)PW_FRAME_POS_MAX/100.0f);
 if(position<0.0f)position=0.0f;else if(position>(float)PW_FRAME_POS_MAX)position=(float)PW_FRAME_POS_MAX;
 return (int32_t)position;
}
