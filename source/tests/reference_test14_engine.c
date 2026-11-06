#include "test_engine.h"
#include "pw_core.h"
#include "akwf_tables.h"
#include "pw_filter_mod.h"
#include "pw_wave_mod.h"
#include "pw_env_pitch.h"
#include "pw_performance.h"
#define VOICES 4u
#define STEAL_FADE 128u
#define PANEL_PORT 254u
#define AUDITION_PORT 255u
static volatile core_settings st;
static volatile uint32_t panel_keys;
static volatile uint8_t panic_requested;
static uint32_t processed_keys,serial;
static int32_t gain,position;
static uint8_t active_bank,audition_on,audition_note,filter_mode;
static float env2_pitch_depth,tuning_cents;
typedef struct {int16_t bend;uint8_t sustain;} midi_control;
static midi_control controls[3][16];
static float track_vel_wave,track_pitch_wave,track_vel_cutoff,track_pitch_cutoff;
static float wave_env_depth,wave_env2_depth;
static float lfo_wave_depth,lfo_filter_depth,lfo_pitch_depth,lfo_increment;
static float filter_wet,filter_g,filter_q,filter_base,filter_depth,filter_depth2;
typedef struct {
 pw_core_voice osc;
 pw_env env,env2;
 pw_filter filter;
 pw_lfo lfo;
 float lfo_smoothed;
 float cutoff_index,pitch_cents,bend_cents,performance_cents;
 int8_t key_octave;
 uint32_t age;
 uint8_t port,channel,note,velocity,held,sustained;
} poly_voice;
typedef struct {poly_voice main,tail;uint32_t fade;float tail_weight;} voice_slot;
static voice_slot slots[VOICES];
static const uint32_t INC[60]={3185015,3374406,3575058,3787642,4012867,4251485,4504291,4772130,5055896,5356535,5675051,6012507,6370030,6748811,7150117,7575285,8025735,8502970,9008582,9544261,10111792,10713070,11350103,12025015,12740059,13497623,14300233,15150569,16051469,17005939,18017165,19088521,20223584,21426141,22700205,24050030,25480119,26995246,28600467,30301139,32102938,34011878,36034330,38177043,40447168,42852281,45400411,48100060,50960238,53990491,57200933,60602278,64205876,68023757,72068660,76354085,80894335,85704563,90800821,96200119};

