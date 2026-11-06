#include "pw_lfo_phase.h"
uint32_t pw_lfo_spread_phase(unsigned slot,unsigned degrees){
 if(degrees>90)degrees=90;
 /* Source shift/add constant, deliberately retaining its integer rounding. */
 return ((uint32_t)slot*(uint32_t)degrees)*11930464u;
}
void pw_lfo_assign_phase(uint32_t *phases,unsigned count,unsigned slot,unsigned mode,unsigned degrees,int any_held,int fresh){
 if(!count||slot>=count)return;
 uint32_t offset=pw_lfo_spread_phase(slot,degrees),anchor=phases[0];
 if(mode==PW_LFO_KEEP){if(fresh)phases[slot]=offset;return;}
 if(mode==PW_LFO_POLY){phases[slot]=offset;return;}
 if(mode==PW_LFO_MONO&&!any_held){for(unsigned i=0;i<count;i++)phases[i]=pw_lfo_spread_phase(i,degrees);return;}
 phases[slot]=anchor+offset;
}
