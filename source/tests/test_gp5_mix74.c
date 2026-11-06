/* TEST74 mixer contract, observed through the real core_render path.
   The test-only callback can return a known signal or delegate unchanged to
   the production reverb. No application or DSP source is substituted. */
#include "../fm1/app/pw_reverb.h"
static void observed_tick74(pw_reverb *,float,float[2]);
#define pw_reverb_tick observed_tick74
#include "../fm1/app/test_engine.c"
#undef pw_reverb_tick
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {float input,wet[2],mix;unsigned model;} observation74;
static observation74 observed[256];
static unsigned observed_count,mock_wet,zero_wet;
static int32_t pcm74[512];
static const float known_wet[2]={1024.0f,-512.0f};

static void observed_tick74(pw_reverb *s,float input,float wet[2]){
 if(mock_wet){wet[0]=zero_wet?0.0f:known_wet[0];wet[1]=zero_wet?0.0f:known_wet[1];}
 else pw_reverb_tick(s,input,wet);
 assert(observed_count<256u);
 observed[observed_count++]=(observation74){input,{wet[0],wet[1]},reverb_mix,s->model};
 assert(isfinite(input)&&isfinite(wet[0])&&isfinite(wet[1]));
}

static void render74(unsigned frames){
 assert(frames<=256u);observed_count=0;core_render(pcm74,frames,4096);
 assert(!core_worker_fault()&&isfinite(reverb_mix));
 assert(reverb_mix>=0.0f&&reverb_mix<=1.0f);
 for(unsigned i=0;i<frames*2u;i++)assert(pcm74[i]>=-8388608&&pcm74[i]<=8388607);
 for(unsigned c=0;c<2;c++)assert(isfinite(output_peaks[c]));
}
static void blocks74(unsigned blocks){while(blocks--)render74(256);}

static void set_model74(core_settings *s,unsigned model,unsigned mix){
 pw_reverb_settings d=pw_reverb_defaults(model);
 s->fx_reverb=1;s->delay_mode=0;s->reverb_model=(uint8_t)model;
 s->reverb_time_ms=(uint16_t)d.time;s->reverb_size=(uint8_t)d.size;
 s->reverb_damping=(uint8_t)d.damping;s->reverb_speed=(uint16_t)d.speed;
 s->reverb_depth=(uint8_t)d.depth;s->reverb_mix=(uint8_t)mix;
}

static core_settings prepare74(unsigned model,unsigned mix,unsigned bypass){
 core_init();core_parallel_set(0);core_settings s;pw_preset_default(&s);
 s.level=50;s.position=35;s.voice_limit=4;s.unison_count=1;
 s.env=(pw_env_params){0,10,127,100};s.filter.mode=PW_FILTER_OFF;
 s.chorus_mode=s.delay_mode=0;s.stereo_mode=0;
 set_model74(&s,model,mix);if(bypass)s.fx_reverb=0;
 core_set(&s);blocks74(64);
 assert(pw_tables_ready()&&!pw_tables_busy()&&gain==2048);
 core_midi(0,0x90,60,100);blocks74(24);
 assert(core_primary_count()==1u&&gain==2048&&synth_gain==1.0f&&voice_headroom==1.0f);
 return s;
}

static void capture74(unsigned model,unsigned mix,unsigned bypass,int32_t output[512],
                      float input[256],float wet[256][2]){
 prepare74(model,mix,bypass);render74(256);memcpy(output,pcm74,sizeof(pcm74));
 if(mix&&!bypass){
  assert(observed_count==256u&&reverb_mix==(float)mix*.01f);
  for(unsigned i=0;i<256;i++){
   if(input)input[i]=observed[i].input;
   if(wet){wet[i][0]=observed[i].wet[0];wet[i][1]=observed[i].wet[1];}
  }
 }else assert(observed_count==0u);
}