static int live(const poly_voice *v){return v->env.stage!=PW_ENV_IDLE||pw_filter_live(&v->filter);}
static void defaults(poly_voice *v){*v=(poly_voice){0};pw_env_init(&v->env);pw_env_init(&v->env2);pw_env_set(&v->env2,(pw_env_params){60,70,64,50});v->osc.pitch_scale_q12=4096;}
void core_init(void){
 st=(core_settings){0,48,0,0,75,{50,60,80,60},{PW_FILTER_OFF,96,0,100},0,{60,70,64,50},0,{40,0,0,0},0,0,{0,0,0,0},PW_LFO_KEEP,0,0};
 panel_keys=processed_keys=serial=panic_requested=0;gain=position=active_bank=audition_on=0;audition_note=48;filter_mode=PW_FILTER_OFF;filter_wet=0.0f;filter_g=pw_filter_g(96);filter_q=pw_filter_q(0);filter_base=96.0f;filter_depth=filter_depth2=0.0f;pw_filter_mod_init();env2_pitch_depth=0.0f;track_vel_wave=track_pitch_wave=track_vel_cutoff=track_pitch_cutoff=0.0f;wave_env_depth=wave_env2_depth=0.0f;lfo_wave_depth=lfo_filter_depth=lfo_pitch_depth=0.0f;lfo_increment=(float)pw_lfo_increment(40);
 tuning_cents=0.0f;for(unsigned p=0;p<3;p++)for(unsigned c=0;c<16;c++)controls[p][c]=(midi_control){0};
 for(unsigned i=0;i<VOICES;i++){defaults(&slots[i].main);defaults(&slots[i].tail);slots[i].fade=0;}
}
void core_set(const core_settings *s){core_settings c=*s;if(c.note<24)c.note=24;if(c.note>83)c.note=83;if(c.position>100)c.position=100;if(c.bank>7)c.bank=7;if(c.level>100)c.level=100;if(c.filter.mode>3)c.filter.mode=3;if(c.filter.cutoff>127)c.filter.cutoff=127;if(c.filter.resonance>100)c.filter.resonance=100;if(c.filter.mix>100)c.filter.mix=100;if(c.filter_env_depth>127)c.filter_env_depth=127;if(c.filter_env2_depth>127)c.filter_env2_depth=127;if(c.lfo.rate>100)c.lfo.rate=100;if(c.lfo.rate< -100)c.lfo.rate= -100;if(c.lfo.wave>100)c.lfo.wave=100;if(c.lfo.cutoff>96)c.lfo.cutoff=96;if(c.lfo.pitch>100)c.lfo.pitch=100;if(c.wave_env_depth>100)c.wave_env_depth=100;if(c.wave_env_depth< -100)c.wave_env_depth= -100;if(c.wave_env2_depth>100)c.wave_env2_depth=100;if(c.wave_env2_depth< -100)c.wave_env2_depth= -100;
 if(c.tracking.velocity_wave>100)c.tracking.velocity_wave=100;if(c.tracking.velocity_wave< -100)c.tracking.velocity_wave= -100;
 if(c.tracking.pitch_wave>100)c.tracking.pitch_wave=100;if(c.tracking.pitch_wave< -100)c.tracking.pitch_wave= -100;
 if(c.tracking.velocity_cutoff< -127)c.tracking.velocity_cutoff= -127;
 if(c.tracking.pitch_cutoff>100)c.tracking.pitch_cutoff=100;if(c.tracking.pitch_cutoff< -100)c.tracking.pitch_cutoff= -100;if(c.lfo_phase_mode>PW_LFO_OFF)c.lfo_phase_mode=PW_LFO_OFF;if(c.lfo_spread>90)c.lfo_spread=90;if(c.env2_pitch>120)c.env2_pitch=120;if(c.env2_pitch< -120)c.env2_pitch= -120;
 if(c.amp_attack_shape< -64)c.amp_attack_shape= -64;if(c.amp_attack_shape>63)c.amp_attack_shape=63;
 if(c.amp_decay_shape< -64)c.amp_decay_shape= -64;if(c.amp_decay_shape>63)c.amp_decay_shape=63;
 if(c.env2_attack_shape< -64)c.env2_attack_shape= -64;if(c.env2_attack_shape>63)c.env2_attack_shape=63;
 if(c.env2_decay_shape< -64)c.env2_decay_shape= -64;if(c.env2_decay_shape>63)c.env2_decay_shape=63;if(c.coarse_tune< -24)c.coarse_tune= -24;if(c.coarse_tune>24)c.coarse_tune=24;
 if(c.fine_tune< -100)c.fine_tune= -100;if(c.fine_tune>100)c.fine_tune=100;
 if(c.panel_octave< -2)c.panel_octave= -2;if(c.panel_octave>2)c.panel_octave=2;
 if(c.bend_range_offset< -2)c.bend_range_offset= -2;if(c.bend_range_offset>22)c.bend_range_offset=22;st=c;}
