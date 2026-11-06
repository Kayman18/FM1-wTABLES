/* TEST73: shared-buffer ownership and live GP5/Clouds integration.
   Includes the engine, as the existing regression tests do, to observe fades,
   incremental clearing, zero-wet sleeping, and the real worker path. */
#include "../fm1/app/test_engine.c"
#include "../fm1/app/pw_presets.h"
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int32_t pcm73[512];
static uint64_t audio_hash;
static uint64_t nonzero_samples;
static float memory_copy[PW_CLOUD_FLOATS];

static void render73(unsigned frames){
 assert(frames<=256u);
 core_render(pcm73,frames,4096);
 assert(!core_worker_fault());
 assert(core_primary_count()<=st.voice_limit);
 assert(isfinite(reverb_mix)&&reverb_mix>=0.0f&&reverb_mix<=1.0f);
 assert(isfinite(delay_mix)&&delay_mix>=0.0f&&delay_mix<=1.0f);
 for(unsigned i=0;i<frames*2u;i++){
  assert(pcm73[i]>=-8388608&&pcm73[i]<=8388607);
  nonzero_samples+=pcm73[i]!=0;
  audio_hash^=(uint32_t)pcm73[i];audio_hash*=UINT64_C(1099511628211);
 }
}

static void blocks73(unsigned count){while(count--)render73(256);}

static void model_settings(core_settings *s,unsigned model){
 pw_reverb_settings d=pw_reverb_defaults(model);
 s->fx_reverb=1;s->delay_mode=0;s->reverb_model=(uint8_t)model;
 s->reverb_size=(uint8_t)d.size;s->reverb_time_ms=(uint16_t)d.time;
 s->reverb_damping=(uint8_t)d.damping;s->reverb_speed=(uint16_t)d.speed;
 s->reverb_depth=(uint8_t)d.depth;s->reverb_mix=65;
}

static unsigned model_memory(unsigned model){
 return model==PW_REVERB_CLOUDS?PW_CLOUD_FLOATS:GP5_REVERB_DELAY_FLOATS;
}

static void check_memory(void){
 assert(reverb.mem==&delay_memory[0][0]);
 for(unsigned i=0;i<reverb.used;i++)assert(isfinite(reverb.mem[i]));
 if(reverb.model!=PW_REVERB_CLOUDS){
  assert(reverb.gp5.initialized);
  assert(isfinite(reverb.gp5.pending_wet[0])&&isfinite(reverb.gp5.pending_wet[1]));
 }
}

static void clearing_finishes(void){
 unsigned calls=0;
 while(!reverb_ready){
  unsigned before=reverb.clear_left;
  assert(before&&reverb_mix==0.0f);
  render73(64);
  assert(reverb.clear_left<before&&before-reverb.clear_left<=2048u);
  if(reverb.clear_left)assert(!reverb_ready&&reverb_mix==0.0f);
  assert(++calls<=9u);
 }
 assert(reverb.clear_left==0u);
 blocks73(2);
 assert(reverb_mix>0.64f&&!reverb_sleeping);
}

static void check_direct_contract(void){
 static float guarded[PW_CLOUD_FLOATS+2];
 assert(PW_REVERB_MODELS==11u&&PW_REVERB_CLOUDS==3u);
 assert(GP5_REVERB_DELAY_FLOATS<PW_CLOUD_FLOATS);
 assert(PW_CLOUD_FLOATS<=PW_DELAY_FRAMES*2u);
 assert(strcmp(pw_reverb_model_name(3),"CLOUDS")==0);
 assert(strcmp(pw_reverb_model_name(255),"CLOUDS")==0);
 for(unsigned model=0;model<PW_REVERB_MODELS;model++){
  unsigned used=model_memory(model);pw_reverb s={0};
  assert(pw_reverb_model_name(model)[0]);
  for(unsigned i=0;i<model;i++)assert(strcmp(pw_reverb_model_name(i),pw_reverb_model_name(model))!=0);
  assert(pw_reverb_configure(&s,guarded+1,used-1,model,70,44100)==used&&!s.used);
  assert(pw_reverb_configure(&s,guarded+1,PW_CLOUD_FLOATS,model,70,48000)==used&&!s.used);
  for(unsigned i=0;i<PW_CLOUD_FLOATS+2u;i++)guarded[i]=123.25f;
  assert(pw_reverb_configure(&s,guarded+1,PW_CLOUD_FLOATS,model,70,44100)==used);
  assert(s.used==used&&s.clear_left==used);
  for(unsigned i=0;i<PW_CLOUD_FLOATS+2u;i++)assert(guarded[i]==123.25f);
  while(s.clear_left){
   float wet[2]={1.0f,1.0f};unsigned before=s.clear_left;
   pw_reverb_tick(&s,1.0f,wet);assert(wet[0]==0.0f&&wet[1]==0.0f);
   pw_reverb_clear(&s,guarded+1,257);
   assert(s.clear_left<before&&before-s.clear_left<=257u);
  }
  for(unsigned i=0;i<used;i++)assert(guarded[i+1]==0.0f);
  assert(guarded[0]==123.25f&&guarded[used+1]==123.25f);
 }
 puts("PASS TEST73: 11 models, shared-buffer bounds, no write before incremental clear, dry while clearing");
}

