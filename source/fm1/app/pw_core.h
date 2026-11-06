#pragma once
#include <stdint.h>
/* Recovered Poly Wavetable oscillator sub-kernel only. Tables are signed
 * 16-bit, frame-major, with 2048 >> mip_shift samples per frame (shift 0..10).
 * Source: firmware 1.26.1, 0x9D029F4C..0x9D02A090 and boundary branches.
 * The source's leading frame-count storage offset is normalized away. */
typedef struct {
    uint32_t phase;
    int32_t frequency_increment;
    int32_t position_q8;
    int32_t pitch_scale_q12;
} pw_core_voice;
int32_t pw_core_tick(pw_core_voice *v,const int16_t *frames,uint32_t frame_count,uint32_t mip_shift);

/* The synth's control/smoothing domain stays at the legacy 16-frame range.
 * Only sample lookup maps that normalized position into smaller banks. */
#define PW_CORE_NORMALIZED_MAX 3839u
typedef struct {uint32_t frame,morph_q15;} pw_core_frame_blend;
static inline __attribute__((always_inline)) uint32_t pw_core_normalized_position(int32_t position){
    if(position<0)return 0;
    if((uint32_t)position>PW_CORE_NORMALIZED_MAX)return PW_CORE_NORMALIZED_MAX;
    return (uint32_t)position;
}
static inline __attribute__((always_inline)) pw_core_frame_blend pw_core_normalized_blend(int32_t position,uint32_t frame_count){
    uint32_t p=pw_core_normalized_position(position);
    /* Exact floor(p*32768/3839) over p=0..3839. The rounded-up reciprocal
     * needs at most one correction; every product fits in uint32_t.
     * This avoids division and 64-bit multiplies in the audio sample loop. */
    uint32_t weight=(p*559387u)>>16;
    if(weight*PW_CORE_NORMALIZED_MAX>(p<<15))--weight;
    weight*=frame_count-1u;
    pw_core_frame_blend blend={weight>>15,weight&32767u};
    /* At 100%, blend the final pair with full weight on its second frame.
     * Never form a last-frame + 1 pointer. */
    if(blend.frame>=frame_count-1u){blend.frame=frame_count-2u;blend.morph_q15=32768u;}
    return blend;
}
static inline __attribute__((always_inline)) int32_t pw_core_blend_sample(int32_t first,int32_t second,uint32_t morph_q15){
    return first*256+(((second-first)*(int32_t)morph_q15)>>7);
}
static inline __attribute__((always_inline)) int32_t pw_core_read_normalized(const int16_t *frames,uint32_t frame_count,uint32_t mip_shift,uint32_t phase,int32_t position){
    pw_core_frame_blend blend=pw_core_normalized_blend(position,frame_count);
    uint32_t length=2048u>>mip_shift,shifted_phase=phase>>mip_shift;
    uint32_t index=shifted_phase>>21,next=(index+1u)&(length-1u),fraction=(shifted_phase>>6)&32767u;
    const int16_t *a=frames+blend.frame*length,*b=a+length;
    int32_t first=((int32_t)a[index]*32768+((int32_t)a[next]-a[index])*(int32_t)fraction)>>15;
    int32_t second=((int32_t)b[index]*32768+((int32_t)b[next]-b[index])*(int32_t)fraction)>>15;
    return pw_core_blend_sample(first,second,blend.morph_q15);
}
/* New mixed-bank entry point. position_q8 remains in the legacy normalized
 * domain; the original recovered frame-unit API above is unchanged. */
int32_t pw_core_tick_normalized(pw_core_voice *v,const int16_t *frames,uint32_t frame_count,uint32_t mip_shift);
