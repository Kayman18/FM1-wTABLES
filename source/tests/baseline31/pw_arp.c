#include "pw_arp.h"
unsigned pw_arp_next(uint8_t *step,unsigned count,unsigned mode,unsigned octaves){
 if(!count||count>16||mode<1||mode>7||octaves<1||octaves>3)return 0;
 unsigned total=count*octaves,period=total;
 if(mode==3&&count>1)period=2*(total-1);
 if(mode==4)period=2*total;
 if((mode==5||mode==6)&&total>1)period=2*(total-1);
 unsigned p=*step==255?0:((unsigned)*step+1)%period;*step=(uint8_t)p;
 if(mode==2)return total-1-p;
 if(mode==5)return (p&1)?p/2+1:0;
 if(mode==6)return (p&1)?total-2-p/2:total-1;
 if(mode==3&&count>1)return p<total?p:period-p;
 if(mode==4)return p<total?p:period-1-p;
 return p;
}
uint32_t pw_arp_interval(unsigned bpm,uint32_t *remainder){
 if(bpm<40)bpm=40;if(bpm>240)bpm=240;
 unsigned divisor=bpm*2;uint32_t sum=2646000u+*remainder;
 *remainder=sum%divisor;return sum/divisor;
}
