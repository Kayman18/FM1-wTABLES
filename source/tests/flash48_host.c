/* Read-only simulated flash for DSP parity tests, initialized before worker start. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "pw_table_layout.h"
static uint8_t memory[PW_TABLE_BYTES] __attribute__((aligned(4)));
static int loaded;
const uint8_t *plat_data(uint32_t off){
 if(!loaded){FILE *f=fopen("data/factory-mips.pwlc","rb");if(!f)abort();if(fread(memory,1,sizeof memory,f)!=sizeof memory||fgetc(f)!=EOF)abort();fclose(f);loaded=1;}
 return off>=PW_TABLE_FLASH_START&&off-PW_TABLE_FLASH_START<PW_TABLE_BYTES?(uint8_t *)memory+off-PW_TABLE_FLASH_START:0;
}
