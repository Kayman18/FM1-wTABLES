/* Mixed-bank loader regression. Including the production unit lets malformed
 * fixtures exercise its decoder without changing the read-only embedded data. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../fm1/app/pw_tables.c"

static uint8_t region[PW_TABLE_BYTES] __attribute__((aligned(4)));
const uint8_t *plat_data(uint32_t off){
 return off>=PW_TABLE_FLASH_START&&off-PW_TABLE_FLASH_START<PW_TABLE_BYTES?region+off-PW_TABLE_FLASH_START:0;
}
/* Independent bitwise CRC oracle, unlike the loader's nibble lookup. */
static uint32_t crc(const uint8_t *p,size_t n){
 uint32_t c=0xffffffffu;
 while(n--){c^=*p++;for(unsigned bit=0;bit<8;bit++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}
 return ~c;
}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static void header_crc(uint8_t *p){put32(p+508,crc(p,508));}
static void load_external(void){
 FILE *f=fopen("data/factory-mips.pwlc","rb");assert(f);
 assert(fread(region,1,sizeof region,f)==sizeof region);assert(fgetc(f)==EOF);fclose(f);
}
static void finish_bank(unsigned desired){
 for(unsigned calls=0;calls<1100&&(pw_tables_busy()||pw_tables_bank()!=desired);calls++){
  pw_tables_service_budget(desired,1,8);
  assert(pw_tables_ready());
 }
 assert(!pw_tables_busy()&&pw_tables_bank()==desired);
}
static void real_banks(void){
 assert(PW_FACTORY_BANKS==7&&PW_VIRUS_BANKS==18&&PW_TABLE_BANKS==25);
 assert(pw_tables_frame_count(0)==0);
 load_external();pw_tables_init();assert(pw_tables_ready()&&!pw_tables_busy()&&pw_tables_bank()==0);
 assert(pw_tables_frame_count(PW_TABLE_BANKS)==0);
 for(unsigned b=0;b<PW_TABLE_BANKS;b++){
  unsigned frames=b<14?16u:2u;
  assert(pw_tables_frame_count(b)==frames);
  finish_bank(b);
  unsigned offset=0;
  for(unsigned mip=0;mip<10;mip++){
   assert(pw_table_mips[mip][b]==cache+offset);
   unsigned width=1024u>>mip;
   for(unsigned frame=0;frame<frames;frame++){
    volatile int32_t edge=pw_table_mips[mip][b][frame*width]+pw_table_mips[mip][b][(frame+1)*width-1];(void)edge;
   }
   offset+=frames*width;
  }
  assert(offset==frames*2046u&&count==offset&&decode_samples==offset);
  assert(crc((const uint8_t *)cache,offset*2)==expected_crc);
 }
 finish_bank(0);
 uint32_t original=crc((const uint8_t *)cache,sizeof cache);
 pw_tables_service_budget(14,0,16);assert(!pw_tables_busy()&&pw_tables_bank()==0);
 assert(crc((const uint8_t *)cache,sizeof cache)==original);
 pw_tables_service_budget(14,1,0);assert(pw_tables_busy()&&pw_tables_bank()==0&&count==32);
 assert(pw_tables_frame_count(14)==2&&pw_tables_frame_count(0)==16);
 pw_tables_service_budget(14,1,999);assert(count==544&&pw_tables_bank()==0);
 /* Switching requests while a partial decode is muted restarts correctly. */
 finish_bank(1);finish_bank(14);finish_bank(0);
 assert(crc((const uint8_t *)cache,sizeof cache)==original);
 pw_tables_service(PW_TABLE_BANKS,1);assert(pw_tables_bank()==0&&!pw_tables_busy());
 /* Corruption present before init must fail closed and clear catalog access. */
 region[512]^=1;pw_tables_init();assert(!pw_tables_ready()&&!pw_tables_busy());
 assert(pw_tables_frame_count(0)==0);region[512]^=1;
}

static uint8_t *fixture;
static unsigned fixture_bytes;
static int16_t source_sample(unsigned b,unsigned i){return (int16_t)((int)((i*73u+b*1709u)%56001u)-28000);}
static void make_fixture(void){
 /* Two then sixteen frames, encoded as legal raw blocks with no padding. */
 const unsigned frames[2]={2,16};
 fixture_bytes=512;
 for(unsigned b=0;b<2;b++){unsigned n=frames[b]*2046u;fixture_bytes+=n*2u+(n+31u)/32u;}
 fixture=calloc(fixture_bytes+1u,1);assert(fixture);memcpy(fixture,"PWLC4901",8);
 put32(fixture+8,2);put32(fixture+12,PW_BANK_SAMPLES);put32(fixture+488,2);
 fixture[492]=0xf1;
 unsigned offset=512;
 for(unsigned b=0;b<2;b++){
  unsigned start=offset,n=frames[b]*2046u;uint8_t *raw=malloc(n*2u);assert(raw);
  for(unsigned i=0;i<n;i++){
   uint16_t sample=(uint16_t)source_sample(b,i);raw[2*i]=(uint8_t)sample;raw[2*i+1]=(uint8_t)(sample>>8);
  }
  for(unsigned i=0;i<n;){unsigned take=n-i;if(take>32)take=32;fixture[offset++]=0;memcpy(fixture+offset,raw+i*2,take*2);offset+=take*2;i+=take;}
  put32(fixture+16+16*b,start);put32(fixture+20+16*b,offset-start);
  put32(fixture+24+16*b,crc(raw,n*2));put32(fixture+28+16*b,crc(fixture+start,offset-start));free(raw);
 }
 assert(offset==fixture_bytes);put32(fixture+480,fixture_bytes);
 put32(fixture+484,crc(fixture+512,fixture_bytes-512));header_crc(fixture);
 assert(validate(fixture,fixture_bytes,crc(fixture,fixture_bytes),2));
}
static void reject_changed_field(unsigned offset,uint32_t value,unsigned width){
 uint8_t *p=malloc(fixture_bytes);assert(p);memcpy(p,fixture,fixture_bytes);
 if(width==1)p[offset]=(uint8_t)value;else put32(p+offset,value);
 header_crc(p);assert(!validate(p,fixture_bytes,crc(p,fixture_bytes),2));free(p);
}
static void malformed_headers(void){
 make_fixture();
 assert(!validate(0,fixture_bytes,0,2));assert(!validate(fixture,511,0,2));
 assert(!validate(fixture,fixture_bytes,crc(fixture,fixture_bytes),0));
 assert(!validate(fixture,fixture_bytes,crc(fixture,fixture_bytes),30));
 reject_changed_field(0,'X',1);reject_changed_field(8,3,4);
 reject_changed_field(12,4092,4);reject_changed_field(488,3,4);
 reject_changed_field(480,fixture_bytes-1,4);reject_changed_field(492,0xf0,1);
 reject_changed_field(16,511,4);reject_changed_field(20,0,4);
 reject_changed_field(20,fixture_bytes,4);reject_changed_field(32,read32(fixture+32)+1,4);
 reject_changed_field(484,read32(fixture+484)^1u,4);
 reject_changed_field(28,read32(fixture+28)^1u,4);
 assert(!validate(fixture,fixture_bytes,crc(fixture,fixture_bytes)^1u,2));
 fixture[508]^=1;assert(!validate(fixture,fixture_bytes,crc(fixture,fixture_bytes),2));fixture[508]^=1;
 /* All legal 2..16 counts can be represented; nibble zero (one frame) cannot. */
 for(unsigned n=2;n<=16;n++){uint8_t saved=fixture[492];fixture[492]=(uint8_t)(0xf0u+n-1u);assert(file_frame_count(fixture,0)==n);fixture[492]=saved;}
}
static void fixture_start(void){
 factory=fixture;frame_counts[0]=2;frame_counts[1]=16;ready=1;busy=0;bank=255;
}
static void malformed_payloads(void){
 fixture_start();finish_bank(0);
 for(unsigned i=0;i<4092;i++)assert(cache[i]==source_sample(0,i));
 finish_bank(1);for(unsigned i=0;i<PW_BANK_SAMPLES;i++)assert(cache[i]==source_sample(1,i));
 const unsigned off=read32(fixture+16),size=read32(fixture+20);
 /* Decode failures may never publish the requested bank. */
 const uint8_t bad_headers[]={3,4,(19u<<2)|1u};
 for(unsigned i=0;i<sizeof bad_headers;i++){
  uint8_t saved=fixture[off];fixture[off]=bad_headers[i];fixture_start();
  pw_tables_service_budget(0,1,1);assert(!pw_tables_ready()&&!pw_tables_busy()&&pw_tables_bank()==255);fixture[off]=saved;
 }
 put32(fixture+20,2);fixture_start();pw_tables_service(0,1);
 assert(!pw_tables_ready()&&!pw_tables_busy()&&pw_tables_bank()==255);put32(fixture+20,size);
 /* A bit flip with syntactically valid samples is caught by decoded CRC. */
 fixture[off+1]^=1;fixture_start();
 for(unsigned i=0;i<200&&pw_tables_ready()&&(pw_tables_bank()!=0||pw_tables_busy());i++)pw_tables_service(0,1);
 assert(!pw_tables_ready()&&!pw_tables_busy()&&pw_tables_bank()==255);fixture[off+1]^=1;
 /* Extra compressed data, or insufficient decoded data for the geometry. */
 put32(fixture+20,size+1);fixture_start();
 for(unsigned i=0;i<200&&pw_tables_ready()&&(pw_tables_bank()!=0||pw_tables_busy());i++)pw_tables_service(0,1);
 assert(!pw_tables_ready()&&pw_tables_bank()==255);put32(fixture+20,size);
 fixture_start();frame_counts[0]=16;
 for(unsigned i=0;i<1100&&pw_tables_ready()&&(pw_tables_bank()!=0||pw_tables_busy());i++)pw_tables_service(0,1);
 assert(!pw_tables_ready()&&pw_tables_bank()==255);
 /* Predictor overflow and truncated seeds/residuals stay inside cache/input. */
 uint8_t overflow[73]={0};overflow[0]=(18u<<2)|1u;overflow[1]=255;overflow[2]=127;overflow[3]=2;
 cursor=overflow;end=overflow+sizeof overflow;count=0;decode_samples=32;checksum=0xffffffffu;assert(block()<0);
 uint8_t short_seed[]={1};cursor=short_seed;end=short_seed+1;count=0;decode_samples=32;assert(block()<0);
 uint8_t too_many_seeds[]={2};cursor=too_many_seeds;end=too_many_seeds+1;count=0;decode_samples=1;assert(block()<0);
 free(fixture);
}
int main(void){
 real_banks();malformed_headers();malformed_payloads();
 puts("PASS: 25 real banks, mixed 2/16 geometry, all mips, silent/restarted switches, legacy reload identity, malformed headers/streams, CRCs and bounds");
 return 0;
}
