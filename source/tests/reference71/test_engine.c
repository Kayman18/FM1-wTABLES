#include "dual_core.h"
#include "pw_bloom.h"
#define BLOOM_PORT 253u
static pw_bloom bloom;
static uint32_t bloom_left,bloom_gate_left,bloom_remainder,bloom_reseed_seen,bloom_restore_seen;
static uint8_t bloom_running,bloom_index,bloom_note,bloom_gate;
static uint16_t bloom_tempo;
static uint32_t ratchet_left,ratchet_interval,ratchet_rng,bloom_disabled,density_rng;
static uint8_t ratchet_count,ratchet_index;
static const uint8_t ratchet_counts[6]={1,2,3,4,6,8};
#include "test_engine.h"
#include "pw_matrix.h"
static pw_matrix matrix;
static float matrix_res_base;
#include "pw_core.h"
/* All AKWF frames, including low-resolution mode, load from flash. */
#include "pw_filter_mod.h"
#include "pw_wave_mod.h"
#include "pw_env_pitch.h"
#include "pw_performance.h"
#include "pw_mip.h"
#include "pw_tables.h"
#include "pw_chorus.h"
#include "pw_stereo.h"
#include "pw_unison.h"
#include "pw_delay.h"
#include "pw_reverb.h"
#include "pw_gain.h"
#include "pw_velocity.h"
#include "plat.h"
#define VOICES CORE_MAX_VOICES
static uint32_t ratchet_age[VOICES];
/* TEST71: 512 samples (11.61 ms) soften audible voice truncation.
   The tail remains bounded to one per slot; the voice ceiling is unchanged. */
#define STEAL_FADE 512u
#define PANEL_PORT 254u
#define AUDITION_PORT 255u
static volatile core_settings st;
/* Preserve the hardware-tested 4-slot renderer by default; use 6 only
   when the user selects 5 or 6 voice slots. */
static unsigned voice_span(void){return st.voice_limit>CORE_DEFAULT_VOICES?VOICES:CORE_DEFAULT_VOICES;}
static volatile uint32_t panel_keys;
static volatile uint8_t panic_requested;
static uint32_t processed_keys,serial;
static int32_t gain,position;
static float voice_headroom;
static unsigned voice_span(void);
static uint8_t active_bank,audition_on,audition_note,filter_mode;
static float env2_pitch_depth,tuning_cents,velocity_depth,velocity_blend;
static pw_chorus chorus;
static pw_reverb reverb;
static unsigned reverb_active,reverb_ready;
static float reverb_mix;
/* TEST67: zero-mix effects sleep only once their audible fade reaches zero.
   Reactivation resets histories before the fade-in, preventing stale echoes. */
static unsigned reverb_sleeping, reverb_zero_samples;
static unsigned delay_sleeping, chorus_sleeping;
static float delay_memory[PW_DELAY_FRAMES][2] PLAT_POOL;
static pw_delay delay;
static unsigned delay_active,delay_style_active;static float delay_mix,delay_feedback,synth_gain;
static uint8_t chorus_active;
static float chorus_mix,output_peaks[2];
static float chorus_compensation_mix=-1.0f,chorus_compensation;
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
 /* TEST70: smooth only matrix base-position edits; never low-pass audio-rate LFO movement. */
 float matrix_position_smoothed;
 uint8_t matrix_position_valid;
 float cutoff_index,pitch_cents,bend_cents,performance_cents;
 float pan_left,pan_right,pan_target_left,pan_target_right;
 float detune_units,detune_target;
 uint8_t layers,member,slot,geometry;
 int8_t key_octave;
 uint32_t age,wait_samples;
 uint8_t port,channel,note,velocity,held,sustained,root;
 uint8_t mip_from,mip_to,mip_fade,mip_ready;
 uint32_t cached_pitch_base,cached_mip_increment;
 int32_t cached_pitch_units,cached_pitch_increment,cached_mip_scale;
 float cached_wave_v,cached_wave_p,cached_wave_u,cached_wave;
 float cached_wave_static;
 float cached_cut_v,cached_cut_p,cached_cut,cached_velocity_depth,cached_velocity_gain;
 uint8_t cache_pitch,cache_mip,cached_mip,cache_wave,cache_wave_static,cache_cut,cache_velocity;

} poly_voice;
typedef struct {poly_voice main,tail;uint32_t fade;float tail_weight;} __attribute__((aligned(64))) voice_slot;
static voice_slot slots[VOICES] __attribute__((aligned(64)));
typedef struct {
 float lfo_increment,filter_wet,track_vel_wave,track_pitch_wave,unison_wave_depth,wave_env_depth,wave_env2_depth,lfo_wave_depth,env2_pitch_depth,lfo_pitch_depth,tuning_cents,velocity_blend,velocity_depth,filter_depth,filter_depth2,lfo_filter_depth,track_vel_cutoff,track_pitch_cutoff,filter_base,filter_g,filter_q,voice_headroom;
 int32_t position,gain,active_bank,filter_mode;
} voice_frame;
/* One small route descriptor shared by every voice in the current sample. */
typedef struct {float position,pitch,depth;unsigned position_on,depth_override;} wave_matrix_frame;
/* TEST68: retain hardware-confirmed 64-frame CPU1 chunks. */
#ifndef PW_VOICE_CHUNK
#define PW_VOICE_CHUNK 64u
#endif
#define VOICE_CHUNK PW_VOICE_CHUNK
static struct {
 voice_frame frame[VOICE_CHUNK];
 float output[VOICES][VOICE_CHUNK][2];
 float matrix_cutoff[VOICE_CHUNK],matrix_q[VOICE_CHUNK],matrix_position[VOICE_CHUNK],matrix_pitch_wave[VOICE_CHUNK],matrix_wave_depth[VOICE_CHUNK];
 unsigned matrix_active,matrix_wave_override,matrix_position_route;
 core_settings settings;
 midi_control controls[3][16];
 unsigned count,render_slots,uniform;
} voice_job __attribute__((aligned(64)));
static unsigned voice_worker_fault;
static volatile unsigned voice_parallel=1;
static uint32_t voice_worker_jobs;

/* 44.1 kHz equal temperament, nearest phase increment; old notes unchanged. */
static const uint32_t INC[128]={796254,843601,893765,946911,1003217,1062871,1126073,1193033,1263974,1339134,1418763,1503127,1592507,1687203,1787529,1893821,2006434,2125742,2252146,2386065,2527948,2678268,2837526,3006254,3185015,3374406,3575058,3787642,4012867,4251485,4504291,4772130,5055896,5356535,5675051,6012507,6370030,6748811,7150117,7575285,8025735,8502970,9008582,9544261,10111792,10713070,11350103,12025015,12740059,13497623,14300233,15150569,16051469,17005939,18017165,19088521,20223584,21426141,22700205,24050030,25480119,26995246,28600467,30301139,32102938,34011878,36034330,38177043,40447168,42852281,45400411,48100060,50960238,53990491,57200933,60602278,64205876,68023757,72068660,76354085,80894335,85704563,90800821,96200119,101920476,107980983,114401866,121204555,128411753,136047513,144137319,152708170,161788671,171409126,181601643,192400238,203840952,215961966,228803732,242409110,256823506,272095026,288274639,305416341,323577341,342818251,363203285,384800477,407681904,431923931,457607465,484818220,513647012,544190053,576549277,610832681,647154683,685636503,726406571,769600953,815363807,863847862,915214929,969636441,1027294024,1088380105,1153098554,1221665363};

