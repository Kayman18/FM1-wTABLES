#pragma once
#include <stdint.h>
#define PW_BLOOM_STEPS 32u
#define PW_BLOOM_SCALES 17u
int pw_bloom_offset(unsigned note,unsigned scale,int steps);
/* Compact integer-note adaptation of Bloom checkpoint 04's recovered routines. */
typedef struct {uint32_t rng;uint8_t seed[PW_BLOOM_STEPS],notes[PW_BLOOM_STEPS],reference[PW_BLOOM_STEPS];uint8_t tree[2][255][PW_BLOOM_STEPS];uint32_t tree_rng;uint16_t build;uint8_t active,ready,valid,length,scale,node,depth,branches,path,service_phase;} pw_bloom;
uint32_t pw_bloom_random(uint32_t *rng);
unsigned pw_bloom_quantize(unsigned note,unsigned scale);
unsigned pw_bloom_mutate(uint32_t *rng,unsigned scale,unsigned raw,unsigned reference);
void pw_bloom_init(pw_bloom *b);
void pw_bloom_reseed(pw_bloom *b);
void pw_bloom_restore(pw_bloom *b);
unsigned pw_bloom_step(pw_bloom *b,unsigned index,unsigned scale,unsigned amount);

void pw_bloom_branch(uint8_t *dst,const uint8_t *src,const uint8_t *ancestor,unsigned length,unsigned scale,uint32_t *rng);
void pw_bloom_service(pw_bloom *b,unsigned length,unsigned scale);
void pw_bloom_boundary(pw_bloom *b,unsigned branches,unsigned path);
unsigned pw_bloom_tree_step(pw_bloom *b,unsigned index,unsigned scale,unsigned amount);
unsigned pw_bloom_edit(pw_bloom *b,unsigned index,int delta,unsigned scale);

/* Shared internal eighth-note tempo math: Bloom owns the clock, delay follows sequencer BPM. */
uint32_t pw_bloom_eighth_interval(unsigned bpm,uint32_t *remainder);