static void endpoints74(void){
 static int32_t dry[512],zero[512],half[512],full[512],quiet[512],one[512];
 static float input1[256],input50[256],input100[256];
 mock_wet=1;zero_wet=0;capture74(0,0,1,dry,0,0);
 unsigned varied=0;for(unsigned i=2;i<512;i++)varied+=dry[i]!=dry[i-2];assert(varied>100u);
 for(unsigned model=0;model<PW_REVERB_MODELS;model++)if(model!=PW_REVERB_CLOUDS){
  capture74(model,0,0,zero,0,0);assert(memcmp(dry,zero,sizeof(dry))==0);
  capture74(model,50,0,half,input50,0);
  capture74(model,100,0,full,input100,0);
  capture74(model,1,0,one,input1,0);
  assert(memcmp(input1,input50,sizeof(input1))==0&&memcmp(input50,input100,sizeof(input50))==0);
  for(unsigned i=0;i<512;i++){
   /* Settled master gain is 2048: output scale is exactly 32. */
   assert(full[i]==(int32_t)(known_wet[i&1u]*32.0f));
   int64_t error=(int64_t)half[i]*2-(int64_t)dry[i]-(int64_t)full[i];
   assert(error>=-2&&error<=2); /* Only integer PCM rounding is permitted. */
  }
  zero_wet=1;capture74(model,100,0,quiet,0,0);zero_wet=0;
  for(unsigned i=0;i<512;i++)assert(quiet[i]==0); /* Nonzero input, no dry leakage. */
 }
 puts("PASS TEST74: every GP5 model: 0% exact dry, 50% half of each, 100% unity wet, zero dry leak");
 puts("PASS TEST74: input excitation identical at 1/50/100% with the isolated mixer callback");
}

static void real_core74(void){
 static int32_t out[512];static float input_ref[256],input[256];
 static float wet_ref[256][2],wet[256][2],tank_memory[GP5_REVERB_DELAY_FLOATS];
 gp5_reverb tank_state;mock_wet=zero_wet=0;
 for(unsigned model=0;model<PW_REVERB_MODELS;model++)if(model!=PW_REVERB_CLOUDS){
  capture74(model,1,0,out,input_ref,wet_ref);
  tank_state=reverb.gp5;memcpy(tank_memory,reverb.mem,sizeof(tank_memory));
  for(unsigned test=0;test<2;test++){
   unsigned mix=test?100u:50u;capture74(model,mix,0,out,input,wet);
   assert(memcmp(input_ref,input,sizeof(input))==0);
   assert(memcmp(wet_ref,wet,sizeof(wet))==0); /* Mix does not change tank history. */
   assert(memcmp(&tank_state,&reverb.gp5,sizeof(tank_state))==0);
   assert(memcmp(tank_memory,reverb.mem,sizeof(tank_memory))==0);
   unsigned audible=0;
   for(unsigned i=0;i<256;i++)for(unsigned c=0;c<2;c++){
    float expected=(test?wet[i][c]:(input[i]+wet[i][c])*.5f)*32.0f;
    assert(fabsf(expected)<7549747.0f); /* Below the existing output soft knee. */
    int64_t error=(int64_t)out[2*i+c]-(int32_t)expected;
    assert(error>=-1&&error<=1);audible+=wet[i][c]!=0.0f;
   }
   assert(audible>256u);
  }
 }
 puts("PASS TEST74: real GP5 input and wet PCM unchanged by Mix 1/50/100; 50/100 output endpoints verified");
}

static void checked_transition_sample74(void){
 float before=reverb_mix;render74(1);
 assert(fabsf(reverb_mix-before)<=1.0f/256.0f+1.0e-6f);
 float dry[2]={0,0};
 for(unsigned i=0;i<voice_job.render_slots;i++){
  dry[0]+=voice_job.output[i][0][0];dry[1]+=voice_job.output[i][0][1];
 }
 assert(synth_gain==1.0f&&voice_headroom==1.0f&&gain==2048);
 for(unsigned c=0;c<2;c++){
  float expected=dry[c];
  if(observed_count){
   assert(observed_count==1u);
   float m=observed[0].mix,w=observed[0].wet[c];
   expected=observed[0].model==PW_REVERB_CLOUDS?
       dry[c]+w*(.375f*m):dry[c]*(1.0f-m)+w*m;
  }
  expected*=32.0f;assert(fabsf(expected)<7549747.0f);
  int64_t error=(int64_t)pcm74[c]-(int32_t)expected;assert(error>=-1&&error<=1);
 }
}