static int live(const poly_voice *v){return v->env.stage!=PW_ENV_IDLE||pw_filter_live(&v->filter);}
static void defaults(poly_voice *v){*v=(poly_voice){0};pw_env_init(&v->env);pw_env_init(&v->env2);pw_env_set(&v->env2,(pw_env_params){60,70,64,50});v->geometry=4;v->osc.pitch_scale_q12=4096;v->pan_left=v->pan_right=v->pan_target_left=v->pan_target_right=1.0f;}
void core_init(void){
 pw_tables_init();
 reverb=(pw_reverb){0};reverb_active=reverb_ready=0;reverb_mix=0;reverb_sleeping=reverb_zero_samples=0;delay_sleeping=chorus_sleeping=0;
 voice_worker_fault=0;voice_worker_jobs=0;voice_parallel=1;pw_matrix_init(&matrix);matrix_res_base=0;
 ratchet_left=ratchet_interval=ratchet_index=0;ratchet_count=1;ratchet_rng=0x54b10001u;bloom_disabled=0;density_rng=0x57de1234u;for(unsigned i=0;i<VOICES;i++)ratchet_age[i]=0;pw_bloom_init(&bloom);bloom_left=bloom_gate_left=bloom_remainder=bloom_reseed_seen=bloom_restore_seen=0;bloom_running=bloom_gate=0;bloom_index=255;bloom_note=0;bloom_tempo=0;
 st=(core_settings){0,48,0,0,75,{50,60,80,60},{PW_FILTER_OFF,96,0,100},0,{60,70,64,50},0,{40,0,0,0},0,0,{0,0,0,0},PW_LFO_KEEP,0,0};
 voice_headroom=1.0f;st.voice_limit=CORE_DEFAULT_VOICES;st.delay_sync=1;st.delay_division=2;st.sequencer_bpm=120;st.bloom_gate_percent=50;
 pw_delay_init(&delay,delay_memory,PW_DELAY_FRAMES);pw_delay_reset(&delay,11026.0f);delay_active=delay_style_active=0;synth_gain=1.0f;st.gain_db=0;st.delay_return_db= -3;st.delay_style=0;delay_mix=delay_feedback=0.0f;st.delay_time_ms=500;st.delay_feedback=50;st.delay_mix=35;st.delay_spread=0;st.delay_cross=100;st.delay_lowcut_hz=80;st.delay_highcut_hz=8000;
 st.fx_reverb=0;st.reverb_model=3;st.reverb_size=70;st.reverb_damping=50;st.reverb_depth=20;st.reverb_mix=25;st.reverb_time_ms=2000;st.reverb_speed=25;
 st.velocity_amount=100;velocity_depth=100.0f;velocity_blend=0.0f;
 st.unison_detune=10;unison_wave_depth=0.0f;
 st.wave_quality=1;st.chorus_mix=50;pw_chorus_reset(&chorus);chorus_active=0;chorus_mix=0.0f;output_peaks[0]=output_peaks[1]=0.0f;
 panel_keys=processed_keys=serial=panic_requested=0;gain=position=active_bank=audition_on=0;audition_note=48;filter_mode=PW_FILTER_OFF;filter_wet=0.0f;filter_g=pw_filter_g(96);filter_q=pw_filter_q(0);filter_base=96.0f;filter_depth=filter_depth2=0.0f;pw_filter_mod_init();env2_pitch_depth=0.0f;track_vel_wave=track_pitch_wave=track_vel_cutoff=track_pitch_cutoff=0.0f;wave_env_depth=wave_env2_depth=0.0f;lfo_wave_depth=lfo_filter_depth=lfo_pitch_depth=0.0f;lfo_increment=(float)pw_lfo_increment(40);
 tuning_cents=0.0f;for(unsigned p=0;p<3;p++)for(unsigned c=0;c<16;c++)controls[p][c]=(midi_control){0};
 for(unsigned i=0;i<VOICES;i++){defaults(&slots[i].main);defaults(&slots[i].tail);slots[i].fade=0;}
}
void core_set(const core_settings *s){core_settings c=*s;if(c.env.sustain<0)c.env.sustain=0;if(c.lfo_pitch_cents< -1200)c.lfo_pitch_cents= -1200;if(c.lfo_pitch_cents>1200)c.lfo_pitch_cents=1200;if(c.note>127)c.note=127;if(c.position>100)c.position=100;if(c.bank>=PW_TABLE_BANKS)c.bank=PW_TABLE_BANKS-1;if(c.level>100)c.level=100;if(c.filter.mode>3)c.filter.mode=3;if(c.filter.cutoff>127)c.filter.cutoff=127;if(c.filter.resonance>100)c.filter.resonance=100;if(c.filter.mix>100)c.filter.mix=100;if(c.filter_env_depth< -127)c.filter_env_depth= -127;if(c.filter_env2_depth< -127)c.filter_env2_depth= -127;if(c.lfo.rate>100)c.lfo.rate=100;if(c.lfo.rate< -100)c.lfo.rate= -100;if(c.lfo.wave>100)c.lfo.wave=100;if(c.lfo.wave< -100)c.lfo.wave= -100;if(c.lfo.cutoff< -127)c.lfo.cutoff= -127;if(c.lfo.pitch>100)c.lfo.pitch=100;if(c.lfo.pitch+c.lfo_pitch_cents>1200)c.lfo_pitch_cents=(int16_t)(1200-c.lfo.pitch);if(c.wave_env_depth>100)c.wave_env_depth=100;if(c.wave_env_depth< -100)c.wave_env_depth= -100;if(c.wave_env2_depth>100)c.wave_env2_depth=100;if(c.wave_env2_depth< -100)c.wave_env2_depth= -100;
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
 if(c.bend_range_offset< -2)c.bend_range_offset= -2;if(c.bend_range_offset>46)c.bend_range_offset=46;if(c.wave_quality>1)c.wave_quality=1;if(c.chorus_mode>2)c.chorus_mode=2;if(c.chorus_mix>100)c.chorus_mix=100;if(c.stereo_mode>3)c.stereo_mode=3;if(c.stereo_spread< -100)c.stereo_spread= -100;if(c.stereo_spread>100)c.stereo_spread=100;if(c.unison_mode>2)c.unison_mode=2;if(c.unison_detune>100)c.unison_detune=100;if(c.unison_wave_spread< -100)c.unison_wave_spread= -100;if(c.unison_wave_spread>100)c.unison_wave_spread=100;if(c.gain_db< -40)c.gain_db= -40;if(c.gain_db>24)c.gain_db=24;if(c.delay_return_db< -40)c.delay_return_db= -40;if(c.delay_return_db>0)c.delay_return_db=0;if(c.delay_sync>1)c.delay_sync=1;if(c.delay_division>3)c.delay_division=3;if(c.delay_style>1)c.delay_style=1;if(c.delay_mode>2)c.delay_mode=2;if(c.delay_feedback>90)c.delay_feedback=90;if(c.delay_mix>100)c.delay_mix=100;if(c.delay_time_ms<1)c.delay_time_ms=1;if(c.delay_time_ms>PW_DELAY_MAX_MS)c.delay_time_ms=PW_DELAY_MAX_MS;if(c.delay_spread>100)c.delay_spread=100;if(c.delay_cross>100)c.delay_cross=100;if(c.delay_lowcut_hz<20)c.delay_lowcut_hz=20;if(c.delay_lowcut_hz>1000)c.delay_lowcut_hz=1000;if(c.delay_highcut_hz<500)c.delay_highcut_hz=500;if(c.delay_highcut_hz>9500)c.delay_highcut_hz=9500;if(c.velocity_mode>1)c.velocity_mode=1;if(c.velocity_amount>100)c.velocity_amount=100;if(c.velocity_curve>3)c.velocity_curve=3;if(c.trigger_mode>2)c.trigger_mode=2;if(c.sustain_mode>2)c.sustain_mode=2;if(c.voice_limit>CORE_MAX_VOICES)c.voice_limit=CORE_MAX_VOICES;if(c.unison_count>CORE_MAX_UNISON)c.unison_count=CORE_MAX_UNISON;if(c.sustain_hold>1)c.sustain_hold=1;if(c.sequencer_bpm<40)c.sequencer_bpm=120;if(c.sequencer_bpm>240)c.sequencer_bpm=240;if(c.bloom_gate_percent<10)c.bloom_gate_percent=50;if(c.bloom_gate_percent>90)c.bloom_gate_percent=90;if(c.bloom_density>100)c.bloom_density=100;if(c.bloom_mode>1)c.bloom_mode=1;if(c.bloom_length<1)c.bloom_length=16;if(c.bloom_length>PW_BLOOM_STEPS)c.bloom_length=PW_BLOOM_STEPS;if(c.bloom_scale>=PW_BLOOM_SCALES)c.bloom_scale=PW_BLOOM_SCALES-1;if(c.bloom_offset< -24)c.bloom_offset= -24;if(c.bloom_offset>24)c.bloom_offset=24;if(c.bloom_root>11)c.bloom_root=11;if(c.bloom_octave>4)c.bloom_octave=4;if(c.bloom_mutation>100)c.bloom_mutation=100;if(c.bloom_branches>7)c.bloom_branches=7;if(c.bloom_path>127)c.bloom_path=127;if(c.bloom_ratchet>5)c.bloom_ratchet=5;if(c.bloom_ratchet_chance>100)c.bloom_ratchet_chance=100;
 if(c.fx_reverb>1)c.fx_reverb=1;c.reverb_model=3;if(c.reverb_size<1)c.reverb_size=1;if(c.reverb_size>100)c.reverb_size=100;if(c.reverb_damping>100)c.reverb_damping=100;if(c.reverb_depth>100)c.reverb_depth=100;if(c.reverb_mix>100)c.reverb_mix=100;if(c.reverb_time_ms<400)c.reverb_time_ms=400;if(c.reverb_time_ms>30000)c.reverb_time_ms=30000;if(c.reverb_speed<1)c.reverb_speed=1;if(c.reverb_speed>500)c.reverb_speed=500;
 for(unsigned r=0;r<4;r++){if(c.matrix_source[r]>11)c.matrix_source[r]=0;if(c.matrix_dest[r]>6)c.matrix_dest[r]=0;if(c.matrix_amount[r]>100)c.matrix_amount[r]=100;if(c.matrix_amount[r]<-100)c.matrix_amount[r]=-100;}
 if(st.bloom_mode!=c.bloom_mode)c.playing=0;st=c;}
