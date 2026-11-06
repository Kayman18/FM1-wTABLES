#include "pw_lfo.h"
#include "plat.h"
#include "pw_lfo_tables.h"
float pw_lfo_hz(int rate){if(rate>100)rate=100;if(rate< -100)rate= -100;return pw_lfo_rates[rate<0?-rate:rate];}
int32_t pw_lfo_increment(int rate){if(rate>100)rate=100;if(rate< -100)rate= -100;return rate<0?-pw_lfo_inc44[-rate]:pw_lfo_inc44[rate];}
PLAT_HOT float pw_lfo_value(const pw_lfo *s){return pw_lfo_sine[s->phase>>22];}
PLAT_HOT float pw_lfo_tick(pw_lfo *s,int32_t increment){s->phase+=(uint32_t)increment;return pw_lfo_value(s);}
PLAT_HOT int32_t pw_lfo_pitch_scale(float cents){
 if(cents>100.0f)cents=100.0f;else if(cents< -100.0f)cents= -100.0f;
 float z=cents*0.0005776226504666211f;
 float ratio=1.0f+z*(1.0f+z*(0.5f+z*(1.0f/6.0f+z/24.0f)));
 return (int32_t)(4096.0f*ratio+0.5f);
}
