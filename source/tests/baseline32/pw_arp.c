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

unsigned pw_arp_random_next(pw_arp_random *s,unsigned count,unsigned mode,unsigned octaves){
 if(!count||count>16||mode<8||mode>10||octaves<1||octaves>3)return 0;
 unsigned total=count*octaves,index=0;
 if(total>1){
  s->seed=s->seed*0x02faa12du+0x00ec4babu;
  unsigned random=s->seed>>16;
  if(mode==8)index=random%total;
  else {
   uint8_t candidates[48];unsigned n=0;
   for(unsigned i=0;i<total;i++){
    unsigned excluded=mode==9?(i==s->previous):((s->used[i>>5]>>(i&31))&1u);
    if(!excluded)candidates[n++]=(uint8_t)i;
   }
   if(n)index=candidates[random%n];
   else {s->used[0]=s->used[1]=0;index=random%total;}
  }
 }
 if(mode!=8&&total>1)s->used[index>>5]|=1u<<(index&31);
 s->previous=(uint8_t)index;return index;
}
