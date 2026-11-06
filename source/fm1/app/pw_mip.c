#include "pw_mip.h"
#include "pw_mip_lut.h"
unsigned pw_mip_for_bucket(unsigned index,unsigned sample_rate){
 if(index>2999)index=2999;if(!index)return 0;
 unsigned period=sample_rate/(8u*(index+1u)),length=2048,shift=0;
 while(length>period&&shift<10){length>>=1;shift++;}
 return shift;
}
unsigned pw_mip_index(float hz){
 int f=(int)hz;if(f<1)f=1;if(f>24000)f=24000;
 return (unsigned)(f-1)>>3;
}
unsigned pw_mip_select(uint32_t increment,int32_t scale){
 float hz=(float)increment*(float)scale*(44100.0f/(4294967296.0f*4096.0f));
 return pw_mip_lut[pw_mip_index(hz)];
}
