#include "pw_arp.h"
#include "test_engine.h"
#include "pw_core.h"
#include "akwf_tables.h"
#include "pw_filter_mod.h"
#include "pw_wave_mod.h"
#include "pw_env_pitch.h"
#include "pw_performance.h"
#include "pw_mip.h"
#include "pw_mip_tables.h"
#include "pw_chorus.h"
#include "pw_stereo.h"
#include "pw_unison.h"
#include "pw_delay.h"
#include "pw_gain.h"
#include "pw_velocity.h"
#include "pw_chord.h"
#include "pw_break.h"
#include "plat.h"
#define VOICES 4u
#define STEAL_FADE 128u
#define PANEL_PORT 254u
#define AUDITION_PORT 255u
static volatile core_settings st;
static volatile uint32_t panel_keys;
static volatile uint8_t panic_requested;
static uint32_t processed_keys,serial;
static uint8_t break_previous;
static int32_t gain,position;
static uint8_t active_bank,audition_on,audition_note,filter_mode;
static float env2_pitch_depth,tuning_cents,velocity_depth,velocity_blend;
static pw_chorus chorus;
static float delay_memory[PW_DELAY_FRAMES][2] PLAT_POOL;
static pw_delay delay;
static unsigned delay_active,delay_style_active;static float delay_mix,delay_feedback,synth_gain;
static uint8_t chorus_active;
static float chorus_mix,output_peaks[2];
typedef struct {int16_t bend;uint8_t sustain;} midi_control;
static midi_control controls[3][16];
static float track_vel_wave,track_pitch_wave,track_vel_cutoff,track_pitch_cutoff;
static float wave_env_depth,wave_env2_depth,unison_wave_depth;
static float lfo_wave_depth,lfo_filter_depth,lfo_pitch_depth,lfo_increment;
static float filter_wet,filter_g,filter_q,filter_base,filter_depth,filter_depth2;
typedef struct {
 pw_core_voice osc;
 pw_env env,env2;
 pw_filter filter;
 pw_lfo lfo;
 float lfo_smoothed;
 float cutoff_index,pitch_cents,bend_cents,performance_cents;
 float pan_left,pan_right,pan_target_left,pan_target_right;
 float detune_units,detune_target;
 uint8_t layers,member,slot;
 int8_t key_octave;
 uint32_t age,wait_samples;
 uint8_t port,channel,note,velocity,held,sustained,root;
 uint8_t mip_from,mip_to,mip_fade,mip_ready;
} poly_voice;
typedef struct {poly_voice main,tail;uint32_t fade;float tail_weight;} voice_slot;
static voice_slot slots[VOICES];
static const uint32_t INC[60]={3185015,3374406,3575058,3787642,4012867,4251485,4504291,4772130,5055896,5356535,5675051,6012507,6370030,6748811,7150117,7575285,8025735,8502970,9008582,9544261,10111792,10713070,11350103,12025015,12740059,13497623,14300233,15150569,16051469,17005939,18017165,19088521,20223584,21426141,22700205,24050030,25480119,26995246,28600467,30301139,32102938,34011878,36034330,38177043,40447168,42852281,45400411,48100060,50960238,53990491,57200933,60602278,64205876,68023757,72068660,76354085,80894335,85704563,90800821,96200119};

