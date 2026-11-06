#include "pw_tables.h"
#include "pw_table_layout.h"
#include "plat.h"
#include "pw_virus_data.h"
/* Single bank cache is in the on-chip audio pool, not scarce normal RAM. */
static int16_t cache[PW_BANK_SAMPLES] PLAT_POOL;
const int16_t *pw_table_mips[10][PW_TABLE_BANKS];
/* Both formats share the 512-byte header and 16-byte directory entries.
 * Version 1 is the original 16-frame geometry. Version 2 adds per-bank
 * (frame_count - 1) nibbles at 492: even bank low, odd bank high. */
enum { SAMPLES_PER_FRAME=2046u, DIRECTORY_BANKS=29u };
static uint8_t frame_counts[PW_TABLE_BANKS];
static const uint8_t *factory,*file,*cursor,*end;
static uint32_t expected_crc;
static unsigned ready,busy,bank,loading,count,decode_samples;
static uint32_t checksum;
static uint32_t read32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned file_frame_count(const uint8_t *p,unsigned local){
 return read32(p+8)==1u?16u:1u+((p[492u+local/2u]>>(4u*(local&1u)))&15u);
}
static uint32_t crc_step(uint32_t c,const uint8_t *p,unsigned n){
 static const uint32_t t[16]={0,0x1db71064u,0x3b6e20c8u,0x26d930acu,0x76dc4190u,0x6b6b51f4u,0x4db26158u,0x5005713cu,0xedb88320u,0xf00f9344u,0xd6d6a3e8u,0xcb61b38cu,0x9b64c2b0u,0x86d3d2d4u,0xa00ae278u,0xbdbdf21cu};
 while(n--){c^=*p++;c=(c>>4)^t[c&15];c=(c>>4)^t[c&15];}return c;
}
static int block(void){
 if(count>=decode_samples||decode_samples>PW_BANK_SAMPLES)return -1;
 unsigned n=decode_samples-count;if(n>32)n=32;
 if(cursor>=end)return -1;
 unsigned h=*cursor++,mode=h&3,width=h>>2;
 if(mode==3||mode>n||width>18||(!mode&&width))return -1;
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
static void begin(unsigned b){
 loading=b;count=0;decode_samples=(unsigned)frame_counts[b]*SAMPLES_PER_FRAME;checksum=0xffffffffu;
 unsigned local=b;if(b<PW_FACTORY_BANKS)file=factory;else{file=pw_virus_data;local-=PW_FACTORY_BANKS;}
 cursor=file+read32(file+16+16*local);end=cursor+read32(file+20+16*local);
 expected_crc=read32(file+24+16*local);busy=1;
}
void pw_tables_service_budget(unsigned desired,int silent,unsigned max_blocks){
 if(!ready||desired>=PW_TABLE_BANKS||!silent)return;
 if((busy&&desired!=loading)||(!busy&&desired!=bank))begin(desired);
 if(!busy)return;
 if(max_blocks<1u)max_blocks=1u;if(max_blocks>16u)max_blocks=16u;
 for(unsigned i=0;i<max_blocks&&count<decode_samples;i++)if(block()){ready=0;busy=0;return;}
 /* The selected bank (and thus its geometry) is published only once every
  * sample and the decoded CRC are valid, before any audio worker is started. */
 if(count==decode_samples){if(cursor!=end||~checksum!=expected_crc){ready=0;busy=0;return;}bank=loading;busy=0;}
}
void pw_tables_service(unsigned desired,int silent){pw_tables_service_budget(desired,silent,16u);}
static int validate(const uint8_t *p,unsigned bytes,uint32_t crc,unsigned banks){
 static const uint8_t magic[8]={'P','W','L','C','4','9','0','1'};
 if(!p||bytes<512||!banks||banks>DIRECTORY_BANKS)return 0;
 for(unsigned i=0;i<8;i++)if(p[i]!=magic[i])return 0;
 unsigned version=read32(p+8);
 if((version!=1u&&version!=2u)||read32(p+488)!=banks||read32(p+12)!=PW_BANK_SAMPLES||read32(p+480)!=bytes)return 0;
 if(~crc_step(0xffffffffu,p,508)!=read32(p+508)||~crc_step(0xffffffffu,p,bytes)!=crc)return 0;
 if(~crc_step(0xffffffffu,p+512,bytes-512)!=read32(p+484))return 0;
 unsigned last=512;
 for(unsigned b=0;b<banks;b++){
  unsigned frames=file_frame_count(p,b);
  if(frames<2u||frames>16u||frames*SAMPLES_PER_FRAME>PW_BANK_SAMPLES)return 0;
  unsigned off=read32(p+16+16*b),size=read32(p+20+16*b);
  if(off!=last||off>bytes||!size||size>bytes-off)return 0;
  if(~crc_step(0xffffffffu,p+off,size)!=read32(p+28+16*b))return 0;
  last=off+size;
 }
 return last==bytes;
}
void pw_tables_init(void){
 ready=busy=0;bank=255;decode_samples=count=0;factory=plat_data(PW_TABLE_FLASH_START);
 for(unsigned b=0;b<PW_TABLE_BANKS;b++){frame_counts[b]=0;for(unsigned m=0;m<10;m++)pw_table_mips[m][b]=0;}
 if(!factory||plat_data(PW_TABLE_FLASH_START+PW_TABLE_BYTES-1)!=factory+PW_TABLE_BYTES-1)return;
 if(!validate(factory,PW_TABLE_BYTES,PW_TABLE_FILE_CRC,PW_FACTORY_BANKS)||!validate(pw_virus_data,PW_VIRUS_BYTES,PW_VIRUS_CRC,PW_VIRUS_BANKS))return;
 for(unsigned b=0;b<PW_TABLE_BANKS;b++){
  unsigned frames=b<PW_FACTORY_BANKS?file_frame_count(factory,b):file_frame_count(pw_virus_data,b-PW_FACTORY_BANKS);
  frame_counts[b]=(uint8_t)frames;
  unsigned offset=0;
  for(unsigned m=0;m<10;m++){pw_table_mips[m][b]=cache+offset;offset+=frames*(1024u>>m);}
 }
 ready=1;begin(0);while(ready&&busy)pw_tables_service(0,1);
}
int pw_tables_ready(void){return ready;}
int pw_tables_busy(void){return busy;}
unsigned pw_tables_bank(void){return bank;}
unsigned pw_tables_frame_count(unsigned logical_bank){return ready&&logical_bank<PW_TABLE_BANKS?frame_counts[logical_bank]:0u;}
