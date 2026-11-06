#include "pw_tables.h"
#include "pw_table_layout.h"
const int16_t *pw_table_mips[10][8];
static int ready;
#ifdef PW_TABLES_EMBEDDED
/* Reference-only host DSP tests. Never enabled by fm1.toml. */
#include "pw_mip_tables.h"
void pw_tables_init(void){for(unsigned m=0;m<10;m++)for(unsigned b=0;b<8;b++)pw_table_mips[m][b]=pw_mip_banks[m][b];ready=1;}
#else
#include "plat.h"
static uint32_t read32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t crc32(const uint8_t *p,unsigned n){
 static const uint32_t t[16]={0,0x1db71064u,0x3b6e20c8u,0x26d930acu,0x76dc4190u,0x6b6b51f4u,0x4db26158u,0x5005713cu,0xedb88320u,0xf00f9344u,0xd6d6a3e8u,0xcb61b38cu,0x9b64c2b0u,0x86d3d2d4u,0xa00ae278u,0xbdbdf21cu};
 uint32_t c=0xffffffffu;while(n--){c^=*p++;c=(c>>4)^t[c&15];c=(c>>4)^t[c&15];}return ~c;
}
void pw_tables_init(void){
 static const uint32_t offsets[10]=PW_TABLE_OFFSETS,lengths[10]=PW_TABLE_LENGTHS;
 static const uint8_t magic[8]={'P','W','M','T','B','L','0','1'};
 ready=0;for(unsigned m=0;m<10;m++)for(unsigned b=0;b<8;b++)pw_table_mips[m][b]=0;
 /* Both endpoints are checked before any header/payload access. The platform
  * guarantees a contiguous read-only XIP range; it configures plaintext before app_init. */
 const uint8_t *p=plat_data(PW_TABLE_FLASH_START);
 const uint8_t *last=plat_data(PW_TABLE_FLASH_START+PW_TABLE_BYTES-1u);
 if(!p||!last||last!=p+PW_TABLE_BYTES-1u||((uintptr_t)p&3u))return;
 for(unsigned i=0;i<8;i++)if(p[i]!=magic[i])return;
 if(read32(p+8)!=1||read32(p+12)!=PW_TABLE_HEADER_BYTES||read32(p+16)!=PW_TABLE_BYTES-PW_TABLE_HEADER_BYTES||read32(p+20)!=PW_TABLE_PAYLOAD_CRC||read32(p+24)!=8||read32(p+28)!=8||read32(p+32)!=10||read32(p+36)!=2)return;
 for(unsigned m=0;m<10;m++)if(read32(p+40+4*m)!=offsets[m]||read32(p+80+4*m)!=lengths[m])return;
 for(unsigned i=120;i<252;i++)if(p[i])return;
 if(read32(p+252)!=crc32(p,252)||crc32(p+PW_TABLE_HEADER_BYTES,PW_TABLE_BYTES-PW_TABLE_HEADER_BYTES)!=PW_TABLE_PAYLOAD_CRC)return;
 for(unsigned m=0;m<10;m++)for(unsigned b=0;b<8;b++)pw_table_mips[m][b]=(const int16_t *)(p+offsets[m]+b*8u*lengths[m]*2u);
 ready=1;
}
#endif
int pw_tables_ready(void){return ready;}
