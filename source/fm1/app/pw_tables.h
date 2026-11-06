#pragma once
#include <stdint.h>
#include "pw_table_layout.h"
void pw_tables_init(void);
int pw_tables_ready(void);
int pw_tables_busy(void);
unsigned pw_tables_bank(void);
/* Validated catalog geometry, immutable after init; 0 if unavailable/invalid.
 * The cache may still be loading: callers must keep the existing busy gate. */
unsigned pw_tables_frame_count(unsigned logical_bank);
/* CPU0 only, before launching workers; budget is capped at 16 codec blocks. */
void pw_tables_service(unsigned desired,int silent);
void pw_tables_service_budget(unsigned desired,int silent,unsigned max_blocks);
extern const int16_t *pw_table_mips[10][PW_TABLE_BANKS];