void core_keys(uint32_t keys){panel_keys=keys&0x7ffffffu;}
static int matches(const poly_voice *v,unsigned port,unsigned channel,unsigned note){return live(v)&&v->port==port&&v->channel==channel&&v->root==note;}
/* The source combines its global sustain parameter with the pedal state.
   FM1 keeps pedals scoped per MIDI port/channel, and adds panel keys. */
static int sustain_active(unsigned port,unsigned channel){return (port!=AUDITION_PORT&&port!=BLOOM_PORT&&st.sustain_hold)||(port<3&&controls[port][channel].sustain);}
static void reconcile_sustain(void){
 for(unsigned i=0;i<VOICES;i++){
  poly_voice *v=&slots[i].main;
  if(v->sustained==2&&st.trigger_mode==2)continue;
  if(v->sustained&&!sustain_active(v->port,v->channel)){
   v->sustained=0;if(!v->held){pw_env_off(&v->env);pw_env_off(&v->env2);}
  }
 }
}
static void voice_note_off(unsigned port,unsigned channel,unsigned note){
 for(unsigned i=0;i<VOICES;i++){
  poly_voice *v=&slots[i].main;
  if(matches(v,port,channel,note)&&v->held){if(v->wait_samples){defaults(v);continue;}v->held=0;if(v->env.triggered){v->sustained=0;continue;}if(st.trigger_mode==2&&port!=AUDITION_PORT&&port!=BLOOM_PORT)v->sustained=2;else if(sustain_active(port,channel))v->sustained=1;else{v->sustained=0;pw_env_off(&v->env);pw_env_off(&v->env2);}}
 }
}
static void note_on_at(unsigned port,unsigned channel,unsigned note,unsigned velocity,unsigned forced,unsigned root){
 if(note>127||!velocity)return;
 unsigned span=voice_span();uint32_t phases[VOICES];int any_held=0;
 for(unsigned i=0;i<VOICES;i++){phases[i]=slots[i].main.lfo.phase;unsigned stage=slots[i].main.env.stage;if(stage>=PW_ENV_SUSTAIN&&stage<=PW_ENV_DECAY)any_held=1;}
 unsigned chosen=forced<VOICES?forced:VOICES;
 /* Repeated note-on retriggers its own slot, preserving phase and level. */
 if(chosen==VOICES)for(unsigned i=0;i<span;i++)if(matches(&slots[i].main,port,channel,root)){chosen=i;break;}
 int fresh=forced<VOICES?!(matches(&slots[chosen].main,port,channel,root)&&slots[chosen].main.note==note):chosen==VOICES;
 if(chosen==VOICES){
  for(unsigned i=0;i<span;i++)if(!live(&slots[i].main)&&!slots[i].fade){chosen=i;break;}
  if(chosen==VOICES){
   /* Prefer the quietest released voice, then the oldest held voice. */
   float quiet=2.0f;
   for(unsigned i=0;i<span;i++)if(!slots[i].main.held&&!slots[i].main.sustained&&slots[i].main.env.level<quiet){chosen=i;quiet=slots[i].main.env.level;}
   if(chosen==VOICES){for(unsigned i=0;i<span;i++)if(slots[i].main.sustained&&(chosen==VOICES||(uint32_t)(serial-slots[i].main.age)>(uint32_t)(serial-slots[chosen].main.age)))chosen=i;}
   if(chosen==VOICES){chosen=0;for(unsigned i=1;i<span;i++)if((uint32_t)(serial-slots[i].main.age)>(uint32_t)(serial-slots[chosen].main.age))chosen=i;}
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
 pw_lfo_assign_phase(phases,span,chosen,st.lfo_phase_mode,st.lfo_spread,any_held,fresh);
 for(unsigned i=0;i<VOICES;i++)slots[i].main.lfo.phase=phases[i];
 poly_voice *v=&slots[chosen].main;
 v->cache_pitch=v->cache_mip=v->cache_wave=v->cache_wave_static=v->cache_cut=v->cache_velocity=0;
 v->wait_samples=0;v->layers=1;v->member=0;v->slot=(uint8_t)chosen;if(fresh)v->geometry=(uint8_t)span;
 v->port=(uint8_t)port;v->channel=(uint8_t)channel;v->note=(uint8_t)note;v->root=(uint8_t)root;v->velocity=(uint8_t)velocity;v->held=1;v->sustained=0;v->age=++serial;v->key_octave=port==PANEL_PORT?st.panel_octave:0;
 v->bend_cents=port<3?pw_performance_bend(controls[port][channel].bend,(unsigned)(2+st.bend_range_offset)):0.0f;
 pw_stereo_gains(st.stereo_mode,st.stereo_spread,chosen,v->geometry,(int)note,&v->pan_target_left,&v->pan_target_right);if(fresh){v->pan_left=v->pan_target_left;v->pan_right=v->pan_target_right;}
 v->osc.frequency_increment=(int32_t)INC[note];v->osc.position_q8=position;
 pw_env_shapes(&v->env,64+st.amp_attack_shape,64+st.amp_decay_shape);pw_env_shapes(&v->env2,64+st.env2_attack_shape,64+st.env2_decay_shape);
 pw_env_trigger_mode(&v->env,st.trigger_mode==1);pw_env_trigger_mode(&v->env2,st.trigger_mode==1);
 pw_env_set(&v->env,st.env);pw_env_on(&v->env);pw_env_set(&v->env2,st.env2);pw_env_on(&v->env2);
}
static void retire_group(unsigned port,unsigned channel,unsigned note){
 for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note)){
  voice_slot *s=&slots[i];
  if(!s->main.wait_samples&&(!s->fade||s->main.env.level>0.0f)){s->tail_weight=1.0f-(float)s->fade/(float)STEAL_FADE;s->tail=s->main;s->fade=STEAL_FADE;}
  defaults(&s->main);
 }
}
static unsigned voice_budget(void){return st.voice_limit?st.voice_limit:CORE_DEFAULT_VOICES;}
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
static void enforce_budget(void){
 /* Retire groups above slot 3 when returning to the four-slot layout. */
 if(voice_span()==4)for(unsigned i=4;i<VOICES;i++)if(live(&slots[i].main))retire_group(slots[i].main.port,slots[i].main.channel,slots[i].main.root);
 while(core_primary_count()>voice_budget())if(!retire_one_group())break;}
static void voice_note_on(unsigned port,unsigned channel,unsigned note,unsigned velocity){
 if(note>127||!velocity)return;
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
 unsigned count=core_unison_effective(&st),existing[VOICES],found=0,stacked=0;
 unsigned budget=voice_budget();while(count>budget)count>>=1;
 if(!count){retire_group(port,channel,note);return;}
 for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main;if(matches(v,port,channel,note))existing[found++]=i;if(live(v)&&(v->layers>1||v->note!=v->root))stacked=1;}
 if(count==1&&!stacked&&budget==voice_span()){note_on_at(port,channel,note,velocity,VOICES,note);return;}
 if(found&&found!=count){retire_group(port,channel,note);found=0;}
 if(!found){
  for(;;){
   unsigned used=core_primary_count(),free=used<budget?budget-used:0;
   if(free>=count)break;
   if(!retire_one_group())return;
  }
  if(count==2){unsigned span=voice_span();for(unsigned i=0;i<span/2;i++)if(!live(&slots[i].main)&&!live(&slots[span-1-i].main)){existing[0]=i;existing[1]=span-1-i;found=2;break;}}
  if(!found)for(unsigned i=0;i<voice_span()&&found<count;i++)if(!live(&slots[i].main))existing[found++]=i;
 }
 for(unsigned k=0;k<count;k++){
  unsigned slot=existing[k];int retrigger=matches(&slots[slot].main,port,channel,note);
  unsigned pitch=note;
  note_on_at(port,channel,pitch,velocity,slot,note);poly_voice *v=&slots[slot].main;v->layers=(uint8_t)count;v->member=(uint8_t)k;
  v->detune_target=(float)pw_unison_detune(v->layers,v->member,st.unison_detune);if(!retrigger)v->detune_units=v->detune_target;
 }
}
/* MIDI, panel and sequencer events always address the normal polyphonic engine. */
static void note_on(unsigned port,unsigned channel,unsigned note,unsigned velocity){voice_note_on(port,channel,note,velocity);}
static void note_off(unsigned port,unsigned channel,unsigned note){voice_note_off(port,channel,note);}

