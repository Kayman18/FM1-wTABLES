#pragma once
#include <stdint.h>
void pw_tables_init(void);
int pw_tables_ready(void);
int pw_tables_busy(void);
unsigned pw_tables_bank(void);
/* CPU0 only, before launching workers; at most 16 blocks per render call. */
void pw_tables_service(unsigned desired,int silent);
extern const int16_t *pw_table_mips[10][8];
