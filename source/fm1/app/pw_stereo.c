#include "pw_stereo.h"
#include "fastmath.h"
void pw_stereo_gains(unsigned mode,int spread,unsigned slot,unsigned count,int note,float *left,float *right){
 if(!mode||mode>3){*left=*right=1.0f;return;}
 if(spread< -100)spread= -100;if(spread>100)spread=100;if(count<2)count=2;if(count>8)count=8;if(slot>=count)slot=count-1;
 float x=0.5f,amount=(float)spread*0.009999999776482582f;
 if(mode==1)x+=amount*((float)slot*(count==8?0.1428571492433548f:1.0f/(float)(count-1))-0.5f);
 else if(mode==2){float offset=(float)(slot/2+1)/(float)count;if(slot&1)offset=-offset;x+=amount*offset;}
 else x+=(float)(note-48)*((float)spread*0.00013888889225199819f);
 if(x<0.0f)x=0.0f;else if(x>1.0f)x=1.0f;
 *left=fm_sqrtf(1.0f-x);*right=fm_sqrtf(x);
}