typedef struct {uint8_t port,channel,note,velocity,held;} arp_root;
static arp_root arp_roots[16];
static pw_arp_random arp_random;
static uint8_t arp_count,arp_mode,arp_step_index,arp_gate,arp_port,arp_channel,arp_pitch,arp_emitting;
static uint32_t arp_left,arp_gate_left,arp_remainder,arp_events;
static uint16_t arp_tempo;
static uint8_t arp_source,arp_clock_running,arp_clock_phase,arp_clock_gate_ticks;
static uint32_t arp_clock_age;
static int live(const poly_voice *v){return v->env.stage!=PW_ENV_IDLE||pw_filter_live(&v->filter);}
static void defaults(poly_voice *v){*v=(poly_voice){0};pw_env_init(&v->env);pw_env_init(&v->env2);pw_env_set(&v->env2,(pw_env_params){60,70,64,50});v->osc.pitch_scale_q12=4096;v->pan_left=v->pan_right=v->pan_target_left=v->pan_target_right=1.0f;}
void core_init(void){
 arp_source=arp_clock_running=arp_clock_phase=arp_clock_gate_ticks=0;arp_clock_age=0;
 arp_random=(pw_arp_random){0,{0,0},255};arp_count=arp_mode=arp_gate=arp_emitting=0;arp_step_index=255;arp_random=(pw_arp_random){0,{0,0},255};arp_left=arp_gate_left=arp_remainder=arp_events=arp_tempo=0;
 st=(core_settings){0,48,0,0,75,{50,60,80,60},{PW_FILTER_OFF,96,0,100},0,{60,70,64,50},0,{40,0,0,0},0,0,{0,0,0,0},PW_LFO_KEEP,0,0};
 st.arp_bpm=120;st.arp_octaves=1;st.arp_gate_percent=50;
 pw_delay_init(&delay,delay_memory,PW_DELAY_FRAMES);pw_delay_reset(&delay,22051.0f);delay_active=delay_style_active=0;synth_gain=1.0f;st.gain_db=0;st.delay_return_db= -3;st.delay_style=0;delay_mix=delay_feedback=0.0f;st.delay_time_ms=500;st.delay_feedback=50;st.delay_mix=35;
 st.velocity_amount=100;velocity_depth=100.0f;velocity_blend=0.0f;
 st.unison_detune=10;unison_wave_depth=0.0f;
 st.wave_quality=1;st.chorus_mix=50;pw_chorus_reset(&chorus);chorus_active=0;chorus_mix=0.0f;output_peaks[0]=output_peaks[1]=0.0f;
 panel_keys=processed_keys=serial=panic_requested=0;break_previous=0;gain=position=active_bank=audition_on=0;audition_note=48;filter_mode=PW_FILTER_OFF;filter_wet=0.0f;filter_g=pw_filter_g(96);filter_q=pw_filter_q(0);filter_base=96.0f;filter_depth=filter_depth2=0.0f;pw_filter_mod_init();env2_pitch_depth=0.0f;track_vel_wave=track_pitch_wave=track_vel_cutoff=track_pitch_cutoff=0.0f;wave_env_depth=wave_env2_depth=0.0f;lfo_wave_depth=lfo_filter_depth=lfo_pitch_depth=0.0f;lfo_increment=(float)pw_lfo_increment(40);
 tuning_cents=0.0f;for(unsigned p=0;p<3;p++)for(unsigned c=0;c<16;c++)controls[p][c]=(midi_control){0};
 for(unsigned i=0;i<VOICES;i++){defaults(&slots[i].main);defaults(&slots[i].tail);slots[i].fade=0;}
}
void core_set(const core_settings *s){core_settings c=*s;if(c.env.sustain<0)c.env.sustain=0;if(c.lfo_pitch_cents< -1200)c.lfo_pitch_cents= -1200;if(c.lfo_pitch_cents>1200)c.lfo_pitch_cents=1200;if(c.note<24)c.note=24;if(c.note>83)c.note=83;if(c.position>100)c.position=100;if(c.bank>7)c.bank=7;if(c.level>100)c.level=100;if(c.filter.mode>3)c.filter.mode=3;if(c.filter.cutoff>127)c.filter.cutoff=127;if(c.filter.resonance>100)c.filter.resonance=100;if(c.filter.mix>100)c.filter.mix=100;if(c.filter_env_depth< -127)c.filter_env_depth= -127;if(c.filter_env2_depth< -127)c.filter_env2_depth= -127;if(c.lfo.rate>100)c.lfo.rate=100;if(c.lfo.rate< -100)c.lfo.rate= -100;if(c.lfo.wave>100)c.lfo.wave=100;if(c.lfo.wave< -100)c.lfo.wave= -100;if(c.lfo.cutoff< -127)c.lfo.cutoff= -127;if(c.lfo.pitch>100)c.lfo.pitch=100;if(c.lfo.pitch+c.lfo_pitch_cents>1200)c.lfo_pitch_cents=(int16_t)(1200-c.lfo.pitch);if(c.wave_env_depth>100)c.wave_env_depth=100;if(c.wave_env_depth< -100)c.wave_env_depth= -100;if(c.wave_env2_depth>100)c.wave_env2_depth=100;if(c.wave_env2_depth< -100)c.wave_env2_depth= -100;
 if(c.tracking.velocity_wave>100)c.tracking.velocity_wave=100;if(c.tracking.velocity_wave< -100)c.tracking.velocity_wave= -100;
 if(c.tracking.pitch_wave>100)c.tracking.pitch_wave=100;if(c.tracking.pitch_wave< -100)c.tracking.pitch_wave= -100;
 if(c.tracking.velocity_cutoff< -127)c.tracking.velocity_cutoff= -127;
 if(c.tracking.pitch_cutoff>100)c.tracking.pitch_cutoff=100;if(c.tracking.pitch_cutoff< -100)c.tracking.pitch_cutoff= -100;if(c.lfo_phase_mode>PW_LFO_OFF)c.lfo_phase_mode=PW_LFO_OFF;if(c.lfo_spread>90)c.lfo_spread=90;if(c.env2_pitch>120)c.env2_pitch=120;if(c.env2_pitch< -120)c.env2_pitch= -120;
 if(c.amp_attack_shape< -64)c.amp_attack_shape= -64;if(c.amp_attack_shape>63)c.amp_attack_shape=63;
 if(c.amp_decay_shape< -64)c.amp_decay_shape= -64;if(c.amp_decay_shape>63)c.amp_decay_shape=63;
 if(c.env2_attack_shape< -64)c.env2_attack_shape= -64;if(c.env2_attack_shape>63)c.env2_attack_shape=63;
 if(c.env2_decay_shape< -64)c.env2_decay_shape= -64;if(c.env2_decay_shape>63)c.env2_decay_shape=63;if(c.coarse_tune< -60)c.coarse_tune= -60;if(c.coarse_tune>60)c.coarse_tune=60;
 if(c.fine_tune< -100)c.fine_tune= -100;if(c.fine_tune>100)c.fine_tune=100;
 if(c.panel_octave< -2)c.panel_octave= -2;if(c.panel_octave>2)c.panel_octave=2;
 if(c.bend_range_offset< -2)c.bend_range_offset= -2;if(c.bend_range_offset>46)c.bend_range_offset=46;if(c.wave_quality>1)c.wave_quality=1;if(c.chorus_mode>2)c.chorus_mode=2;if(c.chorus_mix>100)c.chorus_mix=100;if(c.stereo_mode>3)c.stereo_mode=3;if(c.stereo_spread< -100)c.stereo_spread= -100;if(c.stereo_spread>100)c.stereo_spread=100;if(c.unison_mode>2)c.unison_mode=2;if(c.unison_detune>100)c.unison_detune=100;if(c.unison_wave_spread< -100)c.unison_wave_spread= -100;if(c.unison_wave_spread>100)c.unison_wave_spread=100;if(c.gain_db< -40)c.gain_db= -40;if(c.gain_db>24)c.gain_db=24;if(c.delay_return_db< -40)c.delay_return_db= -40;if(c.delay_return_db>0)c.delay_return_db=0;if(c.delay_style>1)c.delay_style=1;if(c.delay_mode>2)c.delay_mode=2;if(c.delay_feedback>90)c.delay_feedback=90;if(c.delay_mix>100)c.delay_mix=100;if(c.delay_time_ms<1)c.delay_time_ms=1;if(c.delay_time_ms>550)c.delay_time_ms=550;if(c.velocity_mode>1)c.velocity_mode=1;if(c.velocity_amount>100)c.velocity_amount=100;if(c.velocity_curve>3)c.velocity_curve=3;if(c.trigger_mode>1)c.trigger_mode=1;if(c.chord_shape>14)c.chord_shape=14;if(c.chord_scale>8)c.chord_scale=8;if(c.chord_key< -12)c.chord_key= -12;if(c.chord_key>12)c.chord_key=12;if(c.chord_inversion>3)c.chord_inversion=3;if(c.break_time_ms>1000)c.break_time_ms=1000;if(c.break_direction>2)c.break_direction=2;if(c.sustain_mode>2)c.sustain_mode=2;if(c.voice_limit>4)c.voice_limit=4;if(c.sustain_hold>1)c.sustain_hold=1;if(c.arp_clock>2)c.arp_clock=2;if(c.arp_mode>14)c.arp_mode=14;if(st.arp_mode&&!c.arp_mode)c.playing=0;if(c.arp_bpm<40)c.arp_bpm=120;if(c.arp_bpm>240)c.arp_bpm=240;if(c.arp_octaves<1)c.arp_octaves=1;if(c.arp_octaves>3)c.arp_octaves=3;if(c.arp_gate_percent<10)c.arp_gate_percent=50;if(c.arp_gate_percent>90)c.arp_gate_percent=90;st=c;}
void core_keys(uint32_t keys){panel_keys=keys&0x7ffffffu;}
static int matches(const poly_voice *v,unsigned port,unsigned channel,unsigned note){return live(v)&&v->port==port&&v->channel==channel&&v->root==note;}
/* The source combines its global sustain parameter with the pedal state.
   FM1 keeps pedals scoped per MIDI port/channel, and adds panel keys. */