static void check_control_units(void){
 core_init();core_settings s;pw_preset_default(&s);
 assert(s.reverb_model==3u);
 for(unsigned model=0;model<PW_REVERB_MODELS;model++){
  s.reverb_model=(uint8_t)model;s.reverb_time_ms=0;s.reverb_size=0;s.reverb_speed=0;
  core_set(&s);
  assert(st.reverb_time_ms==(model==3u?400u:0u));
  assert(st.reverb_size==(model==3u?1u:0u)&&st.reverb_speed==1u);
  s.reverb_time_ms=UINT16_MAX;s.reverb_speed=UINT16_MAX;
  s.reverb_size=s.reverb_damping=s.reverb_depth=s.reverb_mix=UINT8_MAX;
  core_set(&s);
  assert(st.reverb_time_ms==(model==3u?30000u:100u));
  assert(st.reverb_speed==500u&&st.reverb_size==100u);
  assert(st.reverb_damping==100u&&st.reverb_depth==100u&&st.reverb_mix==100u);
 }
 s.reverb_model=UINT8_MAX;core_set(&s);assert(st.reverb_model==3u);
 puts("PASS TEST73: GP5 0..100 controls, Clouds 400..30000 ms, rate 1..500, invalid model -> Clouds");
}

static void check_clouds_unchanged(void){
 /* Golden output from the exact uploaded TEST72 pw_reverb.c, same FP flags:
    132,300 stereo frames with seeded excitation, tail, and live controls. */
 static float mem[PW_CLOUD_FLOATS];pw_reverb s={0};
 uint64_t h=UINT64_C(14695981039346656037);unsigned rng=12345;
 pw_reverb_configure(&s,mem,PW_CLOUD_FLOATS,3,70,44100);
 while(!pw_reverb_clear(&s,mem,2048)){}
 pw_reverb_controls(&s,2000,50,25,20);
 for(unsigned i=0;i<132300;i++){
  if(i==44100){pw_reverb_space(&s,91);pw_reverb_controls(&s,9000,80,73,55);}
  if(i==88200){pw_reverb_space(&s,20);pw_reverb_controls(&s,400,0,1,0);}
  rng=rng*1664525u+1013904223u;
  float x=i<44100?(float)((int)(rng>>16)-32768)*(1.0f/131072.0f):0.0f;
  float wet[2];pw_reverb_tick(&s,x,wet);
  for(unsigned c=0;c<2;c++){
   uint32_t bits;assert(isfinite(wet[c]));memcpy(&bits,&wet[c],sizeof(bits));
   h^=bits;h*=UINT64_C(1099511628211);
  }
 }
 assert(h==UINT64_C(0xf95738bdaea969d5));
 printf("PASS TEST73: Clouds model 3 PCM matches uploaded TEST72 golden %016" PRIx64 "\n",h);
}

