#include "pw_velocity.h"
#include "pw_velocity_tables.h"
/* 0x9d0ef12c: pure velocity gain, before the independent global dB gain. */
float pw_velocity_gain(unsigned velocity,float amount){
 if(velocity>127)velocity=127;
 if(amount<0.0f)amount=0.0f;if(amount>100.0f)amount=100.0f;
 return 1.0f+((float)velocity*.0078125f-1.0f)*(amount*.01f);
}
/* 0x9d0ee2e0, tables initialized by 0x9d168360. */
unsigned pw_velocity_map(unsigned velocity,unsigned gate,unsigned curve){
 if(!gate)return 0;if(curve==1)return 127;
 if(velocity>127)velocity=127;
 if(curve==2||curve==3)velocity=velocity_tables[curve-2][velocity];
 return velocity?velocity:1;
}