void core_keys(uint32_t keys){panel_keys=keys&0x7ffffffu;}
static int matches(const poly_voice *v,unsigned port,unsigned channel,unsigned note){return live(v)&&v->port==port&&v->channel==channel&&v->note==note;}
static void note_off(unsigned port,unsigned channel,unsigned note){
 for(unsigned i=0;i<VOICES;i++){
  poly_voice *v=&slots[i].main;
  if(matches(v,port,channel,note)&&v->held){v->held=0;if(port<3&&controls[port][channel].sustain)v->sustained=1;else{v->sustained=0;pw_env_off(&v->env);pw_env_off(&v->env2);}}
 }
}
static void note_on(unsigned port,unsigned channel,unsigned note,unsigned velocity){
 if(note<24||note>83||!velocity)return;
 uint32_t phases[VOICES];int any_held=0;
 for(unsigned i=0;i<VOICES;i++){phases[i]=slots[i].main.lfo.phase;unsigned stage=slots[i].main.env.stage;if(stage>=PW_ENV_SUSTAIN&&stage<=PW_ENV_DECAY)any_held=1;}
 unsigned chosen=VOICES;
 /* Repeated note-on retriggers its own slot, preserving phase and level. */
 for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note)){chosen=i;break;}
 int fresh=chosen==VOICES;
 if(chosen==VOICES){
  for(unsigned i=0;i<VOICES;i++)if(!live(&slots[i].main)&&!slots[i].fade){chosen=i;break;}
  if(chosen==VOICES){
   /* Prefer the quietest released voice, then the oldest held voice. */
   float quiet=2.0f;
   for(unsigned i=0;i<VOICES;i++)if(!slots[i].main.held&&!slots[i].main.sustained&&slots[i].main.env.level<quiet){chosen=i;quiet=slots[i].main.env.level;}
   if(chosen==VOICES){for(unsigned i=0;i<VOICES;i++)if(slots[i].main.sustained&&(chosen==VOICES||(uint32_t)(serial-slots[i].main.age)>(uint32_t)(serial-slots[chosen].main.age)))chosen=i;}
   if(chosen==VOICES){chosen=0;for(unsigned i=1;i<VOICES;i++)if((uint32_t)(serial-slots[i].main.age)>(uint32_t)(serial-slots[chosen].main.age))chosen=i;}
  }
  voice_slot *slot=&slots[chosen];
  if(live(&slot->main)&&(!slot->fade||slot->main.env.level>0.0f)){
   /* Preserve an interrupted crossfade's approximate current amplitude. */
   slot->tail_weight=1.0f-(float)slot->fade/(float)STEAL_FADE;
   slot->tail=slot->main;slot->fade=STEAL_FADE;
  }
  defaults(&slot->main);
 }
 pw_lfo_assign_phase(phases,VOICES,chosen,st.lfo_phase_mode,st.lfo_spread,any_held,fresh);
 for(unsigned i=0;i<VOICES;i++)slots[i].main.lfo.phase=phases[i];
 poly_voice *v=&slots[chosen].main;
 v->port=(uint8_t)port;v->channel=(uint8_t)channel;v->note=(uint8_t)note;v->velocity=(uint8_t)velocity;v->held=1;v->sustained=0;v->age=++serial;v->key_octave=port==PANEL_PORT?st.panel_octave:0;
 v->bend_cents=port<3?pw_performance_bend(controls[port][channel].bend,(unsigned)(2+st.bend_range_offset)):0.0f;
 v->osc.frequency_increment=(int32_t)INC[note-24];v->osc.position_q8=position;
 pw_env_shapes(&v->env,64+st.amp_attack_shape,64+st.amp_decay_shape);pw_env_shapes(&v->env2,64+st.env2_attack_shape,64+st.env2_decay_shape);
 pw_env_set(&v->env,st.env);pw_env_on(&v->env);pw_env_set(&v->env2,st.env2);pw_env_on(&v->env2);
}
static void pedal_up(unsigned port,unsigned channel){
 controls[port][channel].sustain=0;
 for(unsigned i=0;i<VOICES;i++){
  poly_voice *v=&slots[i].main;
  if(v->port==port&&v->channel==channel&&v->sustained){v->sustained=0;if(!v->held){pw_env_off(&v->env);pw_env_off(&v->env2);}}
 }
}
void core_midi(unsigned port,uint8_t status,uint8_t note,uint8_t velocity){
 if(port>2)return;unsigned channel=status&15,command=status&240;note&=127;velocity&=127;
 if(command==0x90&&velocity)note_on(port,channel,note,velocity);
 else if(command==0x80||command==0x90)note_off(port,channel,note);
 else if(command==0xe0)controls[port][channel].bend=(int16_t)((int)((unsigned)velocity<<7|note)-8192);
 else if(command==0xb0){
  if(note==64){if(velocity>=64)controls[port][channel].sustain=1;else pedal_up(port,channel);}
  else if(note==121){controls[port][channel].bend=0;pedal_up(port,channel);}
  else if(note==120||note==123){
   for(unsigned i=0;i<VOICES;i++){
    voice_slot *s=&slots[i];
    if(s->main.port==port&&s->main.channel==channel){
     if(note==120)defaults(&s->main);
     else if(s->main.held)note_off(port,channel,s->main.note);
    }
    if(note==120&&s->tail.port==port&&s->tail.channel==channel){s->fade=0;defaults(&s->tail);}
   }
  }
 }
}
void core_panic(void){st.playing=0;panic_requested=1;}
static const poly_voice *latest(unsigned held_only){
 const poly_voice *v=0;
 for(unsigned i=0;i<VOICES;i++)if(live(&slots[i].main)&&(!held_only||slots[i].main.held))
  if(!v||(uint32_t)(serial-slots[i].main.age)<(uint32_t)(serial-v->age))v=&slots[i].main;
 return v;
}
unsigned core_active_note(void){const poly_voice *v=latest(1);return v?v->note:0;}
unsigned core_env_stage(void){const poly_voice *v=latest(0);return v?v->env.stage:0;}
float core_env_level(void){float level=0;for(unsigned i=0;i<VOICES;i++)if(slots[i].main.env.level>level)level=slots[i].main.env.level;return level;}
unsigned core_voice_count(void){unsigned n=0;for(unsigned i=0;i<VOICES;i++)if(live(&slots[i].main)||slots[i].fade)n++;return n;}
unsigned core_held_count(void){unsigned n=0;for(unsigned i=0;i<VOICES;i++)if(slots[i].main.held&&live(&slots[i].main))n++;return n;}
unsigned core_note_held(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(slots[i].main.held&&matches(&slots[i].main,port,channel,note))return 1;return 0;}
float core_env2_level(void){float level=0;for(unsigned i=0;i<VOICES;i++)if(live(&slots[i].main)&&slots[i].main.env2.level>level)level=slots[i].main.env2.level;return level;}
float core_note_env2(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return slots[i].main.env2.level;return -1.0f;}
float core_lfo_level(void){const poly_voice *v=latest(0);return v?v->lfo_smoothed:0.0f;}
uint32_t core_lfo_phase(void){const poly_voice *v=latest(0);return v?v->lfo.phase:0;}
float core_note_lfo(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return slots[i].main.lfo_smoothed;return 0.0f;}
uint32_t core_voice_phase(unsigned slot){return slot<VOICES?slots[slot].main.lfo.phase:0;}
uint32_t core_note_phase(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return slots[i].main.lfo.phase;return 0;}
float core_voice_pitch(unsigned slot){return slot<VOICES&&live(&slots[slot].main)?slots[slot].main.pitch_cents:-2000.0f;}
float core_note_pitch(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return slots[i].main.pitch_cents;return -2000.0f;}
int32_t core_note_pitch_scale(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return slots[i].main.osc.pitch_scale_q12;return 0;}
float core_note_performance(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return slots[i].main.performance_cents;return 0.0f;}
uint32_t core_note_increment(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return (uint32_t)(((uint64_t)(uint32_t)slots[i].main.osc.frequency_increment*(uint32_t)slots[i].main.osc.pitch_scale_q12)>>12);return 0;}
unsigned core_note_sustained(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return slots[i].main.sustained;return 0;}
unsigned core_sustain(unsigned port,unsigned channel){return port<3&&channel<16?controls[port][channel].sustain:0;}
float core_voice_wave(unsigned slot){return slot<VOICES&&live(&slots[slot].main)?(float)slots[slot].main.osc.position_q8*(100.0f/1791.0f):-1.0f;}
float core_note_wave(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return (float)slots[i].main.osc.position_q8*(100.0f/1791.0f);return -1.0f;}
float core_voice_cutoff(unsigned slot){return slot<VOICES&&live(&slots[slot].main)?slots[slot].main.cutoff_index:-1.0f;}
float core_note_cutoff(unsigned port,unsigned channel,unsigned note){
 for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return slots[i].main.cutoff_index;
 return -1.0f;
}
static void advance_lfo(poly_voice *v){
 float raw=pw_lfo_tick(&v->lfo,(int32_t)lfo_increment);v->lfo_smoothed+=(raw-v->lfo_smoothed)*(1.0f/32.0f);
}
static float voice_sample(poly_voice *v,int advanced){
 if(filter_wet<=0.0f)pw_filter_reset(&v->filter);
 if(!live(v))return 0.0f;
 if(!advanced)advance_lfo(v);
 float movement=v->lfo_smoothed;
 float dry=0.0f,envelope=v->env.level;
 float wave_amp=v->env.level,wave_env2=v->env2.level;
 float envelope2=pw_env_tick(&v->env2);
 if(v->env.stage!=PW_ENV_IDLE){
  v->osc.position_q8=position;
  if(track_vel_wave!=0.0f||track_pitch_wave!=0.0f){
   float mod=(float)position+pw_tracking_wave_offset(v->note,v->velocity,track_vel_wave,track_pitch_wave)*1791.0f;
   mod+=(wave_env_depth*wave_amp+wave_env2_depth*wave_env2+lfo_wave_depth*movement)*(1791.0f/100.0f);
   if(mod<0.0f)mod=0.0f;else if(mod>1791.0f)mod=1791.0f;v->osc.position_q8=(int32_t)mod;
  }
  else if(wave_env_depth!=0.0f||wave_env2_depth!=0.0f)v->osc.position_q8=pw_wave_mod_position(position,wave_env_depth,wave_amp,wave_env2_depth,wave_env2,lfo_wave_depth,movement);
  else if(lfo_wave_depth>0.0f){float mod=(float)position+movement*lfo_wave_depth*(1791.0f/100.0f);if(mod<0.0f)mod=0.0f;else if(mod>1791.0f)mod=1791.0f;v->osc.position_q8=(int32_t)mod;}
  v->pitch_cents=wave_env2*env2_pitch_depth*10.0f+movement*lfo_pitch_depth;
  if(env2_pitch_depth!=0.0f)v->osc.pitch_scale_q12=pw_env_pitch_scale(pw_env_pitch_units(wave_env2,env2_pitch_depth,movement*lfo_pitch_depth));
  else v->osc.pitch_scale_q12=lfo_pitch_depth>0.0f?pw_lfo_pitch_scale(movement*lfo_pitch_depth):4096;
  float bend_target=v->port<3?pw_performance_bend(controls[v->port][v->channel].bend,(unsigned)(2+st.bend_range_offset)):0.0f;
  v->bend_cents+=(bend_target-v->bend_cents)*(1.0f/64.0f);
  float bend_error=bend_target-v->bend_cents;if(bend_error<.001f&&bend_error> -.001f)v->bend_cents=bend_target;
  v->performance_cents=tuning_cents+(float)v->key_octave*1200.0f+v->bend_cents;
  v->osc.frequency_increment=(int32_t)INC[v->note-24];
  if(v->performance_cents!=0.0f){v->osc.frequency_increment=pw_performance_increment(INC[v->note-24],pw_performance_units(v->performance_cents+v->pitch_cents));v->osc.pitch_scale_q12=4096;}
  int32_t sample=pw_core_tick(&v->osc,&drone_wt[active_bank][0][0],8,3)/256;
  envelope=pw_env_tick(&v->env);dry=(float)sample*(envelope*(float)v->velocity/127.0f);
 }
 v->cutoff_index=pw_filter_mod_index(filter_base,filter_depth,envelope);
 if(filter_depth2>0.0f)v->cutoff_index=pw_filter_mod_index(v->cutoff_index,filter_depth2,envelope2);
 if(lfo_filter_depth>0.0f)v->cutoff_index=pw_filter_mod_index(filter_base+filter_depth*envelope+filter_depth2*envelope2+movement*lfo_filter_depth,0.0f,0.0f);
 if(track_vel_cutoff!=0.0f||track_pitch_cutoff!=0.0f){
  float mod=filter_base+filter_depth*envelope+filter_depth2*envelope2+movement*lfo_filter_depth;
  mod+=pw_tracking_cutoff_offset(v->note,v->velocity,track_vel_cutoff,track_pitch_cutoff);
  v->cutoff_index=pw_filter_mod_index(mod,0.0f,0.0f);
 }
 if(filter_wet>0.0f){
  float g=(filter_depth>0.0f||filter_depth2>0.0f||lfo_filter_depth>0.0f||track_vel_cutoff!=0.0f||track_pitch_cutoff!=0.0f)?pw_filter_mod_g(v->cutoff_index):filter_g;
  pw_filter_tick(&v->filter,dry*(1.0f/32768.0f),g,pw_filter_damping(g,filter_q));
  float wet=pw_filter_output(&v->filter,filter_mode,dry*(1.0f/32768.0f))*32768.0f;
  dry+=(wet-dry)*filter_wet;
  if(v->env.stage==PW_ENV_IDLE&&!pw_filter_live(&v->filter))pw_filter_reset(&v->filter);
 }
 return dry;
}
void core_render(int32_t *lr,uint32_t n,uint32_t master){
 if(panic_requested){
  for(unsigned p=0;p<3;p++)for(unsigned c=0;c<16;c++)controls[p][c]=(midi_control){0};
  for(unsigned i=0;i<VOICES;i++){defaults(&slots[i].main);defaults(&slots[i].tail);slots[i].fade=0;}
  gain=0;processed_keys=panel_keys;audition_on=0;panic_requested=0;
 }
 uint32_t keys=panel_keys,changed=keys^processed_keys;
 for(unsigned k=0;k<27;k++)if((changed&(1u<<k))&&!(keys&(1u<<k)))note_off(PANEL_PORT,0,53+k);
 for(unsigned k=0;k<27;k++)if((changed&keys)&(1u<<k))note_on(PANEL_PORT,0,53+k,127);
 processed_keys=keys;
 const core_settings settings=st;
 if(audition_on&&(!settings.playing||settings.note!=audition_note))note_off(AUDITION_PORT,0,audition_note);
 if(settings.playing&&(!audition_on||settings.note!=audition_note))note_on(AUDITION_PORT,0,settings.note,127);
 audition_on=settings.playing;audition_note=settings.note;
 for(unsigned i=0;i<VOICES;i++){pw_env_shapes(&slots[i].main.env,64+settings.amp_attack_shape,64+settings.amp_decay_shape);pw_env_shapes(&slots[i].main.env2,64+settings.env2_attack_shape,64+settings.env2_decay_shape);pw_env_set(&slots[i].main.env,settings.env);pw_env_set(&slots[i].main.env2,settings.env2);}
 if(master>4096)master=4096;
 const int32_t target_gain=settings.bank==active_bank?(int32_t)(master*(uint32_t)settings.level/100u):0;
 const int32_t target_pos=(int32_t)settings.position*1791/100;
 const float target_g=pw_filter_g(settings.filter.cutoff),target_q=pw_filter_q(settings.filter.resonance);
 const float base_step=n?((float)settings.filter.cutoff-filter_base)/(float)n:0.0f,depth_step=n?((float)settings.filter_env_depth-filter_depth)/(float)n:0.0f;
 const float depth2_step=n?((float)settings.filter_env2_depth-filter_depth2)/(float)n:0.0f;
 const float increment_target=(float)pw_lfo_increment(settings.lfo.rate);
 const float inc_step=n?(increment_target-lfo_increment)/(float)n:0.0f;
 const float wave_step=n?((float)settings.lfo.wave-lfo_wave_depth)/(float)n:0.0f;
 const float lfo_filter_step=n?((float)settings.lfo.cutoff-lfo_filter_depth)/(float)n:0.0f;
 const float pitch_step=n?((float)settings.lfo.pitch-lfo_pitch_depth)/(float)n:0.0f;
 const float wave_env_step=n?((float)settings.wave_env_depth-wave_env_depth)/(float)n:0.0f;
 const float wave_env2_step=n?((float)settings.wave_env2_depth-wave_env2_depth)/(float)n:0.0f;
 const float tvw_step=n?((float)settings.tracking.velocity_wave-track_vel_wave)/(float)n:0.0f;
 const float tpw_step=n?((float)settings.tracking.pitch_wave-track_pitch_wave)/(float)n:0.0f;
 const float tvc_step=n?((float)settings.tracking.velocity_cutoff-track_vel_cutoff)/(float)n:0.0f;
 const float tpc_step=n?((float)settings.tracking.pitch_cutoff-track_pitch_cutoff)/(float)n:0.0f;
 const float tuning_target=(float)settings.coarse_tune*100.0f+(float)settings.fine_tune;
 const float tuning_step=n?(tuning_target-tuning_cents)/(float)n:0.0f;
 const float env2_pitch_step=n?((float)settings.env2_pitch-env2_pitch_depth)/(float)n:0.0f;
 const float g_step=n?(target_g-filter_g)/(float)n:0.0f,q_step=n?(target_q-filter_q)/(float)n:0.0f;
 for(uint32_t j=0;j<n;j++){
  tuning_cents+=tuning_step;env2_pitch_depth+=env2_pitch_step;track_vel_wave+=tvw_step;track_pitch_wave+=tpw_step;track_vel_cutoff+=tvc_step;track_pitch_cutoff+=tpc_step;
  wave_env_depth+=wave_env_step;wave_env2_depth+=wave_env2_step;
  lfo_increment+=inc_step;lfo_wave_depth+=wave_step;lfo_filter_depth+=lfo_filter_step;lfo_pitch_depth+=pitch_step;
  filter_g+=g_step;filter_q+=q_step;filter_base+=base_step;filter_depth+=depth_step;filter_depth2+=depth2_step;
  float target_wet=filter_mode==settings.filter.mode&&filter_mode!=PW_FILTER_OFF?(float)settings.filter.mix*0.01f:0.0f;
  if(filter_wet<target_wet){filter_wet+=1.0f/256.0f;if(filter_wet>target_wet)filter_wet=target_wet;}
  else if(filter_wet>target_wet){filter_wet-=1.0f/256.0f;if(filter_wet<target_wet)filter_wet=target_wet;}
  if(filter_wet==0.0f)filter_mode=settings.filter.mode;
  if(position<target_pos)position++;else if(position>target_pos)position--;
  int32_t delta=target_gain-gain,step=delta/256;if(!step&&delta)step=delta>0?1:-1;gain+=step;
  if(!gain)active_bank=settings.bank;
  float mix=0.0f;
  for(unsigned i=0;i<VOICES;i++){
   voice_slot *s=&slots[i];int running=settings.lfo_phase_mode!=PW_LFO_KEEP;if(running)advance_lfo(&s->main);float sample=voice_sample(&s->main,running);
   if(s->fade){float f=(float)s->fade/(float)STEAL_FADE;sample=sample*(1.0f-f)+voice_sample(&s->tail,0)*(f*s->tail_weight);s->fade--;}
   mix+=sample;
  }
  /* Four-voice fixed headroom: no gain pumping as release tails finish.
   * Slot crossfades are convex, keeping their summed peak bounded. */
  float output=mix*((float)gain*(256.0f/(4096.0f*4.0f)));
  /* Source also saturates final PCM; resonant gain can exceed unity. */
  if(output>8388607.0f)output=8388607.0f;else if(output< -8388608.0f)output= -8388608.0f;
  int32_t out=(int32_t)output;
  lr[2*j]=lr[2*j+1]=out;
 }
 if(n){tuning_cents=tuning_target;filter_base=(float)settings.filter.cutoff;filter_depth=(float)settings.filter_env_depth;filter_depth2=(float)settings.filter_env2_depth;lfo_increment=increment_target;lfo_wave_depth=(float)settings.lfo.wave;lfo_filter_depth=(float)settings.lfo.cutoff;lfo_pitch_depth=(float)settings.lfo.pitch;wave_env_depth=(float)settings.wave_env_depth;wave_env2_depth=(float)settings.wave_env2_depth;track_vel_wave=(float)settings.tracking.velocity_wave;track_pitch_wave=(float)settings.tracking.pitch_wave;track_vel_cutoff=(float)settings.tracking.velocity_cutoff;track_pitch_cutoff=(float)settings.tracking.pitch_cutoff;env2_pitch_depth=(float)settings.env2_pitch;}
}

int32_t core_wave_sample(uint32_t index){
    if(index>255)index=255;
    int32_t pos=(int32_t)st.position*1791/100;
    uint32_t frame=(uint32_t)pos>>8;int32_t mix=pos&255;
    int32_t a=drone_wt[st.bank][frame][index],b=drone_wt[st.bank][frame+1][index];
    return a+(((b-a)*mix)>>8);
}
