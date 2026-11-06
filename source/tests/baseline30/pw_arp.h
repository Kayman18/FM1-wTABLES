#pragma once
#include <stdint.h>
/* Source modes 1 Up, 2 Down, 3 Alt, 4 Alt2; sorted input notes. */
unsigned pw_arp_next(uint8_t *step,unsigned count,unsigned mode,unsigned octaves);
uint32_t pw_arp_interval(unsigned bpm,uint32_t *remainder);
