#include "pw_env_pitch.h"
#include "plat.h"
#include "pw_pitch_table.h"
PLAT_HOT int32_t pw_env_pitch_units(float env2,float amount,float lfo_cents){
 if(env2< -1.0f)env2= -1.0f;else if(env2>1.0f)env2=1.0f;
 if(amount< -120.0f)amount= -120.0f;else if(amount>120.0f)amount=120.0f;
 if(lfo_cents< -1200.0f)lfo_cents= -1200.0f;else if(lfo_cents>1200.0f)lfo_cents=1200.0f;
 float coefficient=amount*4369.06689453125f;
 return (int32_t)(coefficient*env2+lfo_cents*436.90667724609375f);
}
PLAT_HOT int32_t pw_env_pitch_scale(int32_t units){
 /* Relative-Q12 adaptation of the source exponent lookup. Bound the helper
  * to two octaves each way; the exposed combined range is only +/-13 st. */
 if(units< -1048576)units= -1048576;else if(units>1048576)units=1048576;
 int exponent=units/524288;if(units<0&&units%524288)exponent--;
 unsigned index=((uint32_t)units>>9)&1023u;
 return (int32_t)(pw_pitch_mantissa[index]>>(unsigned)(6-exponent));
}
