#include "pw_unison.h"
int32_t pw_unison_detune(unsigned count,unsigned member,unsigned amount){
 if(count<2)return 0;if(count>8)count=8;if(member>=count)member=count-1;if(amount>100)amount=100;
 int32_t edge=(int32_t)amount*436;
 return (int32_t)(member*(unsigned)(edge*2)/(count-1))-edge;
}
float pw_unison_wave(unsigned slot,unsigned count,float spread){
 if(count<2)return 0.0f;if(count>8)count=8;if(slot>=count)slot=count-1;if(spread< -100.0f)spread= -100.0f;else if(spread>100.0f)spread=100.0f;
 float x=(float)slot*(count==8?0.1428571492433548f:1.0f/(float)(count-1))-0.5f;
 return x*(spread*0.009999999776482582f);
}