/* Dedicated ownership: generated gates cannot latch on global sustain. */
static void bloom_off(void){
 for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main;if(v->port==BLOOM_PORT&&(v->held||v->sustained)){if(v->wait_samples){defaults(v);continue;}v->held=v->sustained=0;pw_env_off(&v->env);pw_env_off(&v->env2);}}
 bloom_gate_left=0;bloom_gate=0;
}
static void ratchet_reset(void){ratchet_count=1;ratchet_index=0;ratchet_left=0;}
/* Retrigger only this step's still-owned voice group: no extra voices/tails. */
static void ratchet_retrigger(void){
 for(unsigned i=0;i<VOICES;i++){
  poly_voice *v=&slots[i].main;
  if(!ratchet_age[i]||v->age!=ratchet_age[i]||!matches(v,BLOOM_PORT,0,bloom_note))continue;
  v->held=1;v->sustained=0;
  pw_env_shapes(&v->env,64+st.amp_attack_shape,64+st.amp_decay_shape);pw_env_shapes(&v->env2,64+st.env2_attack_shape,64+st.env2_decay_shape);
  pw_env_trigger_mode(&v->env,st.trigger_mode==1);pw_env_trigger_mode(&v->env2,st.trigger_mode==1);
  pw_env_set(&v->env,st.env);pw_env_set(&v->env2,st.env2);pw_env_on(&v->env);pw_env_on(&v->env2);
 }
 bloom_gate=1;
}
static void ratchet_schedule(void){
 unsigned start=(uint64_t)ratchet_index*ratchet_interval/ratchet_count;
 unsigned end=(uint64_t)(ratchet_index+1)*ratchet_interval/ratchet_count;
 ratchet_left=end-start;bloom_gate_left=ratchet_left*st.bloom_gate_percent/100u;
}
static void bloom_sync(void){
 if(bloom_reseed_seen!=st.bloom_reseed){bloom_reseed_seen=st.bloom_reseed;pw_bloom_reseed(&bloom);bloom_left=bloom_remainder=0;bloom_index=255;ratchet_reset();bloom_off();}
 if(bloom_restore_seen!=st.bloom_restore){bloom_restore_seen=st.bloom_restore;pw_bloom_restore(&bloom);bloom_left=bloom_remainder=0;bloom_index=255;ratchet_reset();bloom_off();}
 unsigned run=st.bloom_mode&&st.playing;
 if(run!=bloom_running){bloom_off();bloom_running=run;bloom_left=bloom_remainder=0;bloom_index=255;bloom.depth=7;ratchet_reset();}
}
static void bloom_tick(void){
 if(!bloom_running)return;
 if(!bloom_left){
  bloom_off();bloom_index=bloom_index==255?0:(uint8_t)((bloom_index+1)%st.bloom_length);
  if(bloom_index>=st.bloom_length)bloom_index=0;
  if(!bloom_index)pw_bloom_boundary(&bloom,st.bloom_branches,st.bloom_path);
  int pitch=pw_bloom_offset(pw_bloom_tree_step(&bloom,bloom_index,st.bloom_scale,st.bloom_mutation),st.bloom_scale,st.bloom_offset)+st.bloom_root+12*((int)st.bloom_octave+1+(int)st.panel_octave);
  while(pitch<0)pitch+=12;while(pitch>127)pitch-=12;
  bloom_note=(uint8_t)pitch;
  if(bloom_tempo!=st.sequencer_bpm){bloom_tempo=st.sequencer_bpm;bloom_remainder=0;}
  bloom_left=pw_bloom_eighth_interval(st.sequencer_bpm,&bloom_remainder);bloom_gate_left=bloom_left*st.bloom_gate_percent/100u;
  unsigned audible=!(bloom_disabled&(1u<<bloom_index));
  if(st.bloom_density<100){unsigned roll=(uint64_t)pw_bloom_random(&density_rng)*100u>>32;audible=audible&&roll<st.bloom_density;}
  if(audible){voice_note_on(BLOOM_PORT,0,bloom_note,100);bloom_gate=1;}
  ratchet_count=audible?ratchet_counts[st.bloom_ratchet]:1;
  if(ratchet_count>1&&((uint64_t)pw_bloom_random(&ratchet_rng)*100u>>32)>=st.bloom_ratchet_chance)ratchet_count=1;
  ratchet_index=0;ratchet_interval=bloom_left;ratchet_schedule();
  for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main;ratchet_age[i]=matches(v,BLOOM_PORT,0,bloom_note)&&v->held?v->age:0;}
 }else if(ratchet_index+1<ratchet_count&&!ratchet_left){
  bloom_off();++ratchet_index;ratchet_retrigger();ratchet_schedule();
 }else if(bloom_gate&&!bloom_gate_left)bloom_off();
 if(ratchet_left)--ratchet_left;
 --bloom_left;if(bloom_gate_left)--bloom_gate_left;
}
unsigned core_bloom_seed(unsigned index){return index<PW_BLOOM_STEPS?bloom.seed[index]:0;}
unsigned core_bloom_enabled(unsigned index){return index<PW_BLOOM_STEPS&&!(bloom_disabled&(1u<<index));}
unsigned core_bloom_edit(unsigned index,int delta,unsigned scale){
 if(index>=PW_BLOOM_STEPS)return 0;
 uint32_t bit=1u<<index;
 if(bloom_disabled&bit){if(delta<=0)return bloom.seed[index];bloom_disabled&=~bit;--delta;}
 else if(delta<0&&!bloom.seed[index]){bloom_disabled|=bit;return 0;}
 return pw_bloom_edit(&bloom,index,delta,scale);
}
unsigned core_bloom_branch(void){return bloom.depth;}
unsigned core_bloom_step(void){return bloom_index;}
unsigned core_bloom_note(void){return bloom_note;}
unsigned core_bloom_pattern(unsigned index){return index<PW_BLOOM_STEPS?pw_bloom_quantize(bloom.valid&&bloom.node?bloom.tree[bloom.active][bloom.node][index]:bloom.notes[index],st.bloom_scale):0;}

