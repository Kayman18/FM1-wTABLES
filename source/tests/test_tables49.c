#include "../fm1/app/pw_tables.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t memory[PW_TABLE_BYTES] __attribute__((aligned(4)));static int available=1;
const uint8_t *plat_data(uint32_t off){return available&&off>=PW_TABLE_FLASH_START&&off-PW_TABLE_FLASH_START<PW_TABLE_BYTES?memory+off-PW_TABLE_FLASH_START:0;}
int main(void){FILE *f=fopen("data/factory-mips.pwlc","rb");assert(f&&fread(memory,1,sizeof memory,f)==sizeof memory);fclose(f);pw_tables_init();assert(pw_tables_ready());
 f=fopen("data/reference49.raw","rb");assert(f);int16_t reference[16368];
 for(unsigned b=0;b<PW_TABLE_BANKS;b++){assert(fread(reference,1,sizeof reference,f)==sizeof reference);for(unsigned i=0;i<32;i++)pw_tables_service(b,1);assert(pw_tables_bank()==b&&!pw_tables_busy());unsigned off=0;for(unsigned m=0;m<10;m++){unsigned n=8*(1024u>>m);assert(!memcmp(pw_table_mips[m][b],reference+off,n*2));off+=n;}}fclose(f);
 unsigned changes[]={0,8,12,16,20,24,144,252,256,480,484,488,508,512,PW_TABLE_BYTES-1};
 for(unsigned i=0;i<sizeof changes/sizeof changes[0];i++){memory[changes[i]]^=1;pw_tables_init();assert(!pw_tables_ready());memory[changes[i]]^=1;}
 available=0;pw_tables_init();assert(!pw_tables_ready());available=1;pw_tables_init();assert(pw_tables_ready());puts("PASS: all 16 banks/261888 samples checked byte-for-byte against raw reference, malformed bank rejected, missing mapping and recovery.");}