static int sustain_active(unsigned port,unsigned channel){return (port!=AUDITION_PORT&&st.sustain_hold)||(port<3&&controls[port][channel].sustain);}
static void reconcile_sustain(void){
 for(unsigned i=0;i<VOICES;i++){
  poly_voice *v=&slots[i].main;
  if(v->sustained&&!sustain_active(v->port,v->channel)){
   v->sustained=0;if(!v->held){pw_env_off(&v->env);pw_env_off(&v->env2);}
  }
 }
}
static void voice_note_off(unsigned port,unsigned channel,unsigned note){
 for(unsigned i=0;i<VOICES;i++){
  poly_voice *v=&slots[i].main;
  if(matches(v,port,channel,note)&&v->held){if(v->wait_samples){defaults(v);continue;}v->held=0;if(v->env.triggered){v->sustained=0;continue;}if(sustain_active(port,channel))v->sustained=1;else{v->sustained=0;pw_env_off(&v->env);pw_env_off(&v->env2);}}
 }
}
static void note_on_at(unsigned port,unsigned channel,unsigned note,unsigned velocity,unsigned forced,unsigned root){
 if(note<24||note>83||!velocity)return;
 uint32_t phases[VOICES];int any_held=0;
 for(unsigned i=0;i<VOICES;i++){phases[i]=slots[i].main.lfo.phase;unsigned stage=slots[i].main.env.stage;if(stage>=PW_ENV_SUSTAIN&&stage<=PW_ENV_DECAY)any_held=1;}
 unsigned chosen=forced<VOICES?forced:VOICES;
 /* Repeated note-on retriggers its own slot, preserving phase and level. */
 if(chosen==VOICES)for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,root)){chosen=i;break;}
 int fresh=forced<VOICES?!(matches(&slots[chosen].main,port,channel,root)&&slots[chosen].main.note==note):chosen==VOICES;
 if(chosen==VOICES){
  for(unsigned i=0;i<VOICES;i++)if(!live(&slots[i].main)&&!slots[i].fade){chosen=i;break;}
  if(chosen==VOICES){
   /* Prefer the quietest released voice, then the oldest held voice. */
   float quiet=2.0f;
   for(unsigned i=0;i<VOICES;i++)if(!slots[i].main.held&&!slots[i].main.sustained&&slots[i].main.env.level<quiet){chosen=i;quiet=slots[i].main.env.level;}
   if(chosen==VOICES){for(unsigned i=0;i<VOICES;i++)if(slots[i].main.sustained&&(chosen==VOICES||(uint32_t)(serial-slots[i].main.age)>(uint32_t)(serial-slots[chosen].main.age)))chosen=i;}
   if(chosen==VOICES){chosen=0;for(unsigned i=1;i<VOICES;i++)if((uint32_t)(serial-slots[i].main.age)>(uint32_t)(serial-slots[chosen].main.age))chosen=i;}
  }
 }
 if(fresh){
  voice_slot *slot=&slots[chosen];
  if(live(&slot->main)&&!slot->main.wait_samples&&(!slot->fade||slot->main.env.level>0.0f)){
   /* Preserve an interrupted crossfade's approximate current amplitude. */
   slot->tail_weight=1.0f-(float)slot->fade/(float)STEAL_FADE;
   slot->tail=slot->main;slot->fade=STEAL_FADE;
  }
  defaults(&slot->main);
 }
 pw_lfo_assign_phase(phases,VOICES,chosen,st.lfo_phase_mode,st.lfo_spread,any_held,fresh);
 for(unsigned i=0;i<VOICES;i++)slots[i].main.lfo.phase=phases[i];
 poly_voice *v=&slots[chosen].main;
 v->wait_samples=0;v->layers=1;v->member=0;v->slot=(uint8_t)chosen;
 v->port=(uint8_t)port;v->channel=(uint8_t)channel;v->note=(uint8_t)note;v->root=(uint8_t)root;v->velocity=(uint8_t)velocity;v->held=1;v->sustained=0;v->age=++serial;v->key_octave=port==PANEL_PORT?st.panel_octave:0;
 v->bend_cents=port<3?pw_performance_bend(controls[port][channel].bend,(unsigned)(2+st.bend_range_offset)):0.0f;
 pw_stereo_gains(st.stereo_mode,st.stereo_spread,chosen,VOICES,(int)note,&v->pan_target_left,&v->pan_target_right);if(fresh){v->pan_left=v->pan_target_left;v->pan_right=v->pan_target_right;}
 v->osc.frequency_increment=(int32_t)INC[note-24];v->osc.position_q8=position;
 pw_env_shapes(&v->env,64+st.amp_attack_shape,64+st.amp_decay_shape);pw_env_shapes(&v->env2,64+st.env2_attack_shape,64+st.env2_decay_shape);
 pw_env_trigger_mode(&v->env,st.trigger_mode);pw_env_trigger_mode(&v->env2,st.trigger_mode);
 pw_env_set(&v->env,st.env);pw_env_on(&v->env);pw_env_set(&v->env2,st.env2);pw_env_on(&v->env2);
}
static void retire_group(unsigned port,unsigned channel,unsigned note){
 for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note)){
  voice_slot *s=&slots[i];
  if(!s->main.wait_samples&&(!s->fade||s->main.env.level>0.0f)){s->tail_weight=1.0f-(float)s->fade/(float)STEAL_FADE;s->tail=s->main;s->fade=STEAL_FADE;}
  defaults(&s->main);
 }
}
static unsigned voice_budget(void){return st.voice_limit?st.voice_limit:VOICES;}
unsigned core_primary_count(void){unsigned n=0;for(unsigned i=0;i<VOICES;i++)if(live(&slots[i].main))n++;return n;}
static int retire_one_group(void){
   unsigned victim=VOICES,best_rank=3;float best_quiet=100.0f;uint32_t best_age=0;
   for(unsigned i=0;i<VOICES;i++){
    poly_voice *v=&slots[i].main;if(!live(v))continue;
    unsigned rank=v->held?2:v->sustained?1:0;float quiet=0.0f;uint32_t age=0;
    for(unsigned j=0;j<VOICES;j++){poly_voice *peer=&slots[j].main;if(matches(peer,v->port,v->channel,v->root)){quiet+=peer->env.level;uint32_t a=serial-peer->age;if(a>age)age=a;}}
    if(victim==VOICES||rank<best_rank||(rank==best_rank&&((!rank&&quiet<best_quiet)||(rank&&age>best_age)))){victim=i;best_rank=rank;best_quiet=quiet;best_age=age;}
   }

 if(victim==VOICES)return 0;
 poly_voice *v=&slots[victim].main;retire_group(v->port,v->channel,v->root);return 1;
}
static void enforce_budget(void){while(core_primary_count()>voice_budget())if(!retire_one_group())break;}
static void voice_note_on(unsigned port,unsigned channel,unsigned note,unsigned velocity){
 if(note<24||note>83||!velocity)return;
 enforce_budget();
 /* Source sustain response applies only to re-striking a pedal-held note.
    Root ownership keeps FM1 chord/unison groups coherent across note-offs. */
 if(st.sustain_mode&&sustain_active(port,channel)){
  unsigned sustained=0;for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note)&&slots[i].main.sustained)sustained=1;
  if(sustained){
   if(st.sustain_mode==1){for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main;if(matches(v,port,channel,note)){v->held=1;v->sustained=0;}}return;}
   retire_group(port,channel,note);
  }
 }
 uint8_t pitches[5];unsigned chord=st.chord_shape!=0&&!arp_emitting,count=1u<<st.unison_mode,existing[VOICES],found=0,stacked=0;
 if(chord){unsigned n=pw_chord_notes(note,st.chord_key,st.chord_scale,pw_chord_shape(st.chord_shape),st.chord_inversion,pitches);count=0;for(unsigned k=0;k<n;k++)if(pitches[k]>=24&&pitches[k]<=83)pitches[count++]=pitches[k];}
 unsigned budget=voice_budget();if(chord){count=pw_chord_fit(pitches,count,budget);}else while(count>budget)count>>=1;
 if(!count){retire_group(port,channel,note);return;}
 for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main;if(matches(v,port,channel,note))existing[found++]=i;if(live(v)&&(v->layers>1||v->note!=v->root))stacked=1;}
 if(count==1&&!stacked&&!chord&&budget==VOICES){note_on_at(port,channel,note,velocity,VOICES,note);return;}
 unsigned strum=chord&&count>1&&st.break_time_ms;
 if(strum&&found){retire_group(port,channel,note);found=0;}
 if(found&&found!=count){retire_group(port,channel,note);found=0;}
 if(!found){
  for(;;){
   unsigned used=core_primary_count(),free=used<budget?budget-used:0;
   if(free>=count)break;
   if(!retire_one_group())return;
  }
  if(count==2){if(!live(&slots[0].main)&&!live(&slots[3].main)){existing[0]=0;existing[1]=3;found=2;}else if(!live(&slots[1].main)&&!live(&slots[2].main)){existing[0]=1;existing[1]=2;found=2;}}
  if(!found)for(unsigned i=0;i<VOICES&&found<count;i++)if(!live(&slots[i].main))existing[found++]=i;
 }
 unsigned direction=0;if(strum){direction=pw_break_direction(st.break_direction,break_previous);break_previous=(uint8_t)direction;}
 for(unsigned k=0;k<count;k++){
  unsigned slot=existing[k];int retrigger=matches(&slots[slot].main,port,channel,note);
  unsigned pitch=chord?pitches[direction?count-1-k:k]:note;
  note_on_at(port,channel,pitch,velocity,slot,note);poly_voice *v=&slots[slot].main;v->layers=(uint8_t)(chord?1:count);v->member=(uint8_t)(chord?0:k);
  v->detune_target=(float)pw_unison_detune(v->layers,v->member,st.unison_detune);if(!retrigger)v->detune_units=v->detune_target;
  if(strum)v->wait_samples=pw_break_delay(st.break_time_ms,k);
 }
}
/* A single FM1 internal-clock lane. Root ownership survives sorting/chords. */
static void arp_release_all(void){
 for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main;if(v->wait_samples){defaults(v);continue;}v->held=v->sustained=0;pw_env_off(&v->env);pw_env_off(&v->env2);}
 arp_gate=0;
}
static void arp_gate_off(void){
 if(arp_gate)for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main;if(matches(v,arp_port,arp_channel,arp_pitch)){v->held=v->sustained=0;if(!v->env.triggered){pw_env_off(&v->env);pw_env_off(&v->env2);}}}
 arp_gate=0;
}
static void arp_sync(void){
 if(arp_mode==st.arp_mode&&arp_source==st.arp_clock)return;
 arp_source=st.arp_clock;arp_clock_running=arp_clock_phase=0;arp_clock_age=0;
 arp_release_all();arp_left=arp_gate_left=arp_remainder=0;arp_step_index=255;arp_random=(pw_arp_random){0,{0,0},255};
 if(!arp_mode||!st.arp_mode){arp_count=0;audition_on=0;}
 arp_mode=st.arp_mode;
}
static void arp_remove(unsigned index){
 for(unsigned i=index+1;i<arp_count;i++)arp_roots[i-1]=arp_roots[i];
 --arp_count;
 if(!arp_count){arp_release_all();arp_left=arp_gate_left=arp_remainder=0;arp_step_index=255;arp_random=(pw_arp_random){0,{0,0},255};}
}
static void arp_reconcile(void){
 for(unsigned i=0;i<arp_count;)if(!arp_roots[i].held&&!sustain_active(arp_roots[i].port,arp_roots[i].channel))arp_remove(i);else ++i;
}
static void note_on(unsigned port,unsigned channel,unsigned note,unsigned velocity){
 arp_sync();if(!arp_mode){voice_note_on(port,channel,note,velocity);return;}
 if(note<24||note>83||!velocity)return;
 for(unsigned i=0;i<arp_count;i++)if(arp_roots[i].port==port&&arp_roots[i].channel==channel&&arp_roots[i].note==note){arp_roots[i].velocity=velocity;arp_roots[i].held=1;return;}
 if(arp_count==16)arp_remove(0);
 arp_roots[arp_count++]=(arp_root){port,channel,note,velocity,1};
}
static void note_off(unsigned port,unsigned channel,unsigned note){
 arp_sync();if(!arp_mode){voice_note_off(port,channel,note);return;}
 for(unsigned i=0;i<arp_count;i++)if(arp_roots[i].port==port&&arp_roots[i].channel==channel&&arp_roots[i].note==note){
  arp_roots[i].held=0;if(!sustain_active(port,channel))arp_remove(i);return;
 }
}
static void arp_channel_stop(unsigned port,unsigned channel){
 for(unsigned i=0;i<arp_count;)if(arp_roots[i].port==port&&arp_roots[i].channel==channel)arp_remove(i);else ++i;
 for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main;if(v->port==port&&v->channel==channel){v->held=v->sustained=0;pw_env_off(&v->env);pw_env_off(&v->env2);}}
}
static void arp_step_run(void){
  arp_gate_off();
  struct candidate {uint8_t note,owner;} list[16];unsigned count=0;
  for(unsigned i=0;i<arp_count&&count<16;i++){
   uint8_t pitches[5];unsigned n=1;pitches[0]=arp_roots[i].note;
   if(st.chord_shape)n=pw_chord_notes(arp_roots[i].note,st.chord_key,st.chord_scale,pw_chord_shape(st.chord_shape),st.chord_inversion,pitches);
   for(unsigned k=0;k<n&&count<16;k++)if(pitches[k]>=24&&pitches[k]<=83){
    unsigned at=count++;while(arp_mode!=7&&at&&list[at-1].note>pitches[k]){list[at]=list[at-1];--at;}list[at]=(struct candidate){pitches[k],i};
   }
  }
  if(arp_tempo!=st.arp_bpm){arp_tempo=st.arp_bpm;arp_remainder=0;}
  arp_left=pw_arp_interval(st.arp_bpm,&arp_remainder);arp_gate_left=arp_left*st.arp_gate_percent/100;
  if(count){unsigned selected=arp_mode>=11?pw_arp_octave_next(&arp_step_index,count,arp_mode,st.arp_octaves,pw_chord_omit_octave(pw_chord_shape(st.chord_shape))):arp_mode>=8?pw_arp_random_next(&arp_random,count,arp_mode,st.arp_octaves):pw_arp_next(&arp_step_index,count,arp_mode,st.arp_octaves);unsigned at=selected%count,oct=selected/count;
   unsigned pitch=list[at].note+12*oct;arp_root *root=&arp_roots[list[at].owner];
   /* Out-of-range steps are rests, preserving the recovered pattern rhythm. */
   if(pitch<=83){arp_port=root->port;arp_channel=root->channel;arp_pitch=pitch;arp_emitting=1;voice_note_on(arp_port,arp_channel,pitch,root->velocity);arp_emitting=0;arp_gate=1;++arp_events;}
  }
}
static void arp_clock_message(unsigned port,uint8_t status){
 if(!arp_mode||!arp_source||port!=(arp_source==1?0u:2u))return;
 if(status==0xfa){
  arp_release_all();arp_step_index=255;arp_random=(pw_arp_random){0,{0,0},255};
  arp_clock_phase=0;arp_clock_running=1;arp_clock_age=0;
 }else if(status==0xfb){arp_clock_running=1;arp_clock_age=0;}
 else if(status==0xfc){arp_clock_running=0;arp_release_all();}
 else if(status==0xf8&&arp_clock_running){
  arp_clock_age=0;
  if(!arp_clock_phase){
   arp_clock_gate_ticks=(uint8_t)((12u*st.arp_gate_percent+99u)/100u);
   if(arp_count)arp_step_run();
  }else if(arp_clock_phase>=arp_clock_gate_ticks)arp_gate_off();
  arp_clock_phase=(uint8_t)((arp_clock_phase+1u)%12u);
 }
}
static void arp_tick(void){
 if(arp_source){
  if(arp_clock_running&&++arp_clock_age>=44100u){arp_clock_running=0;arp_release_all();}
  return;
 }
 if(!arp_mode||!arp_count)return;
 if(!arp_left)arp_step_run();else if(!arp_gate_left)arp_gate_off();
 --arp_left;if(arp_gate_left)--arp_gate_left;
}
unsigned core_arp_clock_running(void){return arp_clock_running;}
unsigned core_arp_count(void){return arp_count;}
unsigned core_arp_note(void){return arp_gate?arp_pitch:0;}