static void transitions74(void){
 mock_wet=1;zero_wet=0;core_settings s=prepare74(0,100,0);
 const unsigned mixes[]={50,0,100,0,50,100};
 for(unsigned k=0;k<sizeof(mixes)/sizeof(mixes[0]);k++){
  s.reverb_mix=(uint8_t)mixes[k];core_set(&s);
  for(unsigned i=0;i<768;i++)checked_transition_sample74();
  assert(reverb_mix==(float)mixes[k]*.01f);
 }
 const unsigned models[]={5,3,10,0};
 for(unsigned k=0;k<sizeof(models)/sizeof(models[0]);k++){
  set_model74(&s,models[k],100);core_set(&s);
  for(unsigned i=0;i<768;i++)checked_transition_sample74();
  assert(reverb.model==models[k]&&reverb_ready&&reverb_mix==1.0f);
 }
 s.fx_reverb=0;s.delay_mode=0;core_set(&s);
 for(unsigned i=0;i<768;i++)checked_transition_sample74();
 assert(!reverb_active&&reverb_mix==0.0f);
 set_model74(&s,2,100);core_set(&s);
 for(unsigned i=0;i<768;i++)checked_transition_sample74();
 assert(reverb_active&&reverb_ready&&reverb_mix==1.0f);
 puts("PASS TEST74: sample-wise mix/model/FX transitions preserve the 1/256 ramp and exact dry/wet contract");
}

static void full_wet_stress74(void){
 mock_wet=zero_wet=0;core_init();core_parallel_set(1);
 core_settings s;pw_preset_default(&s);s.voice_limit=6;s.unison_count=1;s.level=100;s.gain_db=12;
 s.env=(pw_env_params){0,10,127,100};s.filter=(pw_filter_params){PW_FILTER_LP,92,80,100};
 s.chorus_mode=2;s.chorus_mix=80;s.position=63;
 set_model74(&s,0,100);core_set(&s);blocks74(64);
 for(unsigned v=0;v<6;v++)core_midi(0,0x90,(uint8_t)(42+v*4),127);
 float peak=0.0f;unsigned audible=0;
 for(unsigned model=0;model<PW_REVERB_MODELS;model++)if(model!=PW_REVERB_CLOUDS){
  set_model74(&s,model,100);s.reverb_time_ms=100;s.reverb_size=100;s.reverb_damping=0;
  s.reverb_depth=100;s.reverb_speed=500;core_set(&s);
  for(unsigned b=0;b<160;b++){
   if(b==40)for(unsigned v=0;v<6;v++)core_midi(0,0x90,(uint8_t)(48+v*3+model),127);
   if(b==80||b==120){s.reverb_mix=b==80?0:50;core_set(&s);}
   if(b==84||b==124){s.reverb_mix=100;core_set(&s);}
   render74(256);assert(core_primary_count()<=6u);
   for(unsigned i=0;i<512;i++)audible+=pcm74[i]!=0;
   for(unsigned i=0;i<observed_count;i++)for(unsigned c=0;c<2;c++)peak=fmaxf(peak,fabsf(observed[i].wet[c]));
   if(reverb_ready)for(unsigned i=0;i<reverb.used;i++)assert(isfinite(reverb.mem[i]));
  }
  assert(reverb.model==model&&reverb_ready&&reverb_mix==1.0f);
 }
 assert(audible>100000u&&core_worker_jobs()>0u);
 printf("PASS TEST74: all ten GP5 models at 100%% wet, six parallel voices, +12 dB synth gain, control extremes; finite/bounded PCM, wet peak %.6g\n",(double)peak);
}

int main(void){
 endpoints74();real_core74();transitions74();full_wet_stress74();
 puts("PASS TEST74: mixer endpoints, no dry leakage, unchanged excitation, transitions and boosted-return stability");
 return 0;
}
