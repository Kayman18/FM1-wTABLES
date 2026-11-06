#include "pw_tables.h"
#include "pw_table_layout.h"
#include "plat.h"
static int16_t cache[16368];
const int16_t *pw_table_mips[10][8];
static const uint8_t *file,*cursor,*end;
static unsigned ready,busy,bank,loading,count;
static uint32_t checksum;
static uint32_t read32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t crc_step(uint32_t c,const uint8_t *p,unsigned n){
 static const uint32_t t[16]={0,0x1db71064u,0x3b6e20c8u,0x26d930acu,0x76dc4190u,0x6b6b51f4u,0x4db26158u,0x5005713cu,0xedb88320u,0xf00f9344u,0xd6d6a3e8u,0xcb61b38cu,0x9b64c2b0u,0x86d3d2d4u,0xa00ae278u,0xbdbdf21cu};
 while(n--){c^=*p++;c=(c>>4)^t[c&15];c=(c>>4)^t[c&15];}return c;
}
static int block(void){
 unsigned n=16368-count;if(n>32)n=32;
 if(cursor>=end)return -1;
 unsigned h=*cursor++,mode=h&3,width=h>>2;
 if(mode==3||width>18||(!mode&&width))return -1;
 unsigned size=mode?mode*2+((n-mode)*width+7)/8:n*2;
 if((unsigned)(end-cursor)<size)return -1;
 unsigned seeds=mode?mode:n;int16_t *dst=cache+count;
 for(unsigned i=0;i<seeds;i++){unsigned x=cursor[2*i]|(unsigned)cursor[2*i+1]<<8;dst[i]=(int16_t)(x<32768?(int)x:(int)x-65536);}
 if(mode){
  const uint8_t *p=cursor+2*mode;
  /* Little-endian bit reservoir: only bytes within this block are consumed. */
  uint32_t bits=0;unsigned have=0;
  for(unsigned i=mode;i<n;i++){
   while(have<width){bits|=(uint32_t)*p++<<have;have+=8;}
   uint32_t zz=bits&((1u<<width)-1u);bits>>=width;have-=width;
   int32_t residual=(int32_t)(zz>>1);if(zz&1)residual=-residual-1;
   int32_t value=residual+dst[i-1];if(mode==2)value+=(int32_t)dst[i-1]-dst[i-2];
   if(value< -32768||value>32767)return -1;dst[i]=(int16_t)value;
  }
 }
 checksum=crc_step(checksum,(const uint8_t *)dst,n*2);cursor+=size;count+=n;
 return 0;
}
static void begin(unsigned b){loading=b;count=0;checksum=0xffffffffu;cursor=file+read32(file+16+16*b);end=cursor+read32(file+20+16*b);busy=1;}
void pw_tables_service(unsigned desired,int silent){
 if(!ready||desired>7||!silent)return;
 if((busy&&desired!=loading)||(!busy&&desired!=bank))begin(desired);
 if(!busy)return;
 for(unsigned i=0;i<16&&count<16368;i++)if(block()){ready=0;busy=0;return;}
 if(count==16368){if(cursor!=end||~checksum!=read32(file+24+16*loading)){ready=0;busy=0;return;}bank=loading;busy=0;}
}
void pw_tables_init(void){
 ready=busy=0;bank=255;file=plat_data(PW_TABLE_FLASH_START);
 if(!file||plat_data(PW_TABLE_FLASH_START+PW_TABLE_BYTES-1)!=file+PW_TABLE_BYTES-1)return;
 static const uint8_t magic[8]={'P','W','L','C','4','8','0','1'};
 for(unsigned i=0;i<8;i++)if(file[i]!=magic[i])return;
 if(read32(file+8)!=1||read32(file+12)!=16368||read32(file+144)!=PW_TABLE_BYTES||~crc_step(0xffffffffu,file,PW_TABLE_BYTES)!=PW_TABLE_FILE_CRC)return;
 unsigned last=256;
 for(unsigned b=0;b<8;b++){unsigned off=read32(file+16+16*b),size=read32(file+20+16*b);if(off!=last||size>PW_TABLE_BYTES-off)return;last=off+size;}
 if(last!=PW_TABLE_BYTES)return;
 unsigned offset=0;
 for(unsigned m=0;m<10;m++){for(unsigned b=0;b<8;b++)pw_table_mips[m][b]=cache+offset;offset+=8*(1024u>>m);}
 ready=1;begin(0);while(ready&&busy)pw_tables_service(0,1);
}
int pw_tables_ready(void){return ready;}
int pw_tables_busy(void){return busy;}
unsigned pw_tables_bank(void){return bank;}
