#include "../fm1/app/pw_tables.h"
#include "../fm1/app/pw_table_layout.h"
#include "../fm1/app/pw_mip_tables.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t memory[PW_TABLE_BYTES/4];static int available=1;
const uint8_t *plat_data(uint32_t off){return available&&off>=PW_TABLE_FLASH_START&&off-PW_TABLE_FLASH_START<PW_TABLE_BYTES?(uint8_t *)memory+off-PW_TABLE_FLASH_START:0;}
int main(void){FILE *f=fopen("data/factory-mips.pwbank","rb");assert(f);assert(fread(memory,1,sizeof memory,f)==sizeof memory);assert(fgetc(f)==EOF);fclose(f);
 pw_tables_init();assert(pw_tables_ready());
 for(unsigned m=0;m<10;m++)for(unsigned b=0;b<8;b++)assert(!memcmp(pw_table_mips[m][b],pw_mip_banks[m][b],8*(2048u>>(m+1))*2));
 unsigned changes[]={0,8,12,16,20,24,28,32,36,40,80,120,252,256,PW_TABLE_BYTES-1};
 for(unsigned i=0;i<sizeof changes/sizeof changes[0];i++){((uint8_t *)memory)[changes[i]]^=1;pw_tables_init();assert(!pw_tables_ready());for(unsigned m=0;m<10;m++)for(unsigned b=0;b<8;b++)assert(!pw_table_mips[m][b]);((uint8_t *)memory)[changes[i]]^=1;}
 available=0;pw_tables_init();assert(!pw_tables_ready());available=1;pw_tables_init();assert(pw_tables_ready());
 puts("PASS: C loader, every original sample, malformed headers/payload, missing mapping and reset recovery.");}