static void pedal_up(unsigned port,unsigned channel){
 controls[port][channel].sustain=0;
 if(sustain_active(port,channel))return;
 for(unsigned i=0;i<VOICES;i++){
  poly_voice *v=&slots[i].main;
  if(v->port==port&&v->channel==channel&&v->sustained){v->sustained=0;if(!v->held){pw_env_off(&v->env);pw_env_off(&v->env2);}}
 }
}
void core_midi(unsigned port,uint8_t status,uint8_t note,uint8_t velocity){
 if(port>2)return;arp_sync();if(status>=0xf8){arp_clock_message(port,status);return;}unsigned channel=status&15,command=status&240;note&=127;velocity&=127;
 if(command==0x90&&velocity)note_on(port,channel,note,pw_velocity_map(velocity,1,st.velocity_curve));
 else if(command==0x80||command==0x90)note_off(port,channel,note);
 else if(command==0xe0)controls[port][channel].bend=(int16_t)((int)((unsigned)velocity<<7|note)-8192);
 else if(command==0xb0){
  if(note==64){if(velocity>=64)controls[port][channel].sustain=1;else pedal_up(port,channel);}
  else if(note==121){controls[port][channel].bend=0;pedal_up(port,channel);}
  else if(note==120||note==123){
   if(arp_mode)arp_channel_stop(port,channel);
   for(unsigned i=0;i<VOICES;i++){
    voice_slot *s=&slots[i];
    if(s->main.port==port&&s->main.channel==channel){
     if(note==120)defaults(&s->main);
     else if(s->main.wait_samples){defaults(&s->main);}
     else if(s->main.env.triggered){s->main.held=s->main.sustained=0;pw_env_off(&s->main.env);pw_env_off(&s->main.env2);}
     else if(s->main.held)note_off(port,channel,s->main.root);
    }
    if(note==120&&s->tail.port==port&&s->tail.channel==channel){s->fade=0;defaults(&s->tail);}
   }
  }
 }
}
void core_panic(void){st.playing=0;panic_requested=1;}
static const poly_voice *latest(unsigned held_only){
 const poly_voice *v=0;
 for(unsigned i=0;i<VOICES;i++)if(live(&slots[i].main)&&!slots[i].main.wait_samples&&(!held_only||slots[i].main.held))
  if(!v||(uint32_t)(serial-slots[i].main.age)<(uint32_t)(serial-v->age))v=&slots[i].main;
 return v;
}
unsigned core_active_note(void){const poly_voice *v=latest(1);return v?v->note:0;}
unsigned core_env_stage(void){const poly_voice *v=latest(0);return v?v->env.stage:0;}
float core_env_level(void){float level=0;for(unsigned i=0;i<VOICES;i++)if(slots[i].main.env.level>level)level=slots[i].main.env.level;return level;}
unsigned core_voice_count(void){unsigned n=0;for(unsigned i=0;i<VOICES;i++)if(live(&slots[i].main)||slots[i].fade)n++;return n;}
unsigned core_held_count(void){unsigned n=0;for(unsigned i=0;i<VOICES;i++)if(slots[i].main.held&&live(&slots[i].main))n++;return n;}
unsigned core_note_held(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(slots[i].main.held&&matches(&slots[i].main,port,channel,note))return 1;return 0;}
float core_env2_level(void){float level=0;for(unsigned i=0;i<VOICES;i++)if(live(&slots[i].main)&&(slots[i].main.env2.level*slots[i].main.env2.level)>(level*level))level=slots[i].main.env2.level;return level;}
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
unsigned core_voice_mip(unsigned slot){return slot<VOICES&&live(&slots[slot].main)?(slots[slot].main.mip_fade?slots[slot].main.mip_to:slots[slot].main.mip_from):0;}
unsigned core_note_mip(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return core_voice_mip(i);return 0;}
static const int16_t *mip_frames(unsigned mip){return mip?pw_mip_banks[mip-1][active_bank]:&drone_wt[active_bank][0][0];}
static int32_t wave_sample(poly_voice *v){
 unsigned desired=st.wave_quality?pw_mip_select((uint32_t)v->osc.frequency_increment,v->osc.pitch_scale_q12):0;
 if(!v->mip_ready){v->mip_from=v->mip_to=(uint8_t)desired;v->mip_ready=1;}
 if(!v->mip_fade&&desired!=v->mip_from){v->mip_to=(uint8_t)desired;v->mip_fade=64;}
 pw_core_voice second=v->osc;
 int32_t sample=pw_core_tick(&v->osc,mip_frames(v->mip_from),8,v->mip_from?v->mip_from:3)/256;
 if(v->mip_fade){
  int32_t next=pw_core_tick(&second,mip_frames(v->mip_to),8,v->mip_to?v->mip_to:3)/256;
  sample=(sample*(int32_t)v->mip_fade+next*(64-(int32_t)v->mip_fade))/64;
  if(!--v->mip_fade)v->mip_from=v->mip_to;
 }
 return sample;
}
static void advance_lfo(poly_voice *v){
 float raw=pw_lfo_tick(&v->lfo,(int32_t)lfo_increment);v->lfo_smoothed+=(raw-v->lfo_smoothed)*(1.0f/32.0f);
}
static float voice_sample(poly_voice *v,int advanced){
 if(v->wait_samples){v->wait_samples--;return 0.0f;}
 if(filter_wet<=0.0f)pw_filter_reset(&v->filter);
 if(!live(v))return 0.0f;
 if(!advanced)advance_lfo(v);
 float movement=v->lfo_smoothed;
 float dry=0.0f,envelope=v->env.level;
 float wave_amp=v->env.level,wave_env2=v->env2.level;
 float envelope2=pw_env_tick(&v->env2);
 if(v->env.stage!=PW_ENV_IDLE){
  v->osc.position_q8=position;
  if(track_vel_wave!=0.0f||track_pitch_wave!=0.0f||unison_wave_depth!=0.0f){
   float tracked=pw_tracking_wave_offset(v->note,v->velocity,track_vel_wave,track_pitch_wave);
   if(unison_wave_depth!=0.0f)tracked+=pw_unison_wave(v->slot,VOICES,unison_wave_depth);
   float mod=(float)position+tracked*1791.0f;
   mod+=(wave_env_depth*wave_amp+wave_env2_depth*wave_env2+lfo_wave_depth*movement)*(1791.0f/100.0f);
   if(mod<0.0f)mod=0.0f;else if(mod>1791.0f)mod=1791.0f;v->osc.position_q8=(int32_t)mod;
  }
  else if(wave_env_depth!=0.0f||wave_env2_depth!=0.0f)v->osc.position_q8=pw_wave_mod_position(position,wave_env_depth,wave_amp,wave_env2_depth,wave_env2,lfo_wave_depth,movement);
  else if(lfo_wave_depth!=0.0f){float mod=(float)position+movement*lfo_wave_depth*(1791.0f/100.0f);if(mod<0.0f)mod=0.0f;else if(mod>1791.0f)mod=1791.0f;v->osc.position_q8=(int32_t)mod;}
  v->pitch_cents=wave_env2*env2_pitch_depth*10.0f+movement*lfo_pitch_depth;
  if(env2_pitch_depth!=0.0f)v->osc.pitch_scale_q12=pw_env_pitch_scale(pw_env_pitch_units(wave_env2,env2_pitch_depth,movement*lfo_pitch_depth));
  else v->osc.pitch_scale_q12=lfo_pitch_depth!=0.0f?pw_lfo_pitch_scale(movement*lfo_pitch_depth):4096;
  float bend_target=v->port<3?pw_performance_bend(controls[v->port][v->channel].bend,(unsigned)(2+st.bend_range_offset)):0.0f;
  v->bend_cents+=(bend_target-v->bend_cents)*(1.0f/64.0f);
  float bend_error=bend_target-v->bend_cents;if(bend_error<.001f&&bend_error> -.001f)v->bend_cents=bend_target;
  v->performance_cents=tuning_cents+(float)v->key_octave*1200.0f+v->bend_cents;
  v->osc.frequency_increment=(int32_t)INC[v->note-24];
  float delta=v->detune_target-v->detune_units;v->detune_units+=delta*(1.0f/64.0f);if(delta<.5f&&delta> -.5f)v->detune_units=v->detune_target;
  if(v->detune_units!=0.0f){int32_t units=pw_performance_units(v->performance_cents+v->pitch_cents)+(int32_t)v->detune_units;v->osc.frequency_increment=pw_performance_increment(INC[v->note-24],units);v->osc.pitch_scale_q12=4096;}
  else if(v->performance_cents!=0.0f||lfo_pitch_depth>100.0f||lfo_pitch_depth< -100.0f){v->osc.frequency_increment=pw_performance_increment(INC[v->note-24],pw_performance_units(v->performance_cents+v->pitch_cents));v->osc.pitch_scale_q12=4096;}
  int32_t sample=wave_sample(v);
  envelope=pw_env_tick(&v->env);dry=(float)sample*(envelope*(float)v->velocity/127.0f);
  if(velocity_blend>0.0f){float legacy=(float)v->velocity/127.0f,source=pw_velocity_gain(v->velocity,velocity_depth);dry=(float)sample*(envelope*(legacy+(source-legacy)*velocity_blend));}
 }
 if(filter_depth<0.0f||filter_depth2<0.0f||lfo_filter_depth<0.0f||envelope2<0.0f){
  float tracking=pw_tracking_cutoff_offset(v->note,v->velocity,track_vel_cutoff,track_pitch_cutoff);
  v->cutoff_index=pw_filter_mod_signed(filter_base,tracking,filter_depth,envelope,filter_depth2,envelope2,lfo_filter_depth,movement);
 }else{
 v->cutoff_index=pw_filter_mod_index(filter_base,filter_depth,envelope);
 if(filter_depth2>0.0f)v->cutoff_index=pw_filter_mod_index(v->cutoff_index,filter_depth2,envelope2);
 if(lfo_filter_depth>0.0f)v->cutoff_index=pw_filter_mod_index(filter_base+filter_depth*envelope+filter_depth2*envelope2+movement*lfo_filter_depth,0.0f,0.0f);
 if(track_vel_cutoff!=0.0f||track_pitch_cutoff!=0.0f){
  float mod=filter_base+filter_depth*envelope+filter_depth2*envelope2+movement*lfo_filter_depth;
  mod+=pw_tracking_cutoff_offset(v->note,v->velocity,track_vel_cutoff,track_pitch_cutoff);
  v->cutoff_index=pw_filter_mod_index(mod,0.0f,0.0f);
 }
 }
 if(filter_wet>0.0f){
  float g=(filter_depth!=0.0f||filter_depth2!=0.0f||lfo_filter_depth!=0.0f||track_vel_cutoff!=0.0f||track_pitch_cutoff!=0.0f)?pw_filter_mod_g(v->cutoff_index):filter_g;
  pw_filter_tick(&v->filter,dry*(1.0f/32768.0f),g,pw_filter_damping(g,filter_q));
  float wet=pw_filter_output(&v->filter,filter_mode,dry*(1.0f/32768.0f))*32768.0f;
  dry+=(wet-dry)*filter_wet;
  if(v->env.stage==PW_ENV_IDLE&&!pw_filter_live(&v->filter))pw_filter_reset(&v->filter);
 }
 return dry;
}
void core_render(int32_t *lr,uint32_t n,uint32_t master){
 arp_sync();
 if(panic_requested){
  arp_clock_running=arp_clock_phase=0;arp_clock_age=0;
  arp_count=arp_gate=0;arp_left=arp_gate_left=arp_remainder=0;arp_step_index=255;arp_random=(pw_arp_random){0,{0,0},255};
  pw_delay_reset(&delay,(float)st.delay_time_ms*44.1f+1.0f);delay_active=0;delay_mix=0.0f;delay_feedback=0.0f;
  pw_chorus_reset(&chorus);chorus_active=0;chorus_mix=0.0f;
  for(unsigned p=0;p<3;p++)for(unsigned c=0;c<16;c++)controls[p][c]=(midi_control){0};
  for(unsigned i=0;i<VOICES;i++){defaults(&slots[i].main);defaults(&slots[i].tail);slots[i].fade=0;}
  break_previous=0;gain=0;processed_keys=panel_keys;audition_on=0;panic_requested=0;
 }
 arp_reconcile();
 reconcile_sustain();
 enforce_budget();
 uint32_t keys=panel_keys,changed=keys^processed_keys;
 for(unsigned k=0;k<27;k++)if((changed&(1u<<k))&&!(keys&(1u<<k)))note_off(PANEL_PORT,0,53+k);
 for(unsigned k=0;k<27;k++)if((changed&keys)&(1u<<k))note_on(PANEL_PORT,0,53+k,127);
 processed_keys=keys;
 const core_settings settings=st;
 output_peaks[0]=output_peaks[1]=0.0f;
 for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main,*t=&slots[i].tail;v->detune_target=(float)pw_unison_detune(v->layers,v->member,settings.unison_detune);t->detune_target=(float)pw_unison_detune(t->layers,t->member,settings.unison_detune);}
 for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main,*t=&slots[i].tail;pw_stereo_gains(settings.stereo_mode,settings.stereo_spread,i,VOICES,v->note,&v->pan_target_left,&v->pan_target_right);pw_stereo_gains(settings.stereo_mode,settings.stereo_spread,i,VOICES,t->note,&t->pan_target_left,&t->pan_target_right);}
 if(audition_on&&(!settings.playing||settings.note!=audition_note))note_off(AUDITION_PORT,0,audition_note);
 if(settings.playing&&(!audition_on||settings.note!=audition_note))note_on(AUDITION_PORT,0,settings.note,127);
 audition_on=settings.playing;audition_note=settings.note;
 for(unsigned i=0;i<VOICES;i++){pw_env_shapes(&slots[i].main.env,64+settings.amp_attack_shape,64+settings.amp_decay_shape);pw_env_shapes(&slots[i].main.env2,64+settings.env2_attack_shape,64+settings.env2_decay_shape);pw_env_set(&slots[i].main.env,settings.env);pw_env_set(&slots[i].main.env2,settings.env2);}
 if(master>4096)master=4096;
 const int32_t target_gain=settings.bank==active_bank?(int32_t)(master*(uint32_t)settings.level/100u):0;
 const float target_synth_gain=pw_gain_db(settings.gain_db),target_return=pw_gain_db(settings.delay_return_db);
 const int32_t target_pos=(int32_t)settings.position*1791/100;
 const float target_g=pw_filter_g(settings.filter.cutoff),target_q=pw_filter_q(settings.filter.resonance);
 const float base_step=n?((float)settings.filter.cutoff-filter_base)/(float)n:0.0f,depth_step=n?((float)settings.filter_env_depth-filter_depth)/(float)n:0.0f;
 const float depth2_step=n?((float)settings.filter_env2_depth-filter_depth2)/(float)n:0.0f;
 const float increment_target=(float)pw_lfo_increment(settings.lfo.rate);
 const float inc_step=n?(increment_target-lfo_increment)/(float)n:0.0f;
 const float wave_step=n?((float)settings.lfo.wave-lfo_wave_depth)/(float)n:0.0f;
 const float lfo_filter_step=n?((float)settings.lfo.cutoff-lfo_filter_depth)/(float)n:0.0f;
 const float pitch_step=n?(((float)settings.lfo.pitch+(float)settings.lfo_pitch_cents)-lfo_pitch_depth)/(float)n:0.0f;
 const float wave_env_step=n?((float)settings.wave_env_depth-wave_env_depth)/(float)n:0.0f;
 const float wave_env2_step=n?((float)settings.wave_env2_depth-wave_env2_depth)/(float)n:0.0f;
 const float tvw_step=n?((float)settings.tracking.velocity_wave-track_vel_wave)/(float)n:0.0f;
 const float tpw_step=n?((float)settings.tracking.pitch_wave-track_pitch_wave)/(float)n:0.0f;
 const float tvc_step=n?((float)settings.tracking.velocity_cutoff-track_vel_cutoff)/(float)n:0.0f;
 const float tpc_step=n?((float)settings.tracking.pitch_cutoff-track_pitch_cutoff)/(float)n:0.0f;
 const float velocity_target=settings.velocity_mode?1.0f:0.0f;
 const float velocity_step=n?(velocity_target-velocity_blend)/(float)n:0.0f;
 const float velocity_depth_step=n?((float)settings.velocity_amount-velocity_depth)/(float)n:0.0f;
 const float unison_wave_step=n?((float)settings.unison_wave_spread-unison_wave_depth)/(float)n:0.0f;
 const float tuning_target=(float)settings.coarse_tune*100.0f+(float)settings.fine_tune;
 const float tuning_step=n?(tuning_target-tuning_cents)/(float)n:0.0f;
 const float env2_pitch_step=n?((float)settings.env2_pitch-env2_pitch_depth)/(float)n:0.0f;
 const float g_step=n?(target_g-filter_g)/(float)n:0.0f,q_step=n?(target_q-filter_q)/(float)n:0.0f;
 for(uint32_t j=0;j<n;j++){
  if(arp_mode)arp_tick();
  velocity_blend+=velocity_step;velocity_depth+=velocity_depth_step;
  unison_wave_depth+=unison_wave_step;tuning_cents+=tuning_step;env2_pitch_depth+=env2_pitch_step;track_vel_wave+=tvw_step;track_pitch_wave+=tpw_step;track_vel_cutoff+=tvc_step;track_pitch_cutoff+=tpc_step;
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
  float mix_left=0.0f,mix_right=0.0f;
  for(unsigned i=0;i<VOICES;i++){
   voice_slot *s=&slots[i];poly_voice *v=&s->main,*t=&s->tail;
   float dl=v->pan_target_left-v->pan_left,dr=v->pan_target_right-v->pan_right;
   v->pan_left+=dl*(1.0f/64.0f);v->pan_right+=dr*(1.0f/64.0f);
   if(dl<.000001f&&dl> -.000001f)v->pan_left=v->pan_target_left;if(dr<.000001f&&dr> -.000001f)v->pan_right=v->pan_target_right;
   int running=settings.lfo_phase_mode!=PW_LFO_KEEP&&!v->wait_samples;if(running)advance_lfo(v);float sample=voice_sample(v,running);
   float left=sample*v->pan_left,right=sample*v->pan_right;
   if(s->fade){
    float f=(float)s->fade/(float)STEAL_FADE;
    t->pan_left+=(t->pan_target_left-t->pan_left)*(1.0f/64.0f);t->pan_right+=(t->pan_target_right-t->pan_right)*(1.0f/64.0f);
    float tail=voice_sample(t,0);left=left*(1.0f-f)+(tail*t->pan_left)*(f*s->tail_weight);right=right*(1.0f-f)+(tail*t->pan_right)*(f*s->tail_weight);s->fade--;
   }
   mix_left+=left;mix_right+=right;
  }
  float gain_error=target_synth_gain-synth_gain,next_gain=synth_gain+gain_error*(1.0f/256.0f);if(next_gain==synth_gain||(gain_error<.000001f&&gain_error> -.000001f))synth_gain=target_synth_gain;else synth_gain=next_gain;
  if(synth_gain!=1.0f){mix_left*=synth_gain;mix_right*=synth_gain;}
  if(chorus_active!=settings.chorus_mode&&chorus_mix==0.0f){pw_chorus_reset(&chorus);chorus_active=settings.chorus_mode;}
  float target_mix=chorus_active==settings.chorus_mode&&chorus_active?(float)settings.chorus_mix*.01f:0.0f;
  if(chorus_mix<target_mix){chorus_mix+=1.0f/256.0f;if(chorus_mix>target_mix)chorus_mix=target_mix;}
  else if(chorus_mix>target_mix){chorus_mix-=1.0f/256.0f;if(chorus_mix<target_mix)chorus_mix=target_mix;}
  if(chorus_active){float input[4]={mix_left,mix_right,0.0f,0.0f},effect[4];pw_chorus_tick_smooth(&chorus,input,effect,chorus_active);if(chorus_mix>0.0f){mix_left+=(effect[0]-mix_left)*chorus_mix;mix_right+=(effect[1]-mix_right)*chorus_mix;if(chorus_active==2){float compensation=pw_chorus_mix_headroom(chorus_mix);mix_left*=compensation;mix_right*=compensation;}}}
  if((delay_active!=settings.delay_mode||delay_style_active!=settings.delay_style)&&delay_mix==0.0f){delay_active=settings.delay_mode;delay_style_active=settings.delay_style;pw_delay_reset(&delay,(float)settings.delay_time_ms*44.1f+1.0f);delay_feedback=(float)settings.delay_feedback*.01f;}
  float target_delay_mix=delay_active==settings.delay_mode&&delay_style_active==settings.delay_style&&delay_active?(delay_style_active?target_return:(float)settings.delay_mix*.01f):0.0f;
  if(delay_mix<target_delay_mix){delay_mix+=1.0f/256.0f;if(delay_mix>target_delay_mix)delay_mix=target_delay_mix;}else if(delay_mix>target_delay_mix){delay_mix-=1.0f/256.0f;if(delay_mix<target_delay_mix)delay_mix=target_delay_mix;}
  if(delay_active){float target=(float)settings.delay_feedback*.01f,error=target-delay_feedback;delay_feedback+=error*(1.0f/64.0f);if(error<.000001f&&error> -.000001f)delay_feedback=target;float input[2]={mix_left,mix_right},out[2];(delay_style_active?pw_delay_tick_source:pw_delay_tick)(&delay,input,out,delay_active,(float)settings.delay_time_ms*44.1f+1.0f,delay_feedback,delay_mix);mix_left=out[0];mix_right=out[1];}
  float scale=(float)gain*(256.0f/(4096.0f*4.0f));
  float output[2]={mix_left*scale,mix_right*scale};
  for(unsigned k=0;k<2;k++){
   if(output[k]>8388607.0f)output[k]=8388607.0f;else if(output[k]< -8388608.0f)output[k]= -8388608.0f;
   float magnitude=output[k]<0.0f?-output[k]:output[k];if(magnitude>output_peaks[k])output_peaks[k]=magnitude;
   lr[2*j+k]=(int32_t)output[k];
  }
 }
 if(n){velocity_blend=velocity_target;velocity_depth=(float)settings.velocity_amount;unison_wave_depth=(float)settings.unison_wave_spread;tuning_cents=tuning_target;filter_base=(float)settings.filter.cutoff;filter_depth=(float)settings.filter_env_depth;filter_depth2=(float)settings.filter_env2_depth;lfo_increment=increment_target;lfo_wave_depth=(float)settings.lfo.wave;lfo_filter_depth=(float)settings.lfo.cutoff;lfo_pitch_depth=(float)settings.lfo.pitch+(float)settings.lfo_pitch_cents;wave_env_depth=(float)settings.wave_env_depth;wave_env2_depth=(float)settings.wave_env2_depth;track_vel_wave=(float)settings.tracking.velocity_wave;track_pitch_wave=(float)settings.tracking.pitch_wave;track_vel_cutoff=(float)settings.tracking.velocity_cutoff;track_pitch_cutoff=(float)settings.tracking.pitch_cutoff;env2_pitch_depth=(float)settings.env2_pitch;}
}

