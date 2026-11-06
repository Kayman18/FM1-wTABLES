#include "pw_tracking.h"
/* Recovered isolated tracking arithmetic; see TRACKING.md. Wave output is
 * a normalized bank fraction; cutoff output is a semitone offset. */
float pw_tracking_wave_offset(unsigned note,unsigned velocity,float velocity_depth,float pitch_depth){
 float velocity_product=(float)velocity*velocity_depth;
 float pitch_product=(float)((int)note-48)*pitch_depth;
 return velocity_product*0.00007812499825377017f+pitch_product*0.00007812499825377017f;
}
float pw_tracking_cutoff_offset(unsigned note,unsigned velocity,float velocity_depth,float pitch_depth){
 float velocity_product=(float)velocity*velocity_depth;
 float pitch_product=(float)((int)note-48)*pitch_depth;
 return velocity_product*0.0078125f+pitch_product*0.009999999776482582f;
}