static void pedal_up(unsigned port,unsigned channel){
 controls[port][channel].sustain=0;
 if(sustain_active(port,channel))return;
 for(unsigned i=0;i<VOICES;i++){
  poly_voice *v=&slots[i].main;
  if(v->sustained==2&&st.trigger_mode==2)continue;
  if(v->port==port&&v->channel==channel&&v->sustained){v->sustained=0;if(!v->held){pw_env_off(&v->env);pw_env_off(&v->env2);}}
 }
}
void core_midi(unsigned port,uint8_t status,uint8_t note,uint8_t velocity){
 if(port>2)return;if(status>=0xf8)return;unsigned channel=status&15,command=status&240;note&=127;velocity&=127;
 if(command==0x90&&velocity)note_on(port,channel,note,pw_velocity_map(velocity,1,st.velocity_curve));
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
     else if(s->main.wait_samples){defaults(&s->main);}
     else if(s->main.env.triggered||st.trigger_mode==2){s->main.held=s->main.sustained=0;pw_env_off(&s->main.env);pw_env_off(&s->main.env2);}
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
float core_voice_wave(unsigned slot){return slot<VOICES&&live(&slots[slot].main)?(float)slots[slot].main.osc.position_q8*(100.0f/(float)PW_FRAME_POS_MAX):-1.0f;}
float core_note_wave(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return (float)slots[i].main.osc.position_q8*(100.0f/(float)PW_FRAME_POS_MAX);return -1.0f;}
float core_voice_cutoff(unsigned slot){return slot<VOICES&&live(&slots[slot].main)?slots[slot].main.cutoff_index:-1.0f;}
float core_note_cutoff(unsigned port,unsigned channel,unsigned note){
 for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return slots[i].main.cutoff_index;
 return -1.0f;
}
unsigned core_voice_mip(unsigned slot){return slot<VOICES&&live(&slots[slot].main)?(slots[slot].main.mip_fade?slots[slot].main.mip_to:slots[slot].main.mip_from):0;}
unsigned core_note_mip(unsigned port,unsigned channel,unsigned note){for(unsigned i=0;i<VOICES;i++)if(matches(&slots[i].main,port,channel,note))return core_voice_mip(i);return 0;}
static const int16_t *mip_frames(unsigned mip,const voice_frame *f){return pw_tables_busy()?0:mip?pw_table_mips[mip-1][f->active_bank]:pw_table_mips[2][f->active_bank];}
/* Per-voice memoization only: identical inputs retain identical arithmetic. */
static int32_t voice_increment(poly_voice *v,uint32_t base,int32_t units){
 if(!v->cache_pitch||v->cached_pitch_base!=base||v->cached_pitch_units!=units){v->cached_pitch_increment=pw_performance_increment(base,units);v->cached_pitch_base=base;v->cached_pitch_units=units;v->cache_pitch=1;}
 return v->cached_pitch_increment;
}
static float voice_tracking(poly_voice *v,const voice_frame *f){
 if(!v->cache_cut||v->cached_cut_v!=f->track_vel_cutoff||v->cached_cut_p!=f->track_pitch_cutoff){v->cached_cut=pw_tracking_cutoff_offset(v->note,v->velocity,f->track_vel_cutoff,f->track_pitch_cutoff);v->cached_cut_v=f->track_vel_cutoff;v->cached_cut_p=f->track_pitch_cutoff;v->cache_cut=1;}return v->cached_cut;
}
static int32_t wave_sample(poly_voice *v,const voice_frame *f){
 unsigned desired=0;
 if(voice_job.settings.wave_quality){
  if(!v->cache_mip||v->cached_mip_increment!=(uint32_t)v->osc.frequency_increment||v->cached_mip_scale!=v->osc.pitch_scale_q12){v->cached_mip=(uint8_t)pw_mip_select((uint32_t)v->osc.frequency_increment,v->osc.pitch_scale_q12);v->cached_mip_increment=(uint32_t)v->osc.frequency_increment;v->cached_mip_scale=v->osc.pitch_scale_q12;v->cache_mip=1;}
  desired=v->cached_mip;
 }
 /* Auto uses its largest available aligned table below the source 2048-point band. */
 if(voice_job.settings.wave_quality&&!desired)desired=1;
 if(!v->mip_ready){v->mip_from=v->mip_to=(uint8_t)desired;v->mip_ready=1;}
 if(!v->mip_fade&&desired!=v->mip_from){v->mip_to=(uint8_t)desired;v->mip_fade=64;}
 pw_core_voice second=v->osc;
 int32_t sample=pw_core_tick(&v->osc,mip_frames(v->mip_from,f),PW_FRAME_COUNT,v->mip_from?v->mip_from:3)/256;
 if(v->mip_fade){
  int32_t next=pw_core_tick(&second,mip_frames(v->mip_to,f),PW_FRAME_COUNT,v->mip_to?v->mip_to:3)/256;
  sample=(sample*(int32_t)v->mip_fade+next*(64-(int32_t)v->mip_fade))/64;
  if(!--v->mip_fade)v->mip_from=v->mip_to;
 }
 return sample;
}
static void advance_lfo(poly_voice *v,const voice_frame *f){
 float raw=pw_lfo_tick(&v->lfo,(int32_t)f->lfo_increment);v->lfo_smoothed+=(raw-v->lfo_smoothed)*(1.0f/32.0f);
}
static float voice_sample(poly_voice *v,int advanced,const voice_frame *f,float matrix_cutoff,float matrix_q,const wave_matrix_frame *wm){
 if(v->wait_samples){v->wait_samples--;return 0.0f;}
 if(f->filter_wet<=0.0f)pw_filter_reset(&v->filter);
 if(!live(v))return 0.0f;
 if(!advanced)advance_lfo(v,f);
 float movement=v->lfo_smoothed;
 float dry=0.0f,envelope=v->env.level;
 float wave_amp=v->env.level,wave_env2=v->env2.level;
 float envelope2=pw_env_tick(&v->env2);
 if(v->env.stage!=PW_ENV_IDLE){
  /* TEST70: matrix-position smoothing is ~0.36ms at 44.1kHz. It does not
     affect the normal sine LFO, envelope morphing or fractional frame reads. */
  float base=(float)f->position;
  if(wm&&wm->position_on){
   const float target=wm->position;
   if(!v->matrix_position_valid){v->matrix_position_smoothed=target;v->matrix_position_valid=1;}
   else{
    const float err=target-v->matrix_position_smoothed;
    /* Below 1/16 of a q8 step, snap to avoid denormal tails. */
    if(err>0.0625f||err< -0.0625f)v->matrix_position_smoothed+=err*(1.0f/16.0f);
    else v->matrix_position_smoothed=target;
   }
   base=v->matrix_position_smoothed;
  }else v->matrix_position_valid=0;
  if(wm&&wm->position_on){if(base<0.0f)base=0.0f;else if(base>(float)PW_FRAME_POS_MAX)base=(float)PW_FRAME_POS_MAX;}
  const int32_t base_position=(int32_t)base;
  const float lfo_wave_depth=wm&&wm->depth_override?wm->depth:f->lfo_wave_depth;
  float pitch_wave=f->track_pitch_wave;
  if(wm&&wm->pitch!=0.0f){
   pitch_wave+=wm->pitch;
   if(pitch_wave< -100.0f)pitch_wave= -100.0f;else if(pitch_wave>100.0f)pitch_wave=100.0f;
  }
  v->osc.position_q8=base_position;
  if(f->track_vel_wave!=0.0f||pitch_wave!=0.0f||f->unison_wave_depth!=0.0f){
   if(!v->cache_wave||v->cached_wave_v!=f->track_vel_wave||v->cached_wave_p!=pitch_wave||v->cached_wave_u!=f->unison_wave_depth){
    /* TEST71: pitch-wave matrix changes every modulation tick, but velocity
       tracking and unison offset usually remain fixed. Reuse that static
       component per voice; preserve audio-rate pitch-wave interpolation. */
    if(!v->cache_wave_static||v->cached_wave_v!=f->track_vel_wave||v->cached_wave_u!=f->unison_wave_depth){
     v->cached_wave_static=(float)v->velocity*f->track_vel_wave*0.00007812499825377017f;
     if(f->unison_wave_depth!=0.0f)v->cached_wave_static+=pw_unison_wave(v->slot,v->geometry,f->unison_wave_depth);
     v->cache_wave_static=1;
    }
    v->cached_wave=v->cached_wave_static+((float)((int)v->note-48)*pitch_wave)*0.00007812499825377017f;
    v->cached_wave_v=f->track_vel_wave;v->cached_wave_p=pitch_wave;v->cached_wave_u=f->unison_wave_depth;v->cache_wave=1;
   }float tracked=v->cached_wave;
   float mod=(float)base_position+tracked*(float)PW_FRAME_POS_MAX;
   mod+=(f->wave_env_depth*wave_amp+f->wave_env2_depth*wave_env2+lfo_wave_depth*movement)*((float)PW_FRAME_POS_MAX/100.0f);
   if(mod<0.0f)mod=0.0f;else if(mod>(float)PW_FRAME_POS_MAX)mod=(float)PW_FRAME_POS_MAX;v->osc.position_q8=(int32_t)mod;
  }
  else if(f->wave_env_depth!=0.0f||f->wave_env2_depth!=0.0f)v->osc.position_q8=pw_wave_mod_position(base_position,f->wave_env_depth,wave_amp,f->wave_env2_depth,wave_env2,lfo_wave_depth,movement);
  else if(lfo_wave_depth!=0.0f){float mod=(float)base_position+movement*lfo_wave_depth*((float)PW_FRAME_POS_MAX/100.0f);if(mod<0.0f)mod=0.0f;else if(mod>(float)PW_FRAME_POS_MAX)mod=(float)PW_FRAME_POS_MAX;v->osc.position_q8=(int32_t)mod;}
  v->pitch_cents=wave_env2*f->env2_pitch_depth*10.0f+movement*f->lfo_pitch_depth;
  if(f->env2_pitch_depth!=0.0f)v->osc.pitch_scale_q12=pw_env_pitch_scale(pw_env_pitch_units(wave_env2,f->env2_pitch_depth,movement*f->lfo_pitch_depth));
  else v->osc.pitch_scale_q12=f->lfo_pitch_depth!=0.0f?pw_lfo_pitch_scale(movement*f->lfo_pitch_depth):4096;
  float bend_target=v->port<3?pw_performance_bend(voice_job.controls[v->port][v->channel].bend,(unsigned)(2+voice_job.settings.bend_range_offset)):0.0f;
  v->bend_cents+=(bend_target-v->bend_cents)*(1.0f/64.0f);
  float bend_error=bend_target-v->bend_cents;if(bend_error<.001f&&bend_error> -.001f)v->bend_cents=bend_target;
  v->performance_cents=f->tuning_cents+(float)v->key_octave*1200.0f+v->bend_cents;
  v->osc.frequency_increment=(int32_t)INC[v->note];
  float delta=v->detune_target-v->detune_units;v->detune_units+=delta*(1.0f/64.0f);if(delta<.5f&&delta> -.5f)v->detune_units=v->detune_target;
  if(v->detune_units!=0.0f){int32_t units=pw_performance_units(v->performance_cents+v->pitch_cents)+(int32_t)v->detune_units;v->osc.frequency_increment=voice_increment(v,INC[v->note],units);v->osc.pitch_scale_q12=4096;}
  else if(v->note>=84||v->performance_cents!=0.0f||f->lfo_pitch_depth>100.0f||f->lfo_pitch_depth< -100.0f){v->osc.frequency_increment=voice_increment(v,INC[v->note],pw_performance_units(v->performance_cents+v->pitch_cents));v->osc.pitch_scale_q12=4096;}
  int32_t sample=wave_sample(v,f);
  envelope=pw_env_tick(&v->env);dry=(float)sample*(envelope*(float)v->velocity/127.0f);
  if(f->velocity_blend>0.0f){if(!v->cache_velocity||v->cached_velocity_depth!=f->velocity_depth){v->cached_velocity_gain=pw_velocity_gain(v->velocity,f->velocity_depth);v->cached_velocity_depth=f->velocity_depth;v->cache_velocity=1;}float legacy=(float)v->velocity/127.0f,source=v->cached_velocity_gain;dry=(float)sample*(envelope*(legacy+(source-legacy)*f->velocity_blend));}
 }
 if(f->filter_depth<0.0f||f->filter_depth2<0.0f||f->lfo_filter_depth<0.0f||envelope2<0.0f){
  float tracking=voice_tracking(v,f);
  v->cutoff_index=pw_filter_mod_signed(f->filter_base,tracking,f->filter_depth,envelope,f->filter_depth2,envelope2,f->lfo_filter_depth,movement);
 }else if(f->track_vel_cutoff!=0.0f||f->track_pitch_cutoff!=0.0f){
  float mod=f->filter_base+f->filter_depth*envelope+f->filter_depth2*envelope2+movement*f->lfo_filter_depth;
  mod+=voice_tracking(v,f);v->cutoff_index=pw_filter_mod_index(mod,0.0f,0.0f);
 }else if(f->lfo_filter_depth>0.0f){
  v->cutoff_index=pw_filter_mod_index(f->filter_base+f->filter_depth*envelope+f->filter_depth2*envelope2+movement*f->lfo_filter_depth,0.0f,0.0f);
 }else{
  v->cutoff_index=pw_filter_mod_index(f->filter_base,f->filter_depth,envelope);
  if(f->filter_depth2>0.0f)v->cutoff_index=pw_filter_mod_index(v->cutoff_index,f->filter_depth2,envelope2);
 }

 if(matrix_cutoff!=0.0f){v->cutoff_index+=matrix_cutoff;if(v->cutoff_index<0)v->cutoff_index=0;if(v->cutoff_index>127)v->cutoff_index=127;}
 if(f->filter_wet>0.0f){
  float g=(matrix_cutoff!=0.0f||f->filter_depth!=0.0f||f->filter_depth2!=0.0f||f->lfo_filter_depth!=0.0f||f->track_vel_cutoff!=0.0f||f->track_pitch_cutoff!=0.0f)?pw_filter_mod_g(v->cutoff_index):f->filter_g;
  pw_filter_tick(&v->filter,dry*(1.0f/32768.0f),g,pw_filter_damping(g,matrix_q!=0.0f?matrix_q:f->filter_q));
  float wet=pw_filter_output(&v->filter,f->filter_mode,dry*(1.0f/32768.0f))*32768.0f;
  dry+=(wet-dry)*f->filter_wet;
  if(v->env.stage==PW_ENV_IDLE&&!pw_filter_live(&v->filter))pw_filter_reset(&v->filter);
 }
 return dry;
}
static void render_voice_range(unsigned first,unsigned last){
 for(unsigned j=0;j<voice_job.count;j++){
  const voice_frame *vf=&voice_job.frame[voice_job.uniform?0:j];
  /* TEST70: no sample-by-sample copying of 104-byte voice_frame objects. */
  const unsigned matrix_on=voice_job.matrix_active;
  const unsigned wave_route=matrix_on&&(voice_job.matrix_position_route||voice_job.matrix_position[j]!=0.0f);
  const float base_position=wave_route?(float)vf->position+voice_job.matrix_position[j]*((float)PW_FRAME_POS_MAX/100.0f):(float)vf->position;
  const float pitch_wave=matrix_on?voice_job.matrix_pitch_wave[j]:0.0f;
  float wave_depth=matrix_on?voice_job.matrix_wave_depth[j]:0.0f;
  if(wave_depth>100.0f)wave_depth=100.0f;else if(wave_depth< -100.0f)wave_depth= -100.0f;
  const unsigned depth_override=matrix_on&&voice_job.matrix_wave_override;
  wave_matrix_frame wave_values;
  const wave_matrix_frame *wave_mod=0;
  if(wave_route||pitch_wave!=0.0f||depth_override){
   wave_values=(wave_matrix_frame){base_position,pitch_wave,wave_depth,wave_route,depth_override};wave_mod=&wave_values;
  }
  float mc=matrix_on?voice_job.matrix_cutoff[j]:0.0f,mq=matrix_on?voice_job.matrix_q[j]:0.0f;
  for(unsigned i=first;i<last;i+=2){
   voice_slot *s=&slots[i];poly_voice *v=&s->main,*t=&s->tail;
   float dl=v->pan_target_left-v->pan_left,dr=v->pan_target_right-v->pan_right;
   v->pan_left+=dl*(1.0f/64.0f);v->pan_right+=dr*(1.0f/64.0f);
   if(dl<.000001f&&dl> -.000001f)v->pan_left=v->pan_target_left;if(dr<.000001f&&dr> -.000001f)v->pan_right=v->pan_target_right;
   int running=voice_job.settings.lfo_phase_mode!=PW_LFO_KEEP&&!v->wait_samples;if(running)advance_lfo(v,vf);float sample=voice_sample(v,running,vf,mc,mq,wave_mod);
   float left=sample*v->pan_left,right=sample*v->pan_right;
   if(s->fade){
    float f=(float)s->fade/(float)STEAL_FADE;
    t->pan_left+=(t->pan_target_left-t->pan_left)*(1.0f/64.0f);t->pan_right+=(t->pan_target_right-t->pan_right)*(1.0f/64.0f);
    float tail=voice_sample(t,0,vf,mc,mq,wave_mod);left=left*(1.0f-f)+(tail*t->pan_left)*(f*s->tail_weight);right=right*(1.0f-f)+(tail*t->pan_right)*(f*s->tail_weight);s->fade--;
   }
   /* Each configured voice contributes equally to the selected 4/6-slot renderer. */
   voice_job.output[i][j][0]=left;voice_job.output[i][j][1]=right;
  }
 }
}
static void render_upper(void *unused){(void)unused;render_voice_range(1,voice_job.render_slots);}
void core_parallel_set(unsigned enabled){voice_parallel=!!enabled;}
unsigned core_parallel_enabled(void){return voice_parallel;}
unsigned core_worker_fault(void){return voice_worker_fault;}
unsigned core_worker_jobs(void){return voice_worker_jobs;}
unsigned core_worker_status(void){return voice_worker_fault?3:dc_status();}
/* Bound work to the next sample that can change voice allocation or gates. */
static unsigned voice_span_length(unsigned limit){
 unsigned span=limit>VOICE_CHUNK?VOICE_CHUNK:limit;
 if(bloom_running){if(ratchet_index+1<ratchet_count&&ratchet_left+1<span)span=ratchet_left+1;if(bloom_left+1<span)span=bloom_left+1;if(bloom_gate&&bloom_gate_left+1<span)span=bloom_gate_left+1;}
 return span;
}
void core_render(int32_t *lr,uint32_t n,uint32_t master){
 if(n){
  /* 8 codec blocks on active DSP/voices, 16 when otherwise idle. The
     audio callback is never allowed an unbounded bank decompression loop. */
  unsigned budget=(st.fx_reverb||st.delay_mode||st.chorus_mode||bloom_running)?8u:16u;
  if(budget==16u)for(unsigned vi=0;vi<VOICES;vi++)if(live(&slots[vi].main)||slots[vi].fade){budget=8u;break;}
  pw_tables_service_budget(st.bank,gain==0,budget);
 }
 if(voice_worker_fault||!pw_tables_ready()){for(uint32_t i=0;i<2*n;i++)lr[i]=0;return;}
 if(panic_requested){
  reverb_active=reverb_ready=0;reverb_mix=0;reverb_sleeping=reverb_zero_samples=0;delay_sleeping=chorus_sleeping=0;
  bloom_running=0;bloom_index=255;bloom_left=bloom_gate_left=0;ratchet_reset();
  pw_delay_reset(&delay,(float)st.delay_time_ms*22.05f+1.0f);delay_active=0;delay_mix=0.0f;delay_feedback=0.0f;
  pw_chorus_reset(&chorus);chorus_active=0;chorus_mix=0.0f;
  for(unsigned p=0;p<3;p++)for(unsigned c=0;c<16;c++)controls[p][c]=(midi_control){0};
  for(unsigned i=0;i<VOICES;i++){defaults(&slots[i].main);defaults(&slots[i].tail);slots[i].fade=0;}
  gain=0;processed_keys=panel_keys;audition_on=0;panic_requested=0;
 }
 bloom_sync();
 /* TEST68: only spend branch-tree generation time in callbacks that do not
    also contain a Bloom note/gate/ratchet boundary or an active steal fade.
    The full tree and RNG order are unchanged; readiness can move by a few
    blocks if the synth is under very heavy generative load. */
 unsigned branch_build_ok=1;
 if(n&&bloom_running){
  if(bloom_left<=n || (bloom_gate && bloom_gate_left<=n) ||
     (ratchet_index+1<ratchet_count && ratchet_left<=n))branch_build_ok=0;
  for(unsigned vi=0;vi<VOICES;vi++)if(slots[vi].fade)branch_build_ok=0;
 }
 if(st.bloom_branches&&branch_build_ok)pw_bloom_service(&bloom,st.bloom_length,st.bloom_scale);
 reconcile_sustain();
 enforce_budget();
 uint32_t keys=panel_keys,changed=keys^processed_keys;
 for(unsigned k=0;k<27;k++)if((changed&(1u<<k))&&!(keys&(1u<<k)))note_off(PANEL_PORT,0,53+k);
 for(unsigned k=0;k<27;k++)if((changed&keys)&(1u<<k))note_on(PANEL_PORT,0,53+k,127);
 processed_keys=keys;
 const core_settings settings=st;
 /* Change shared-buffer owner only after the old effect has faded out.
    Clear incrementally once per block while dry audio/sequence keep running. */
 if(reverb_active!=settings.fx_reverb&&reverb_mix==0.0f&&delay_mix==0.0f){
  reverb_active=settings.fx_reverb;reverb_ready=0;reverb_sleeping=0;reverb_zero_samples=0;delay_sleeping=0;
  if(reverb_active){pw_reverb_configure(&reverb,&delay_memory[0][0],PW_DELAY_FRAMES*2,settings.reverb_model,settings.reverb_size,44100);delay_active=0;}
  else{pw_delay_reset(&delay,(float)settings.delay_time_ms*22.05f+1.0f);delay_active=0;}
 }
 if(reverb_active){
  /* Wake an effect held at zero mix with a clean history. Its 16,744 floats
     are cleared incrementally, never in one expensive audio callback. */
  if(reverb_sleeping&&settings.reverb_mix){
   pw_reverb_configure(&reverb,&delay_memory[0][0],PW_DELAY_FRAMES*2,settings.reverb_model,settings.reverb_size,44100);
   reverb_ready=0;reverb_sleeping=0;reverb_zero_samples=0;
  }
  /* Clouds SPACE changes diffusion in-place; don't discard the reverb tail. */
  pw_reverb_space(&reverb,settings.reverb_size);
  pw_reverb_controls(&reverb,settings.reverb_time_ms,settings.reverb_damping,settings.reverb_speed,settings.reverb_depth);
  if(!reverb_ready&&n&&!reverb_sleeping)reverb_ready=pw_reverb_clear(&reverb,&delay_memory[0][0],2048);
 }
 voice_job.settings=settings;for(unsigned p=0;p<3;p++)for(unsigned c=0;c<16;c++)voice_job.controls[p][c]=controls[p][c];
 output_peaks[0]=output_peaks[1]=0.0f;
 for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main,*t=&slots[i].tail;v->detune_target=(float)pw_unison_detune(v->layers,v->member,settings.unison_detune);t->detune_target=(float)pw_unison_detune(t->layers,t->member,settings.unison_detune);}
 for(unsigned i=0;i<VOICES;i++){poly_voice *v=&slots[i].main,*t=&slots[i].tail;pw_stereo_gains(settings.stereo_mode,settings.stereo_spread,i,v->geometry,v->note,&v->pan_target_left,&v->pan_target_right);pw_stereo_gains(settings.stereo_mode,settings.stereo_spread,i,t->geometry,t->note,&t->pan_target_left,&t->pan_target_right);}
 if(audition_on&&(!settings.playing||settings.bloom_mode||settings.note!=audition_note))note_off(AUDITION_PORT,0,audition_note);
 if(settings.playing&&!settings.bloom_mode&&(!audition_on||settings.note!=audition_note))note_on(AUDITION_PORT,0,settings.note,127);
 audition_on=settings.playing&&!settings.bloom_mode;audition_note=settings.note;
 for(unsigned i=0;i<VOICES;i++){pw_env_shapes(&slots[i].main.env,64+settings.amp_attack_shape,64+settings.amp_decay_shape);pw_env_shapes(&slots[i].main.env2,64+settings.env2_attack_shape,64+settings.env2_decay_shape);pw_env_set(&slots[i].main.env,settings.env);pw_env_set(&slots[i].main.env2,settings.env2);}
 if(master>4096)master=4096;
 unsigned render_slots=voice_span();
 /* When dropping from 6 to 4, keep the upper slots processing until their
    512-sample steal fades finish; this prevents a discontinuity at the switch. */
 if(render_slots==CORE_DEFAULT_VOICES)
  for(unsigned k=CORE_DEFAULT_VOICES;k<VOICES;k++)
   if(live(&slots[k].main)||slots[k].fade){render_slots=VOICES;break;}
 /* 4-voice sound is bit-identical to TEST68; extra capacity gets a gentle
    level reduction to avoid six-note summing overload before Clouds. */
 const float headroom_target=settings.voice_limit>CORE_DEFAULT_VOICES?
  (float)CORE_DEFAULT_VOICES/(float)settings.voice_limit:1.0f;
 pw_delay_controls(&delay,settings.delay_spread,settings.delay_cross,settings.delay_lowcut_hz,settings.delay_highcut_hz);
 const float delay_samples=settings.delay_sync?pw_delay_sync_ms(settings.sequencer_bpm,settings.delay_division,0)*22.05f:(float)settings.delay_time_ms*22.05f+1.0f;
 const int32_t target_gain=settings.bank==active_bank&&!pw_tables_busy()&&pw_tables_bank()==settings.bank?(int32_t)(master*(uint32_t)settings.level/100u):0;
 const float target_synth_gain=pw_gain_db(settings.gain_db),target_return=pw_gain_db(settings.delay_return_db);
 const int32_t target_pos=(int32_t)settings.position*PW_FRAME_POS_MAX/100;
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
 const unsigned matrix_on=pw_matrix_active(&matrix,&settings),matrix_wave_override=pw_matrix_wave_depth_assigned(&settings);
 const float matrix_res_step=n?((float)settings.filter.resonance-matrix_res_base)/(float)n:0.0f;
 const unsigned uniform=base_step==0.0f&&depth_step==0.0f&&depth2_step==0.0f&&inc_step==0.0f&&wave_step==0.0f&&lfo_filter_step==0.0f&&pitch_step==0.0f&&wave_env_step==0.0f&&wave_env2_step==0.0f&&tvw_step==0.0f&&tpw_step==0.0f&&tvc_step==0.0f&&tpc_step==0.0f&&velocity_step==0.0f&&velocity_depth_step==0.0f&&unison_wave_step==0.0f&&tuning_step==0.0f&&env2_pitch_step==0.0f&&(g_step==0.0f||filter_g+g_step==filter_g)&&(q_step==0.0f||filter_q+q_step==filter_q)&&position==target_pos&&gain==target_gain&&filter_mode==settings.filter.mode&&filter_wet==(settings.filter.mode?(float)settings.filter.mix*.01f:0.0f)&&voice_headroom==headroom_target;
 for(uint32_t j=0;j<n;){
  if(bloom_running)bloom_tick();
  unsigned chunk=voice_span_length(n-j);voice_job.count=chunk;voice_job.render_slots=render_slots;voice_job.uniform=uniform;voice_job.matrix_active=matrix_on;voice_job.matrix_wave_override=matrix_wave_override;
  voice_job.matrix_position_route=0;for(unsigned route=0;route<4;route++)if(settings.matrix_source[route]&&(settings.matrix_dest[route]==3||settings.matrix_dest[route]==4)&&settings.matrix_amount[route])voice_job.matrix_position_route=1;
  /* The span ends before any gate/allocation event. Advance only counters. */
  unsigned remaining=chunk-1;
  if(bloom_running){ratchet_left=ratchet_left>remaining?ratchet_left-remaining:0;bloom_left-=remaining;bloom_gate_left=bloom_gate_left>remaining?bloom_gate_left-remaining:0;}
  if(!matrix_on)pw_matrix_advance(&matrix,&settings,chunk);
  for(unsigned q=0;q<(uniform?1:chunk);q++){
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
  if(!gain&&!pw_tables_busy()&&pw_tables_bank()==settings.bank)active_bank=settings.bank;
  /* Smooth gain as polyphony moves from four to six; avoid a level step. */
  if(voice_headroom<headroom_target){voice_headroom+=1.0f/512.0f;if(voice_headroom>headroom_target)voice_headroom=headroom_target;}
  else if(voice_headroom>headroom_target){voice_headroom-=1.0f/512.0f;if(voice_headroom<headroom_target)voice_headroom=headroom_target;}
  voice_job.frame[q]=(voice_frame){lfo_increment,filter_wet,track_vel_wave,track_pitch_wave,unison_wave_depth,wave_env_depth,wave_env2_depth,lfo_wave_depth,env2_pitch_depth,lfo_pitch_depth,tuning_cents,velocity_blend,velocity_depth,filter_depth,filter_depth2,lfo_filter_depth,track_vel_cutoff,track_pitch_cutoff,filter_base,filter_g,filter_q,voice_headroom,position,gain,active_bank,filter_mode};

  }
  if(matrix_on)for(unsigned q=0;q<chunk;q++){
   pw_matrix_tick(&matrix,&settings);matrix_res_base+=matrix_res_step;
   voice_job.matrix_position[q]=matrix.value[2]+matrix.value[3];voice_job.matrix_pitch_wave[q]=matrix.value[4];
   voice_job.matrix_wave_depth[q]=matrix.value[5];voice_job.matrix_cutoff[q]=matrix.value[0];voice_job.matrix_q[q]=matrix.value[1]!=0.0f?pw_matrix_resonance(matrix_res_base+matrix.value[1]):0.0f;
  }
  int submitted=0;
  if(voice_parallel)submitted=dc_submit(render_upper,0);
  render_voice_range(0,render_slots);
  if(submitted){
   if(!dc_wait()){voice_worker_fault=1;for(uint32_t k=j*2;k<n*2;k++)lr[k]=0;return;}
   ++voice_worker_jobs;
  }else render_voice_range(1,render_slots);
  for(unsigned q=0;q<chunk;q++){
  float mix_left=0.0f,mix_right=0.0f;
  for(unsigned i=0;i<render_slots;i++){mix_left+=voice_job.output[i][q][0];mix_right+=voice_job.output[i][q][1];}
  /* No multiplication in the existing 4-voice path. */
  const float poly_headroom=voice_job.frame[uniform?0:q].voice_headroom;
  if(poly_headroom!=1.0f){mix_left*=poly_headroom;mix_right*=poly_headroom;}
  float gain_error=target_synth_gain-synth_gain,next_gain=synth_gain+gain_error*(1.0f/256.0f);if(next_gain==synth_gain||(gain_error<.000001f&&gain_error> -.000001f))synth_gain=target_synth_gain;else synth_gain=next_gain;
  if(synth_gain!=1.0f){mix_left*=synth_gain;mix_right*=synth_gain;}
  if(chorus_active!=settings.chorus_mode&&chorus_mix==0.0f){pw_chorus_reset(&chorus);chorus_active=settings.chorus_mode;chorus_sleeping=1;}
  float target_mix=chorus_active==settings.chorus_mode&&chorus_active?(float)settings.chorus_mix*.01f:0.0f;
  if(chorus_mix<target_mix){chorus_mix+=1.0f/256.0f;if(chorus_mix>target_mix)chorus_mix=target_mix;}
  else if(chorus_mix>target_mix){chorus_mix-=1.0f/256.0f;if(chorus_mix<target_mix)chorus_mix=target_mix;}
  if(chorus_active&&chorus_mix>0.0f&&chorus_sleeping){pw_chorus_reset(&chorus);chorus_sleeping=0;}
  if(chorus_active&&chorus_mix==0.0f&&target_mix==0.0f)chorus_sleeping=1;
  if(chorus_active&&!chorus_sleeping){float input[4]={mix_left,mix_right,0.0f,0.0f},effect[4];pw_chorus_tick_stereo(&chorus,input,effect,chorus_active);if(chorus_mix>0.0f){mix_left+=(effect[0]-mix_left)*chorus_mix;mix_right+=(effect[1]-mix_right)*chorus_mix;if(chorus_active==2){if(chorus_compensation_mix!=chorus_mix){chorus_compensation=pw_chorus_mix_headroom(chorus_mix);chorus_compensation_mix=chorus_mix;}mix_left*=chorus_compensation;mix_right*=chorus_compensation;}}}
  if(!reverb_active){
  if((delay_active!=settings.delay_mode||delay_style_active!=settings.delay_style)&&delay_mix==0.0f){delay_active=settings.delay_mode;delay_style_active=settings.delay_style;pw_delay_reset(&delay,(float)settings.delay_time_ms*22.05f+1.0f);delay_feedback=(float)settings.delay_feedback*.01f;delay_sleeping=1;}
  float target_delay_mix=!settings.fx_reverb&&delay_active==settings.delay_mode&&delay_style_active==settings.delay_style&&delay_active?(delay_style_active?target_return:(float)settings.delay_mix*.01f):0.0f;
  if(delay_mix<target_delay_mix){delay_mix+=1.0f/256.0f;if(delay_mix>target_delay_mix)delay_mix=target_delay_mix;}else if(delay_mix>target_delay_mix){delay_mix-=1.0f/256.0f;if(delay_mix<target_delay_mix)delay_mix=target_delay_mix;}
  if(delay_active&&delay_mix>0.0f&&delay_sleeping){pw_delay_reset(&delay,delay_samples);delay_sleeping=0;}
  if(delay_active&&delay_mix==0.0f&&target_delay_mix==0.0f)delay_sleeping=1;
  if(delay_active&&!delay_sleeping){float target=(float)settings.delay_feedback*.01f,error=target-delay_feedback;delay_feedback+=error*(1.0f/64.0f);if(error<.000001f&&error> -.000001f)delay_feedback=target;float input[2]={mix_left,mix_right},out[2];(delay_style_active?pw_delay_tick_source:pw_delay_tick)(&delay,input,out,delay_active,delay_samples,delay_feedback,delay_mix);mix_left=out[0];mix_right=out[1];}
  }else{
   float target=reverb_ready&&settings.fx_reverb&&reverb.model==settings.reverb_model&&reverb.size==settings.reverb_size?(float)settings.reverb_mix*.01f:0.0f;
   if(reverb_mix<target){reverb_mix+=1.0f/256.0f;if(reverb_mix>target)reverb_mix=target;}
   else if(reverb_mix>target){reverb_mix-=1.0f/256.0f;if(reverb_mix<target)reverb_mix=target;}
   /* Preserve the tail through the 256-sample wet fade; suspend further
      full-rate Clouds ticks at zero wet. Any subsequent wake starts clean. */
   if(settings.reverb_mix==0u&&reverb_mix==0.0f&&target==0.0f){if(reverb_zero_samples<256u)++reverb_zero_samples;}
   else reverb_zero_samples=0;
   if(reverb_zero_samples>=256u)reverb_sleeping=1;
   if(reverb_ready&&!reverb_sleeping){float wet[2];pw_reverb_tick(&reverb,(mix_left+mix_right)*.5f,wet);mix_left+=wet[0]*(.375f*reverb_mix);mix_right+=wet[1]*(.375f*reverb_mix);}
  }
  float scale=(float)voice_job.frame[uniform?0:q].gain*(256.0f/(4096.0f*4.0f));
  float output[2]={mix_left*scale,mix_right*scale};
  for(unsigned k=0;k<2;k++){
   /* Reverb-only soft knee: unity below 90% full scale, bounded peaks above. */
   if(reverb_active&&reverb_mix>0.0f){float a=output[k]<0?-output[k]:output[k];if(a>7549747.0f){float over=a-7549747.0f;a=7549747.0f+838860.0f*(over/(838860.0f+over));output[k]=output[k]<0?-a:a;}}
   if(output[k]>8388607.0f)output[k]=8388607.0f;else if(output[k]< -8388608.0f)output[k]= -8388608.0f;
   float magnitude=output[k]<0.0f?-output[k]:output[k];if(magnitude>output_peaks[k])output_peaks[k]=magnitude;
   lr[2*(j+q)+k]=(int32_t)output[k];
  }
  }
  j+=chunk;
 }
 if(n){matrix_res_base=(float)settings.filter.resonance;velocity_blend=velocity_target;velocity_depth=(float)settings.velocity_amount;unison_wave_depth=(float)settings.unison_wave_spread;tuning_cents=tuning_target;filter_base=(float)settings.filter.cutoff;filter_depth=(float)settings.filter_env_depth;filter_depth2=(float)settings.filter_env2_depth;lfo_increment=increment_target;lfo_wave_depth=(float)settings.lfo.wave;lfo_filter_depth=(float)settings.lfo.cutoff;lfo_pitch_depth=(float)settings.lfo.pitch+(float)settings.lfo_pitch_cents;wave_env_depth=(float)settings.wave_env_depth;wave_env2_depth=(float)settings.wave_env2_depth;track_vel_wave=(float)settings.tracking.velocity_wave;track_pitch_wave=(float)settings.tracking.pitch_wave;track_vel_cutoff=(float)settings.tracking.velocity_cutoff;track_pitch_cutoff=(float)settings.tracking.pitch_cutoff;env2_pitch_depth=(float)settings.env2_pitch;}
}

int32_t core_wave_sample(uint32_t index){
    if(!pw_tables_ready()||pw_tables_busy()||pw_tables_bank()!=st.bank)return 0;
    if(index>255)index=255;
    int32_t pos=(int32_t)st.position*PW_FRAME_POS_MAX/100;
    uint32_t frame=(uint32_t)pos>>8;int32_t mix=pos&255;
    int32_t a,b;if(st.wave_quality){a=pw_table_mips[0][st.bank][frame*1024u+index*4];b=pw_table_mips[0][st.bank][(frame+1)*1024u+index*4];}else{a=pw_table_mips[2][st.bank][frame*256u+index];b=pw_table_mips[2][st.bank][(frame+1)*256u+index];}
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

float core_matrix_voltage(unsigned source){return source<12?matrix.volts[source]:0.0f;}
