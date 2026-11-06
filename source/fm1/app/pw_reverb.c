/* TEST63 EXPERIMENTAL — adapted from genAMB RC2 Stretch ST3 CloudsCleanReverb.
   Topology from Mutable Instruments Clouds/Rings (Emilie Gillet, MIT).
   Exact tank delay lengths, four input allpasses, two cross-coupled branches,
   slow dual MI cosine LFOs, stereo modulation, and full-rate 32-bit float
   storage. Additional 149/211-sample input smear as in genAMB.
   This is NOT the granular Clouds processor or the old disting MultiFX verb.
*/
#include "pw_reverb.h"

static const char *const model_names[PW_REVERB_MODELS]={
 "GP5 AIR","GP5 ROOM","GP5 HALL","CLOUDS","GP5 CHURCH",
 "GP5 PLATE L","GP5 PLATE","GP5 SPRING","GP5 N-STAR",
 "GP5 DEEPSEA","GP5 SWEET"
};
static const pw_reverb_settings model_defaults[PW_REVERB_MODELS]={
 {50,100,0,50,0},{50,88,0,50,0},{50,100,70,50,0},
 {2000,70,50,25,20},{50,68,80,50,0},{50,80,0,65,100},
 {50,86,25,50,0},{50,65,14,500,7},{50,100,0,50,0},
 {50,46,100,50,0},{50,85,0,100,35}
};
const char *pw_reverb_model_name(unsigned model){
 return model_names[model<PW_REVERB_MODELS?model:PW_REVERB_CLOUDS];
}
pw_reverb_settings pw_reverb_defaults(unsigned model){
 return model_defaults[model<PW_REVERB_MODELS?model:PW_REVERB_CLOUDS];
}

#define C_MASK (PW_CLOUD_TANK_FLOATS - 1u)
static const uint16_t BASE[10]={0,114,277,519,919,2573,4612,8024,9938,11602};
static const uint16_t LEN[10]={113,162,241,399,1653,2038,3411,1913,1663,4782};
static float clamp01(float x){return x<0.0f?0.0f:(x>1.0f?1.0f:x);}
/* MI CosineOscillator<COSINE_OSCILLATOR_APPROXIMATE>, adapted verbatim from
   stmlib/dsp/cosine_oscillator.h with scalar float state. */