int32_t core_wave_sample(uint32_t index){
    if(index>255)index=255;
    int32_t pos=(int32_t)st.position*1791/100;
    uint32_t frame=(uint32_t)pos>>8;int32_t mix=pos&255;
    int32_t a,b;if(st.wave_quality){a=pw_wt_mip1[st.bank][frame][index*4];b=pw_wt_mip1[st.bank][frame+1][index*4];}else{a=drone_wt[st.bank][frame][index];b=drone_wt[st.bank][frame+1][index];}
    return a+(((b-a)*mix)>>8);
}

uint32_t core_note_osc_phase(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return slots[i].main.osc.phase;return 0;}

float core_output_level(unsigned channel){return channel<2?output_peaks[channel]*(1.0f/8388608.0f):0.0f;}

unsigned core_note_layers(unsigned port,unsigned channel,unsigned note){unsigned n=0;for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))n++;return n;}
unsigned core_voice_note(unsigned slot){return slot<VOICES&&live(&slots[slot].main)?slots[slot].main.note:0;}
float core_voice_detune(unsigned slot){return slot<VOICES&&live(&slots[slot].main)?slots[slot].main.detune_units*(1200.0f/524288.0f):0.0f;}
uint32_t core_voice_increment(unsigned slot){if(slot>=VOICES)return 0;poly_voice *v=&slots[slot].main;return (uint32_t)(((uint64_t)(uint32_t)v->osc.frequency_increment*(uint32_t)v->osc.pitch_scale_q12)>>12);}

unsigned core_pending_count(void){unsigned n=0;for(unsigned i=0;i<VOICES;i++)if(slots[i].main.wait_samples)n++;return n;}
uint32_t core_voice_delay(unsigned slot){return slot<VOICES?slots[slot].main.wait_samples:0;}
