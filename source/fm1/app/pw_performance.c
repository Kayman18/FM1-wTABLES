#include "pw_performance.h"
#include "pw_pitch_table.h"
int32_t pw_performance_units(float cents){
 if(cents< -16800.0f)cents= -16800.0f;else if(cents>16800.0f)cents=16800.0f;
 return (int32_t)(cents*436.90667724609375f);
}
/* Separate wide multiply from loads for JieLi; no 64-bit division. */
static __attribute__((noinline)) uint64_t product(uint32_t a,uint32_t b){return (uint64_t)a*b;}
int32_t pw_performance_increment(uint32_t base,int32_t units){
 if(units< -7340032)units= -7340032;else if(units>7340032)units=7340032;
 int exponent=units/524288;if(units<0&&units%524288)exponent--;
 unsigned index=((uint32_t)units>>9)&1023u;
 uint64_t increment=product(base,pw_pitch_mantissa[index])>>(unsigned)(18-exponent);
 if(increment>2147418112u)increment=2147418112u;
 return (int32_t)increment;
}
float pw_performance_bend(int value,unsigned semitones){
 if(value< -8192)value= -8192;if(value>8191)value=8191;if(semitones>48)semitones=48;
 return (float)value*((float)semitones*(100.0f/8192.0f));
}