static void lfo_init(pw_cloud_lfo *s,float frequency){
 float sign=16.0f;
 frequency-=0.25f;
 if(frequency<0.0f)frequency=-frequency;
 else if(frequency>0.5f)frequency-=0.5f;
 else sign=-16.0f;
 s->coefficient=sign*frequency*(1.0f-2.0f*frequency);
 s->y1=s->coefficient*0.25f;
 s->y0=0.5f;
}
static float lfo_next(pw_cloud_lfo *s){
 float t=s->y0;
 s->y0=s->coefficient*s->y0-s->y1;
 s->y1=t;
 return t+0.5f;
}
static float ap_extra(pw_reverb *s,unsigned k,float x){
 const unsigned len=k?211u:149u;
 float *b=s->mem+PW_CLOUD_TANK_FLOATS+(k?149u:0u);
 unsigned pos=s->rev_ap_pos[k];
 float z=b[pos];float y=z-s->diffusion*x;
 b[pos]=x+s->diffusion*y;
 s->rev_ap_pos[k]=pos+1u==len?0u:pos+1u;
 return y;
}
typedef struct {float acc,prev;pw_reverb *s;} context;
static void load(context *c,float value){c->acc=value;}
static void read_v(context *c,float value,float scale){c->acc+=value*scale;}
static unsigned location(context *c,unsigned line,int32_t off){
 return (c->s->write_ptr+(unsigned)((int32_t)BASE[line]+off))&C_MASK;
}
static void read_line(context *c,unsigned line,int32_t offset,float scale){
 float v=c->s->mem[location(c,line,offset)];c->prev=v;c->acc+=v*scale;
}
static void write_line(context *c,unsigned line,int32_t offset,float scale){
 c->s->mem[location(c,line,offset)]=c->acc;c->acc*=scale;
}
static void allpass(context *c,unsigned line,float kread,float kwrite){
 read_line(c,line,(int32_t)LEN[line]-1,kread);
 /* MI WriteAllPass(d,scale) writes at offset 0, NOT the tail. */
 write_line(c,line,0,kwrite);
 c->acc+=c->prev;
}
static void interp(context *c,unsigned line,float offset,float scale){
 int32_t i=(int32_t)offset;
 float f=offset-(float)i;
 float a=c->s->mem[location(c,line,i)];
 float b=c->s->mem[location(c,line,i+1)];
 c->prev=a+(b-a)*f;
 c->acc+=c->prev*scale;
}
static void lowpass(context *c,float *state,float k){*state+=k*(c->acc-*state);c->acc=*state;}
static void highpass(context *c,float *state,float k){*state+=k*(c->acc-*state);c->acc-=*state;}
unsigned pw_reverb_configure(pw_reverb *s,float *mem,unsigned capacity,unsigned model,unsigned size,unsigned rate){
 if(model>=PW_REVERB_MODELS)model=PW_REVERB_CLOUDS;
 unsigned required=model==PW_REVERB_CLOUDS?PW_CLOUD_FLOATS:GP5_REVERB_DELAY_FLOATS;
 if(!s||!mem||capacity<required||rate!=44100u)return required;
 *s=(pw_reverb){0};s->mem=mem;s->used=s->clear_left=required;
 s->model=model;
 if(model!=PW_REVERB_CLOUDS){
  gp5_reverb_init(&s->gp5,mem,capacity);
  pw_reverb_settings d=pw_reverb_defaults(model);
  s->time=~0u;s->controls_dirty=1;
  pw_reverb_space(s,size);
  pw_reverb_controls(s,d.time,d.damping,d.speed,d.depth);
  return required;
 }
 s->write_ptr=0;s->diffusion=0.625f;s->lowpass=0.48f;
 s->regen=0.90f;s->depth=0.25f;
 s->time=0u; /* Force first controls application even at genAMB defaults. */
 s->lfo_value[0]=s->lfo_value[1]=0.5f;
 pw_reverb_space(s,size);
 pw_reverb_controls(s,2000u,50u,25u,25u);
 return PW_CLOUD_FLOATS;
}
void pw_reverb_space(pw_reverb*s,unsigned size){
 if(s->model!=PW_REVERB_CLOUDS){
  if(size>100u)size=100u;
  if(s->size!=size){s->size=size;s->controls_dirty=1;}
  return;
 }
 if(size<1u)size=1u;if(size>100u)size=100u;
 s->size=size;s->diffusion=0.45f+0.28f*(float)size*0.01f;
}
void pw_reverb_controls(pw_reverb*s,unsigned time,unsigned damping,unsigned speed,unsigned depth){
 if(s->model!=PW_REVERB_CLOUDS){
  if(time>100u)time=100u;if(damping>100u)damping=100u;
  if(speed<1u)speed=1u;if(speed>500u)speed=500u;if(depth>100u)depth=100u;
  if(s->time==time&&s->damping==damping&&s->speed==speed&&s->moddepth==depth&&!s->controls_dirty)return;
  if(s->time!=time){
   unsigned index=s->model<PW_REVERB_CLOUDS?s->model:s->model-1u;
   gp5_reverb_set_model(&s->gp5,index,(float)time,50.0f,50.0f);
  }
  float p[GP5_REVERB_CONTROL_COUNT];
  for(unsigned i=0;i<GP5_REVERB_CONTROL_COUNT;i++)p[i]=s->gp5.controls[i];
  /* The extra FM1 controls expose the shared engine fields directly;
     model defaults match the original model's center-position settings. */
  p[2]=(float)damping;p[3]=(float)s->size;
  p[9]=(float)depth;p[10]=(float)speed*0.2f;
  gp5_reverb_set_parameters(&s->gp5,p);
  s->time=time;s->damping=damping;s->speed=speed;s->moddepth=depth;s->controls_dirty=0;
  return;
 }
 if(time<400u)time=400u;if(time>30000u)time=30000u;
 if(damping>100u)damping=100u;
 if(speed<1u)speed=1u;if(speed>500u)speed=500u;
 if(depth>100u)depth=100u;
 if(s->time==time&&s->damping==damping&&s->speed==speed&&s->moddepth==depth)return;
 if(time<=2000u)s->regen=0.85f+0.05f*(float)(time-400u)*(1.0f/1600.0f);
 else s->regen=0.90f+0.07f*(float)(time-2000u)*(1.0f/28000.0f);
 s->lowpass=0.68f-0.42f*(float)damping*0.01f;
 s->depth=(float)depth*0.01f;
 if(s->speed!=speed){
  /* genAMB values 0.13 and 0.19 Hz at the TEST62 default speed=25.
     Same MI oscillator, only its frequency is adjustable. */
  float multiplier=(float)speed*(1.0f/25.0f);
  lfo_init(&s->lfo[0],(0.13f/44100.0f)*32.0f*multiplier);
  lfo_init(&s->lfo[1],(0.19f/44100.0f)*32.0f*multiplier);
  s->lfo_value[0]=s->lfo[0].y1+0.5f; /* MI value() before first Next(). */
  s->lfo_value[1]=s->lfo[1].y1+0.5f;
 }
 s->time=time;s->damping=damping;s->speed=speed;s->moddepth=depth;
}
int pw_reverb_clear(pw_reverb*s,float *mem,unsigned count){
 if(!s->used)return 0;
 if(count>s->clear_left)count=s->clear_left;
 unsigned start=s->used-s->clear_left;
 for(unsigned j=0;j<count;j++)mem[start+j]=0.0f;
 s->clear_left-=count;
 return s->clear_left==0u;
}
void pw_reverb_tick(pw_reverb*s,float input,float out[2]){
 if(!s->used||s->clear_left){out[0]=out[1]=0.0f;return;}
 if(s->model!=PW_REVERB_CLOUDS){gp5_reverb_tick(&s->gp5,input,out);return;}
 s->write_ptr=(s->write_ptr-1u)&C_MASK;
 if((s->write_ptr&31u)==0u){
  s->lfo_value[0]=lfo_next(&s->lfo[0]);
  s->lfo_value[1]=lfo_next(&s->lfo[1]);
 }
 context c={0,0,s};
 /* genAMB input is duplicated to left and right at half gain, so sum=input. */
 interp(&c,0,10.0f+60.0f*s->depth*s->lfo_value[0],1.0f);
 write_line(&c,0,100,0.0f);
 read_v(&c,ap_extra(s,1,ap_extra(s,0,input)),0.24f);
 allpass(&c,0,s->diffusion,-s->diffusion);
 allpass(&c,1,s->diffusion,-s->diffusion);
 allpass(&c,2,s->diffusion,-s->diffusion);
 allpass(&c,3,s->diffusion,-s->diffusion);
 float apout=c.acc;
 load(&c,apout);
 interp(&c,9,4680.0f+100.0f*s->depth*s->lfo_value[1],s->regen);
 lowpass(&c,&s->lp[0],s->lowpass);
 highpass(&c,&s->hp[0],0.0015f);
 allpass(&c,4,-s->diffusion,s->diffusion);
 allpass(&c,5,s->diffusion,-s->diffusion);
 write_line(&c,6,0,2.0f);
 out[0]=c.acc;
 load(&c,apout);
 interp(&c,6,3300.0f+70.0f*s->depth*s->lfo_value[0],s->regen);
 lowpass(&c,&s->lp[1],s->lowpass);
 highpass(&c,&s->hp[1],0.0015f);
 allpass(&c,7,s->diffusion,-s->diffusion);
 allpass(&c,8,-s->diffusion,s->diffusion);
 write_line(&c,9,0,2.0f);
 out[1]=c.acc;
 /* Damp inaudibly small feedback states: FM1 FPU must not run on denormals. */
 for(unsigned k=0;k<2;k++){
  if(s->lp[k]<1e-20f&&s->lp[k]>-1e-20f)s->lp[k]=0.0f;
  if(s->hp[k]<1e-20f&&s->hp[k]>-1e-20f)s->hp[k]=0.0f;
 }
}
