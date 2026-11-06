#include "pw_core.h"
/* Keep the 64-bit product separate from sample loads on the JieLi CPU. */
static __attribute__((noinline)) uint32_t scaled_increment(int32_t increment,int32_t scale){
    return (uint32_t)(((int64_t)increment*(int64_t)scale)>>12);
}
int32_t pw_core_tick(pw_core_voice *v,const int16_t *frames,uint32_t frame_count,uint32_t mip_shift){
    uint32_t phase,index,next,fraction,length;
    int32_t position,frame,morph,a,b,c,d,first,second;
    if(frame_count<2u || mip_shift>10u)return 0;
    v->phase+=v->pitch_scale_q12==4096?(uint32_t)v->frequency_increment:scaled_increment(v->frequency_increment,v->pitch_scale_q12);
    position=v->position_q8;
    if(position<0)position=0;
    if(position>=(int32_t)((frame_count-1u)*256u))position=(int32_t)((frame_count-1u)*256u)-1;
    v->position_q8=position;
    if(!frames)return 0; /* Muted cache reload: phase and position still advance. */
    frame=position>>8;morph=position&255;
    length=2048u>>mip_shift;
    phase=v->phase>>mip_shift;
    index=phase>>21;next=(index+1u)&(length-1u);
    fraction=(phase>>6)&32767u;
    a=frames[(uint32_t)frame*length+index];b=frames[(uint32_t)frame*length+next];
    c=frames[(uint32_t)(frame+1)*length+index];d=frames[(uint32_t)(frame+1)*length+next];
    first=(a*32768+(b-a)*(int32_t)fraction)>>15;
    second=(c*32768+(d-c)*(int32_t)fraction)>>15;
    return first*256+(second-first)*morph;
}

int32_t pw_core_tick_normalized(pw_core_voice *v,const int16_t *frames,uint32_t frame_count,uint32_t mip_shift){
    /* Preserve every legacy sample and its near-endpoint convention. */
    if(frame_count==16u)return pw_core_tick(v,frames,frame_count,mip_shift);
    if(frame_count<2u||frame_count>16u||mip_shift>10u)return 0;
    v->phase+=v->pitch_scale_q12==4096?(uint32_t)v->frequency_increment:scaled_increment(v->frequency_increment,v->pitch_scale_q12);
    v->position_q8=(int32_t)pw_core_normalized_position(v->position_q8);
    if(!frames)return 0;
    return pw_core_read_normalized(frames,frame_count,mip_shift,v->phase,v->position_q8);
}