static uint64_t run_live_case(unsigned voices,unsigned dual){
 core_init();core_parallel_set(dual);
 core_settings s;pw_preset_default(&s);
 s.voice_limit=(uint8_t)voices;s.unison_count=1;s.bank=0;s.position=43;s.level=55;
 s.env=(pw_env_params){0,10,127,100};s.env2=(pw_env_params){10,40,80,70};
 s.filter=(pw_filter_params){PW_FILTER_LP,85,30,100};
 s.lfo.rate=35;s.lfo.wave=30;s.tracking.pitch_wave=10;
 s.matrix_source[0]=1;s.matrix_dest[0]=3;s.matrix_amount[0]=60;
 s.matrix_source[1]=5;s.matrix_dest[1]=5;s.matrix_amount[1]=45;
 s.matrix_source[2]=9;s.matrix_dest[2]=6;s.matrix_amount[2]=-50;
 s.chorus_mode=2;s.chorus_mix=30;
 core_set(&s);blocks73(200);
 for(unsigned i=0;i<voices;i++)core_midi(0,0x90,(uint8_t)(48+i*3),100);
 audio_hash=UINT64_C(14695981039346656037);nonzero_samples=0;
 blocks73(24);assert(core_primary_count()==voices&&nonzero_samples>0);
 /* Start on Clouds, then traverse every selectable model while notes sustain. */
 model_settings(&s,3);core_set(&s);blocks73(24);
 assert(reverb_active&&reverb_ready&&reverb.model==3u);
 for(unsigned model=0;model<PW_REVERB_MODELS;model++){
  unsigned outgoing=reverb.model,outgoing_time=reverb.time;
  assert(outgoing!=model&&reverb_mix>0.0f);
  model_settings(&s,model);core_set(&s);
  unsigned calls=0;
  while(reverb.model!=model){
   float previous_mix=reverb_mix;
   render73(17); /* Odd callback sizes also exercise GP5 polyphase continuity. */
   if(reverb.model==outgoing){
    assert(reverb.time==outgoing_time&&reverb_mix<=previous_mix);
    assert(previous_mix-reverb_mix<=17.0f/256.0f+1.0e-6f);
   }else{
    assert(previous_mix==0.0f&&reverb.model==model);
    assert(!reverb_ready&&reverb_mix==0.0f&&reverb.clear_left>0);
   }
   assert(++calls<40u);
  }
  assert(reverb.used==model_memory(model));
  clearing_finishes();blocks73(32);check_memory();
  assert(reverb.time==s.reverb_time_ms&&core_primary_count()==voices);
  if(model!=3u)assert(reverb.gp5.model==(model<3u?model:model-1u));
  /* Changing current-model controls must retain the running tank. */
  unsigned cursor=model==3u?reverb.write_ptr:reverb.gp5.feedback[0].position;
  s.reverb_size=model==3u?91u:73u;s.reverb_time_ms=model==3u?9000u:83u;
  s.reverb_damping=80;s.reverb_depth=55;s.reverb_speed=73;core_set(&s);render73(64);
  assert(reverb_ready&&reverb.clear_left==0u&&reverb.time==s.reverb_time_ms);
  assert((model==3u?reverb.write_ptr:reverb.gp5.feedback[0].position)!=cursor);
  blocks73(8);check_memory();
  /* Zero wet stops state advancement; wake clears in bounded chunks. */
  s.reverb_mix=0;core_set(&s);blocks73(4);
  assert(reverb_sleeping&&reverb_mix==0.0f&&reverb_zero_samples==256u);
  pw_reverb sleeping=reverb;memcpy(memory_copy,reverb.mem,reverb.used*sizeof(float));
  blocks73(4);
  assert(memcmp(&sleeping,&reverb,sizeof(reverb))==0);
  assert(memcmp(memory_copy,reverb.mem,reverb.used*sizeof(float))==0);
  s.reverb_mix=65;core_set(&s);render73(64);
  assert(!reverb_sleeping&&!reverb_ready&&reverb_mix==0.0f&&reverb.clear_left>0);
  clearing_finishes();blocks73(8);check_memory();
 }
 /* Return the same storage to both existing delay styles, then to Clouds. */
 for(unsigned style=0;style<2;style++){
  s.fx_reverb=0;s.delay_mode=2;s.delay_style=(uint8_t)style;s.delay_mix=60;
  s.delay_sync=0;s.delay_time_ms=90;s.delay_feedback=60;core_set(&s);
  unsigned calls=0;
  while(reverb_active){
   float before=reverb_mix;render73(17);
   if(!reverb_active)assert(before==0.0f);
   assert(++calls<40u);
  }
  blocks73(32);assert(delay_active==2u&&delay_mix>0.0f&&delay.valid>0u);
  model_settings(&s,3);core_set(&s);calls=0;
  while(!reverb_active){
   float before=delay_mix;render73(17);
   if(reverb_active)assert(before==0.0f&&reverb.model==3u&&!reverb_ready);
   assert(++calls<40u);
  }
  clearing_finishes();blocks73(8);assert(!delay_active&&delay_mix==0.0f);
 }
 assert(core_primary_count()==voices&&nonzero_samples>10000u);
 if(dual)assert(core_worker_jobs()>0u);else assert(core_worker_jobs()==0u);
 printf("PASS TEST73: voices=%u parallel=%u, all models/live fades/clear/sleep/wake/delay; PCM %016" PRIx64 ", worker jobs %u\n",
        voices,dual,audio_hash,core_worker_jobs());
 return audio_hash;
}

int main(void){
 check_direct_contract();check_control_units();check_clouds_unchanged();
 for(unsigned voices=4;voices<=6;voices+=2){
  uint64_t single=run_live_case(voices,0),parallel=run_live_case(voices,1);
  assert(single==parallel);
 }
 puts("PASS TEST73: four/six-voice single/parallel PCM identical; sanitizer integration complete");
 return 0;
}
