#include "fastmath.h"
#include "test_engine.h"
#include "pw_velocity.h"
#include "pw_presets.h"
#include "pw_matrix.h"
#include "pw_tables.h"
#include "pw_analog_bank_names.h"
#include "pw_bloom.h"
#include "pw_delay.h"
#include "pw_reverb.h"
#include "splash_wtables.h"
static unsigned preset_index;
static unsigned matrix_row,note_row,note_column;
#include "panel_lights.h"
#include "panel_overlay.h"
static panel_overlay overlay;
typedef struct {const char *label;char value[32];int numeric,value_num;} overlay_view;
static overlay_view views[4];
static const unsigned preparing_cards=1u;
static uint32_t last_draw_ms,update_elapsed,update_at;
static unsigned update_state;
static int8_t octave_gesture_base;
static core_settings st;
/* A/B changes restore each model's last controls for this session. Preset
   settings retain the existing layout and Clouds remains model 3. */
static pw_reverb_settings reverb_cache[PW_REVERB_MODELS];
static uint16_t reverb_cache_valid;
static void select_reverb(unsigned model){
 if(model>=PW_REVERB_MODELS||model==st.reverb_model)return;
 unsigned old=st.reverb_model;
 if(old<PW_REVERB_MODELS){
  reverb_cache[old]=(pw_reverb_settings){st.reverb_time_ms,st.reverb_size,st.reverb_damping,st.reverb_speed,st.reverb_depth};
  reverb_cache_valid|=(uint16_t)(1u<<old);
 }
 pw_reverb_settings p=(reverb_cache_valid&(1u<<model))?reverb_cache[model]:pw_reverb_defaults(model);
 st.reverb_model=(uint8_t)model;st.reverb_time_ms=(uint16_t)p.time;
 st.reverb_size=(uint8_t)p.size;st.reverb_damping=(uint8_t)p.damping;
 st.reverb_speed=(uint16_t)p.speed;st.reverb_depth=(uint8_t)p.depth;
}
static uint32_t old_buttons;
static uint8_t redraw,page,selected_button,tables_ready_latched;
/* The splash is a one-shot UI state, never a blocking boot delay. Its first
   frame is sent before app_init/audio/TIMER5 on hardware. TIMER4 already runs
   then, so use that continuous clock rather than the not-yet-updated plat_ms.
   The audio IRQ reads only splash_active; all drawing/timing runs on CPU0's
   main loop. The existing canvas is reused, with no additional framebuffer. */
static volatile uint8_t splash_active;
static uint8_t splash_started,splash_ui_seen;
static uint32_t splash_start_ticks,splash_progress_step,splash_blocked_keys;
#ifdef PLAT_HOST
#define SPLASH_TICKS_PER_MS 1u
static uint32_t splash_ticks(void){return plat_ms();}
#else
#define SPLASH_TICKS_PER_MS (1000u*FM1_TICKS_PER_US)
static uint32_t splash_ticks(void){return fm1_ticks();}
#endif
static unsigned splash_elapsed_ticks(uint32_t now,uint32_t start,uint32_t ticks_per_ms){
 uint32_t elapsed=(uint32_t)(now-start);
 if(elapsed>=WT_SPLASH_DURATION_MS*ticks_per_ms)return WT_SPLASH_DURATION_MS;
 return elapsed/ticks_per_ms;
}
static unsigned splash_elapsed_ms(void){
 return splash_elapsed_ticks(splash_ticks(),splash_start_ticks,SPLASH_TICKS_PER_MS);
}
void app_splash(void){
 if(splash_started)return;
 splash_active=splash_started=1;
 wt_splash_draw(0);
 lcd_sync(); /* Start the six seconds only after the whole image is visible. */
 splash_start_ticks=splash_ticks();
 splash_progress_step=0;
}
static void splash_discard_controls(void){
 old_buttons=plat_buttons();
 splash_blocked_keys=plat_keys();
 for(unsigned i=0;i<NE;i++)(void)plat_enc((int)i);
 core_keys(0);
}
static void splash_finish(void){
 splash_discard_controls();
 /* Consume the last queued pre-ready MIDI packets while the audio consumer
    is excluded. A held startup key must be released before it can sound. */
 uint32_t pkt,lock=plat_lock();int port;
 while(plat_midi_in(&pkt,&port)){}
 splash_active=0;
 plat_unlock(lock);
 overlay=(panel_overlay){0};
 redraw=1;last_draw_ms=plat_ms()-33u;
}
static const char *ratchet_labels[]={"OFF","X2","X3","X4","X6","X8"};
static const char *bloom_scales[]={"CHROMATIC","MAJOR","MINOR","PENT 02579","MIN PENT","HARM MINOR","WHOLE TONE","DORIAN","PHRYGIAN","LYDIAN","MIXOLYDIAN","LOCRIAN","MAJ PENT","MELODIC MIN","MIN BLUES","MAJ BLUES","DIM WH"};
static const char *bloom_roots[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
static int filter_page(void){return page==2||page==3||page==5;}
static unsigned shown_note,shown_stage,shown_voices,shown_pending;
static uint32_t meter_ms,meter_cpu,meter_xruns;
static int battery_pct=-1;
static const char *banks[PW_TABLE_BANKS]={"sinharm","cello","granular","fmsynth","hdrawn","flute","eorgan","bw_blended","ebass","fm-calm","karplus-mid","karplus-hi","bw_squ","pwm-lead",PW_ANALOG_BANK_LABELS};
static const char *sustain_modes[]={"LEGACY","SYNTH","PIANO"};
static int limit(int v,int a,int b){return v<a?a:v>b?b:v;}
static void number(int v,char *p){if(v<0){*p++='-';v= -v;}char b[12];int n=0;do{b[n++]=(char)('0'+v%10);v/=10;}while(v);while(n)*p++=b[--n];*p=0;}
static unsigned section_next(unsigned button){
 static const uint8_t groups[][12]={
  {B_GLO,13,14,18,22,23,26,255},
  {B_FX,2,3,5,15,17,38,39,32,33,16,255},{B_LFO,6,9,30,31,255},{B_EDIT,1,11,19,255},
  {B_ENV,1,11,19,4,10,12,255},
  {B_SEQ,27,28,34,35,36,37,255},{B_HOME,0,7,8,255}};
 for(unsigned g=0;g<sizeof(groups)/sizeof(groups[0]);g++)if(groups[g][0]==button){
  unsigned end=1;while(end<12&&groups[g][end]!=255)end++;
  for(unsigned i=1;i<end;i++)if(page==groups[g][i])return groups[g][i+1<end?i+1:1];
  return groups[g][1];
 }return page;
}
static void load_preset(unsigned index){
 if(index>=PW_PRESET_COUNT)return;
 core_settings next=st;if(!pw_preset_apply(index,&next))return;
 uint32_t lock=plat_lock();st=next;core_set(&st);plat_unlock(lock);
 preset_index=index;page=29;redraw=1;
}
void app_init(void){reverb_cache_valid=0;note_row=note_column=0;matrix_row=0;for(unsigned r=0;r<4;r++){st.matrix_source[r]=(uint8_t[]){1,5,9,11}[r];st.matrix_dest[r]=0;st.matrix_amount[r]=0;}preset_index=0;core_init();st.delay_sync=1;st.delay_division=2;st.fx_reverb=0;st.reverb_model=3;st.reverb_size=70;st.reverb_damping=50;st.reverb_depth=20;st.reverb_mix=25;st.reverb_time_ms=2000;st.reverb_speed=25;st.bloom_ratchet=0;st.bloom_ratchet_chance=100;st.bloom_branches=st.bloom_path=0;st.bloom_mode=1;st.bloom_density=100;st.bloom_length=16;st.bloom_scale=2;st.bloom_root=0;st.bloom_offset=0;st.bloom_octave=3;st.bloom_mutation=0;st.bloom_reseed=st.bloom_restore=0;st.playing=0;st.note=48;st.position=0;st.bank=0;st.level=75;st.env=(pw_env_params){50,60,80,60};st.filter=(pw_filter_params){PW_FILTER_OFF,96,0,100};st.filter_env_depth=0;st.env2=(pw_env_params){60,70,64,50};st.filter_env2_depth=0;st.lfo=(pw_lfo_params){40,0,0,0};st.wave_env_depth=st.wave_env2_depth=0;st.tracking=(pw_tracking_params){0,0,0,0};st.lfo_phase_mode=PW_LFO_KEEP;st.lfo_spread=0;st.env2_pitch=0;st.amp_attack_shape=st.amp_decay_shape=st.env2_attack_shape=st.env2_decay_shape=0;st.sequencer_bpm=120;st.bloom_gate_percent=50;st.voice_limit=CORE_STARTUP_VOICES;st.sustain_mode=0;st.lfo_pitch_cents=0;st.velocity_mode=st.velocity_curve=st.trigger_mode=0;st.velocity_amount=100;overlay=(panel_overlay){0};last_draw_ms=0;update_state=update_elapsed=update_at=0;octave_gesture_base=0;st.gain_db=0;st.delay_return_db= -3;st.delay_style=0;st.delay_mode=0;st.delay_time_ms=500;st.delay_feedback=50;st.delay_mix=35;st.delay_spread=0;st.delay_cross=100;st.delay_lowcut_hz=80;st.delay_highcut_hz=8000;st.unison_count=0;st.unison_mode=0;st.unison_detune=10;st.unison_wave_spread=0;st.wave_quality=1;st.chorus_mode=st.stereo_mode=0;st.stereo_spread=0;st.chorus_mix=50;st.coarse_tune=st.fine_tune=st.panel_octave=st.bend_range_offset=0;selected_button=B_HOME;old_buttons=0;tables_ready_latched=0;redraw=1;page=0;shown_note=shown_stage=shown_pending=0;core_set(&st);}
void app_render(int32_t *lr,uint32_t n,uint32_t master){
 const unsigned loading=splash_active;
 uint32_t pkt;int port;
 while(plat_midi_in(&pkt,&port))if(!loading)core_midi((unsigned)port,(uint8_t)(pkt>>8),(uint8_t)(pkt>>16),(uint8_t)(pkt>>24));
 /* Keep bank decompression and normal engine service running during startup,
    but do not queue notes or leave any nonzero samples in the output buffer. */
 core_render(lr,n,loading?0u:master);
 if(loading)for(uint32_t i=0;i<2u*n;i++)lr[i]=0;
}
void app_panic(void){st.playing=0;core_panic();redraw=1;}
void app_say(const char*a,const char*b){(void)a;(void)b;/* No visible text changed; do not redraw a static page. */}
static int note_page(void){return page==36||page==37;}
static void edit_note(unsigned column,int delta){
 unsigned index=(page==37?16:0)+4*note_row+column;
 uint32_t lock=plat_lock();core_bloom_edit(index,delta,st.bloom_scale);plat_unlock(lock);note_column=column;
}
static void input(void){uint32_t b=plat_buttons(),now=plat_ms();int e;
 if(splash_active){splash_discard_controls();return;}
 const uint32_t octmask=(1u<<B_OCTDN)|(1u<<B_OCTUP);int octcombo=(b&octmask)==octmask;
 if((b&octmask)&&!(old_buttons&octmask))octave_gesture_base=st.panel_octave;
 if(octcombo)st.panel_octave=octave_gesture_base;
 if(overlay_master(&overlay,plat_master(),b,now))redraw=1;
 if((b&(1u<<B_PLAY))&&!(old_buttons&(1u<<B_PLAY))){st.bloom_mode=1;st.playing^=1;redraw=1;}
 if((b&(1u<<B_SEQ))&&!(old_buttons&(1u<<B_SEQ))){page=section_next(B_SEQ);selected_button=B_SEQ;redraw=1;}
 if(page==23&&(b&(1u<<B_REC))&&!(old_buttons&(1u<<B_REC))){core_parallel_set(!core_parallel_enabled());redraw=1;}
 if(page==27||page==28||page==34||page==35||note_page()){if((b&(1u<<B_REC))&&!(old_buttons&(1u<<B_REC))){++st.bloom_reseed;redraw=1;}if((b&(1u<<B_SAVE))&&!(old_buttons&(1u<<B_SAVE))){++st.bloom_restore;st.bloom_mutation=0;redraw=1;}}
 if((b&(1u<<B_GLO))&&!(old_buttons&(1u<<B_GLO))){page=section_next(B_GLO);selected_button=B_GLO;redraw=1;}
 if(!octcombo&&(b&(1u<<B_OCTDN))&&!(old_buttons&(1u<<B_OCTDN))){st.panel_octave=(int8_t)limit(st.panel_octave-1,-2,2);selected_button=B_OCTDN;redraw=1;}
 if(!octcombo&&(b&(1u<<B_OCTUP))&&!(old_buttons&(1u<<B_OCTUP))){st.panel_octave=(int8_t)limit(st.panel_octave+1,-2,2);selected_button=B_OCTUP;redraw=1;}
 if((b&(1u<<B_EDIT))&&!(old_buttons&(1u<<B_EDIT))){page=section_next(B_EDIT);selected_button=B_EDIT;redraw=1;}
 if((b&(1u<<B_LFO))&&!(old_buttons&(1u<<B_LFO))){page=section_next(B_LFO);selected_button=B_LFO;redraw=1;}
 if((b&(1u<<B_FX))&&!(old_buttons&(1u<<B_FX))){page=section_next(B_FX);selected_button=B_FX;redraw=1;}
 if((b&(1u<<B_SEL))&&!(old_buttons&(1u<<B_SEL))&&(page==27||page==28||page==34||page==35||page==29)){page=page==29?0:page==27?28:27;selected_button=B_SEL;redraw=1;}
 else if((b&(1u<<B_SEL))&&!(old_buttons&(1u<<B_SEL))&&(filter_page()||page==15||page==17||page==39||page==16||page==32||page==33)){page=section_next(B_FX);selected_button=B_SEL;redraw=1;}
 else if((b&(1u<<B_SEL))&&!(old_buttons&(1u<<B_SEL))&&(page==0||page==7||page==8)){page=page==0?7:page==7?8:0;selected_button=page?B_SEL:B_HOME;redraw=1;}
 else if((b&(1u<<B_SEL))&&!(old_buttons&(1u<<B_SEL))&&(page==6||page==9||page==30||page==31)){page=section_next(B_LFO);selected_button=B_SEL;redraw=1;}
 else if((b&(1u<<B_SEL))&&!(old_buttons&(1u<<B_SEL))&&(page==4||page==10||page==12)){page=page==4?10:page==10?12:4;selected_button=B_SEL;redraw=1;}
 else if((b&(1u<<B_SEL))&&!(old_buttons&(1u<<B_SEL))&&(page==1||page==11||page==19)){page=page==1?11:page==11?19:1;selected_button=B_SEL;redraw=1;}
 else if((b&(1u<<B_SEL))&&!(old_buttons&(1u<<B_SEL))&&(page==13||page==14||page==18||page==22||page==23||page==26)){page=section_next(B_GLO);selected_button=B_SEL;redraw=1;}
 if((b&(1u<<B_ENV))&&!(old_buttons&(1u<<B_ENV))){page=section_next(B_ENV);selected_button=B_ENV;redraw=1;}
 if((b&(1u<<B_HOME))&&!(old_buttons&(1u<<B_HOME))){page=section_next(B_HOME);selected_button=B_HOME;redraw=1;}
 const uint32_t usable=b&~(1u<<B_ARP), old_usable=old_buttons&~(1u<<B_ARP);
 if(usable!=old_usable){if(usable)overlay_event(&overlay,OV_PANEL,usable,now);else if(overlay.active)overlay.at=now;redraw=1;}
 if(usable&&overlay.active)overlay.at=now;
 if(page==30&&matrix_row>1)matrix_row=0;if(page==31&&matrix_row<2)matrix_row=2;
 pw_env_params *ep=page==4||page==12?&st.env2:&st.env;
 if((e=plat_enc(EN_K1))){overlay_event(&overlay,OV_K1,b,now);
  if(page==39){st.delay_spread=(uint8_t)limit(st.delay_spread+e,0,100);}else if(page==38){st.delay_sync=limit(st.delay_sync+e,0,1);}else if(note_page())edit_note(0,e);else if(page==35)st.bloom_ratchet=limit(st.bloom_ratchet+e,0,5);else if(page==34)st.bloom_branches=limit(st.bloom_branches+e,0,7);else if(page==32)st.fx_reverb=limit(st.fx_reverb+e,0,1);else if(page==33)st.reverb_size=limit(st.reverb_size+e,st.reverb_model==PW_REVERB_CLOUDS?1:0,100);else if(page==30||page==31)matrix_row=(unsigned)limit((int)matrix_row+e,page==30?0:2,page==30?1:3);else if(page==29){unsigned next=(unsigned)limit((int)preset_index+e,0,PW_PRESET_COUNT-1);if(next!=preset_index)load_preset(next);}else if(page==27)st.bloom_gate_percent=limit(st.bloom_gate_percent+e,10,90);else if(page==28)st.bloom_scale=limit(st.bloom_scale+e,0,PW_BLOOM_SCALES-1);else if(page==26)st.gain_db=limit(st.gain_db+e,-40,24);else if(page==23)st.voice_limit=limit(st.voice_limit+e,1,CORE_MAX_VOICES);else if(page==22)st.sustain_mode=limit(st.sustain_mode+e,0,2);else if(page==19)st.trigger_mode=(uint8_t)limit(st.trigger_mode+e,0,2);else if(page==18)st.velocity_mode=(uint8_t)limit(st.velocity_mode+e,0,1);else if(page==17){st.delay_mode=(uint8_t)limit(st.delay_mode+e,0,2);if(st.delay_mode)st.fx_reverb=0;}else if(page==16)st.unison_count=(uint8_t)limit((int)core_unison_count(&st)+e,1,CORE_MAX_UNISON);else if(page==15)st.chorus_mode=(uint8_t)limit(st.chorus_mode+e,0,2);else if(page==14)st.wave_quality=(uint8_t)limit(st.wave_quality+e,0,1);else if(page==13)st.coarse_tune=(int8_t)limit(st.coarse_tune+e,-60,60);else if(page==11)st.amp_attack_shape=(int8_t)limit(st.amp_attack_shape+e,-64,63);else if(page==12)st.env2_attack_shape=(int8_t)limit(st.env2_attack_shape+e,-64,63);else if(page==10)st.env2_pitch=(int8_t)limit(st.env2_pitch+e,-120,120);else if(page==9)st.lfo_phase_mode=(uint8_t)limit(st.lfo_phase_mode+e,0,3);else if(page==8)st.tracking.velocity_wave=(int8_t)limit(st.tracking.velocity_wave+e,-100,100);else if(page==7)st.wave_env_depth=(int8_t)limit(st.wave_env_depth+e,-100,100);else if(page==6)st.lfo.rate=(int8_t)limit(st.lfo.rate+e,-100,100);else if(page==5)st.filter_env2_depth=(int8_t)limit(st.filter_env2_depth+e,-127,127);
  else if(page==3)st.filter_env_depth=(int8_t)limit(st.filter_env_depth+e,-127,127);
  else if(page==2)st.filter.cutoff=(uint8_t)limit(st.filter.cutoff+e,0,127);
  else if(page==1||page==4)ep->attack=(uint8_t)limit(ep->attack+e,0,127);
  else st.note=(uint8_t)limit(st.note+e,0,127);redraw=1;
 }
 if((e=plat_enc(EN_K2))){overlay_event(&overlay,OV_K2,b,now);
  if(page==39){st.delay_cross=(uint8_t)limit(st.delay_cross+e,0,100);}else if(page==38){st.delay_division=limit(st.delay_division+e,0,3);}else if(note_page())edit_note(1,e);else if(page==35)st.bloom_ratchet_chance=limit(st.bloom_ratchet_chance+e,0,100);else if(page==34)st.bloom_path=limit(st.bloom_path+e,0,127);else if(page==32)select_reverb((unsigned)limit(st.reverb_model+e,0,PW_REVERB_MODELS-1));else if(page==33)st.reverb_damping=limit(st.reverb_damping+e,0,100);else if(page==30||page==31)st.matrix_source[matrix_row]=limit(st.matrix_source[matrix_row]+e,0,11);else if(page==29)st.level=limit(st.level+e,0,100);else if(page==27)st.bloom_length=limit(st.bloom_length+e,1,32);else if(page==28)st.bloom_root=limit(st.bloom_root+e,0,11);else if(page==26)st.delay_style=limit(st.delay_style+e,0,1);else if(page==23)st.unison_count=limit((int)core_unison_count(&st)+e,1,CORE_MAX_UNISON);else if(page==22)st.env.release=limit(st.env.release+e,0,127);else if(page==19)st.env.sustain=(int8_t)limit(st.env.sustain+e,0,127);else if(page==18)st.velocity_amount=(uint8_t)limit(st.velocity_amount+e,0,100);else if(page==17){if(st.delay_sync)st.delay_division=limit(st.delay_division+e,0,3);else st.delay_time_ms=(uint16_t)limit(st.delay_time_ms+e*5,1,PW_DELAY_MAX_MS);}else if(page==16)st.unison_detune=(uint8_t)limit(st.unison_detune+e,0,100);else if(page==15)st.chorus_mix=(uint8_t)limit(st.chorus_mix+e,0,100);else if(page==14)st.position=(uint8_t)limit(st.position+e,0,100);else if(page==13)st.fine_tune=(int8_t)limit(st.fine_tune+e,-100,100);else if(page==11)st.amp_decay_shape=(int8_t)limit(st.amp_decay_shape+e,-64,63);else if(page==12)st.env2_decay_shape=(int8_t)limit(st.env2_decay_shape+e,-64,63);else if(page==10)st.env2.attack=(uint8_t)limit(st.env2.attack+e,0,127);else if(page==9)st.lfo_spread=(uint8_t)limit(st.lfo_spread+e,0,90);else if(page==8)st.tracking.pitch_wave=(int8_t)limit(st.tracking.pitch_wave+e,-100,100);else if(page==7)st.wave_env2_depth=(int8_t)limit(st.wave_env2_depth+e,-100,100);else if(page==6)st.lfo.wave=(int8_t)limit(st.lfo.wave+e,-100,100);else if(page==3||page==5)st.filter.cutoff=(uint8_t)limit(st.filter.cutoff+e,0,127);
  else if(page==2)st.filter.resonance=(uint8_t)limit(st.filter.resonance+e,0,100);
  else if(page==1||page==4)ep->decay=(uint8_t)limit(ep->decay+e,0,127);
  else st.position=(uint8_t)limit(st.position+e,0,100);redraw=1;
 }
 if((e=plat_enc(EN_K3))){overlay_event(&overlay,OV_K3,b,now);
  if(page==39){st.delay_lowcut_hz=(uint16_t)limit((int)st.delay_lowcut_hz+e*20,20,1000);}else if(page==38){st.sequencer_bpm=limit(st.sequencer_bpm+e,40,240);}else if(note_page())edit_note(2,e);else if(page==35)st.bloom_gate_percent=limit(st.bloom_gate_percent+e,10,90);else if(page==34)st.bloom_density=limit(st.bloom_density+e,0,100);else if(page==32)st.reverb_time_ms=st.reverb_model==PW_REVERB_CLOUDS?limit(st.reverb_time_ms+e*100,400,30000):limit(st.reverb_time_ms+e,0,100);else if(page==33)st.reverb_speed=limit(st.reverb_speed+e,1,500);else if(page==30||page==31)st.matrix_dest[matrix_row]=limit(st.matrix_dest[matrix_row]+e,0,6);else if(page==29)st.bank=limit(st.bank+e,0,PW_TABLE_BANKS-1);else if(page==27)st.sequencer_bpm=limit(st.sequencer_bpm+e,40,240);else if(page==28)st.bloom_octave=limit(st.bloom_octave+e,0,4);else if(page==26)st.delay_return_db=limit(st.delay_return_db+e,-40,0);else if(page==23)st.unison_detune=limit(st.unison_detune+e,0,100);else if(page==22)st.env2.release=limit(st.env2.release+e,0,127);else if(page==19)st.env2.sustain=(int8_t)limit(st.env2.sustain+e,-127,127);else if(page==18)st.velocity_curve=(uint8_t)limit(st.velocity_curve+e,0,3);else if(page==17)st.delay_feedback=(uint8_t)limit(st.delay_feedback+e,0,90);else if(page==16)st.unison_wave_spread=(int8_t)limit(st.unison_wave_spread+e,-100,100);else if(page==15)st.stereo_mode=(uint8_t)limit(st.stereo_mode+e,0,3);else if(page==14)st.bank=(uint8_t)limit(st.bank+e,0,PW_TABLE_BANKS-1);else if(page==13)st.bend_range_offset=(int8_t)limit(st.bend_range_offset+e,-2,46);else if(page==11||page==12)ep->attack=(uint8_t)limit(ep->attack+e,0,127);else if(page==10)st.env2.decay=(uint8_t)limit(st.env2.decay+e,0,127);else if(page==9)st.lfo.rate=(int8_t)limit(st.lfo.rate+e,-100,100);else if(page==8)st.tracking.velocity_cutoff=(int8_t)limit(st.tracking.velocity_cutoff+e,-127,127);else if(page==7)st.lfo.wave=(int8_t)limit(st.lfo.wave+e,-100,100);else if(page==6)st.lfo.cutoff=(int8_t)limit(st.lfo.cutoff+e,-127,127);else if(page==3||page==5)st.filter.resonance=(uint8_t)limit(st.filter.resonance+e,0,100);
  else if(page==2)st.filter.mode=(uint8_t)limit(st.filter.mode+e,0,3);
  else if(page==1||page==4)ep->sustain=(int8_t)limit(ep->sustain+e,page==4?-127:0,127);
  else st.bank=(uint8_t)limit(st.bank+e,0,PW_TABLE_BANKS-1);redraw=1;
 }
 if((e=plat_enc(EN_K4))){overlay_event(&overlay,OV_K4,b,now);
  if(page==39){st.delay_highcut_hz=(uint16_t)limit((int)st.delay_highcut_hz+e*250,500,9500);}else if(page==38){st.delay_time_ms=limit(st.delay_time_ms+e*5,1,PW_DELAY_MAX_MS);}else if(note_page())edit_note(3,e);else if(page==35)st.sequencer_bpm=limit(st.sequencer_bpm+e,40,240);else if(page==34)st.bloom_mutation=limit(st.bloom_mutation+e,0,100);else if(page==32)st.reverb_mix=limit(st.reverb_mix+e,0,100);else if(page==33)st.reverb_depth=limit(st.reverb_depth+e,0,100);else if(page==30||page==31)st.matrix_amount[matrix_row]=limit(st.matrix_amount[matrix_row]+e,-100,100);else if(page==29)st.position=limit(st.position+e,0,100);else if(page==27)st.bloom_mutation=limit(st.bloom_mutation+e,0,100);else if(page==28)st.bloom_offset=limit(st.bloom_offset+e,-24,24);else if(page==26)st.level=limit(st.level+e,0,100);else if(page==23)st.level=limit(st.level+e,0,100);else if(page==22)st.sustain_hold=limit(st.sustain_hold+e,0,1);else if(page==18)st.level=(uint8_t)limit(st.level+e,0,100);else if(page==17){if(st.delay_style)st.delay_return_db=limit(st.delay_return_db+e,-40,0);else st.delay_mix=(uint8_t)limit(st.delay_mix+e,0,100);}else if(page==16){st.stereo_spread=(int8_t)limit(st.stereo_spread+e,-100,100);st.stereo_mode=1;}else if(page==15)st.stereo_spread=(int8_t)limit(st.stereo_spread+e,-100,100);else if(page==14)st.level=(uint8_t)limit(st.level+e,0,100);else if(page==13)st.panel_octave=(int8_t)limit(st.panel_octave+e,-2,2);else if(page==11||page==12)ep->decay=(uint8_t)limit(ep->decay+e,0,127);else if(page==10)st.env2.sustain=(int8_t)limit(st.env2.sustain+e,-127,127);else if(page==9)st.lfo.cutoff=(int8_t)limit(st.lfo.cutoff+e,-127,127);else if(page==8)st.tracking.pitch_cutoff=(int8_t)limit(st.tracking.pitch_cutoff+e,-100,100);else if(page==7)st.position=(uint8_t)limit(st.position+e,0,100);else if(page==6){st.lfo_pitch_cents=(int16_t)limit(st.lfo_pitch_cents+st.lfo.pitch+e,-1200,1200);st.lfo.pitch=0;}else if(page==3||page==5)st.filter.mode=(uint8_t)limit(st.filter.mode+e,0,3);
  else if(page==2)st.filter.mix=(uint8_t)limit(st.filter.mix+e,0,100);
  else if(page==1||page==4)ep->release=(uint8_t)limit(ep->release+e,0,127);
  else st.level=(uint8_t)limit(st.level+e,0,100);redraw=1;
 }
 if((e=plat_enc(EN_PRESET))){overlay_event(&overlay,OV_PRESET,b,now);unsigned next=(unsigned)limit((int)preset_index+e,0,PW_PRESET_COUNT-1);if(next!=preset_index)load_preset(next);else page=29;redraw=1;}
 if((e=plat_enc(EN_SELECT))){if(note_page())note_row=(unsigned)limit((int)note_row+e,0,3);overlay_event(&overlay,OV_SELECT,b,now);redraw=1;}
 if(plat_enc(EN_ALGO)){overlay_event(&overlay,OV_ALGO,b,now);redraw=1;}
 old_buttons=b;core_set(&st);
 uint32_t keys=plat_keys();splash_blocked_keys&=keys;keys&=~splash_blocked_keys;
 core_keys(keys);plat_lights(panel_hardware_mask(panel_transport_mask(b,selected_button,st.playing)),keys,0,0,0);
}
void app_input_only(void){
 /* Do not extend the six-second deadline to the next scheduled UI frame. */
 if(splash_active&&(uint32_t)(splash_ticks()-splash_start_ticks)>=WT_SPLASH_DURATION_MS*SPLASH_TICKS_PER_MS){app_frame();return;}
 input();
}
static void card(int x,int y,const char*label,const char*value){
 unsigned k=(y==102?2u:0u)+(x==124?1u:0u);views[k].label=label;views[k].numeric=0;unsigned i=0;for(;value[i]&&i<31;i++)views[k].value[i]=value[i];views[k].value[i]=0;if(preparing_cards)return;cv_rect(x,y,108,44,C_LINE);cv_text(x+7,y+5,&FONT_XS,label,C_GRAY);cv_text(x+7,y+20,&FONT_B,value,C_WHITE);}
static void numeric(int x,int y,const char*label,int v){char b[12];number(v,b);card(x,y,label,b);unsigned k=(y==102?2u:0u)+(x==124?1u:0u);views[k].numeric=1;views[k].value_num=v;}
static void rate_card(int x,int y,const char *label){
  char rate[16],*p=rate;int milli=(int)(pw_lfo_hz(st.lfo.rate)*1000.0f+0.5f);
  if(st.lfo.rate<0)*p++='-';
  if(!st.lfo.rate){*p++='S';*p++='T';*p++='O';*p++='P';*p=0;}
  else {number(milli/1000,p);while(*p)p++;*p++='.';*p++=(char)('0'+milli/100%10);*p++=(char)('0'+milli/10%10);*p++=(char)('0'+milli%10);*p=0;}
 card(x,y,label,rate);
}
static void reverb_rate_card(int x,int y){
 char value[12],*p=value;number((int)st.reverb_speed/100,p);
 while(*p)p++;*p++='.';*p++=(char)('0'+st.reverb_speed/10%10);
 *p++=(char)('0'+st.reverb_speed%10);*p=0;
 card(x,y,"K3 RATE Hz",value);
}
static const char *fx_modes[]={"DELAY","REVERB"};
static const char *velocity_names[]={"LINEAR","FIXED","HARD","SOFT"};
static void velocity_graph(int x,int y,int w,int h,uint16_t color){
 if(preparing_cards)return;
 cv_rect(x,y,w,h,C_BLACK);cv_line(x,y+h-1,x+w-1,y+h-1,C_LINE);
 cv_line(x,y,x,y+h-1,C_LINE);
 for(int i=1;i<127;i++){
  unsigned v=pw_velocity_map((unsigned)i,1,st.velocity_curve),v2=pw_velocity_map((unsigned)i+1,1,st.velocity_curve);
  cv_line(x+i*(w-1)/127,y+h-1-(int)v*(h-1)/127,x+(i+1)*(w-1)/127,y+h-1-(int)v2*(h-1)/127,color);
 }
}
static void matrix_view(unsigned k,const char *label,const char *value,int numeric,int num){
 views[k].label=label;views[k].numeric=numeric;views[k].value_num=num;unsigned i=0;for(;value[i]&&i<31;i++)views[k].value[i]=value[i];views[k].value[i]=0;
}
static void draw_matrix(void){
 cv_begin(240,240,C_BLACK);cv_text(10,8,&FONT_B,page==30?"LFO MATRIX 1/2":"LFO MATRIX 2/2",C_WHITE);cv_text(196,10,&FONT_XS,st.playing?"PLAY":"STOP",C_HI);
 cv_text(10,32,&FONT_XS,"OUTPUT",C_GRAY);cv_text(128,32,&FONT_XS,"DESTINATION",C_GRAY);
 unsigned first=page==30?0:2;
 for(unsigned i=0;i<2;i++){
  unsigned r=first+i;int y=52+(int)i*73;unsigned color=r==matrix_row?C_HI:C_LINE;
  cv_rect(8,y,108,66,color);cv_rect(9,y+1,106,64,C_LINE);cv_rect(124,y,108,66,color);cv_rect(125,y+1,106,64,C_LINE);
  char row[12];number(r+1,row);cv_text(14,y+5,&FONT_T,"ROUTE",C_GRAY);cv_text(48,y+5,&FONT_T,row,C_HI);
  cv_text(14,y+22,&FONT_B,pw_matrix_source_name(st.matrix_source[r]),r==matrix_row?C_HI:C_WHITE);cv_text(14,y+47,&FONT_XS,pw_matrix_source_rate(st.matrix_source[r]),C_GRAY);
  unsigned d=st.matrix_dest[r];
  if(d==1||d==2){cv_text(130,y+8,&FONT_XS,"FILTER",C_WHITE);cv_text(130,y+23,&FONT_XS,d==1?"CUTOFF":"RESONANCE",C_WHITE);}
  else cv_text(130,y+18,&FONT_XS,pw_matrix_dest_name(d),d?C_WHITE:C_GRAY);
  char val[12];number(st.matrix_amount[r],val);cv_text(130,y+45,&FONT_XS,val,C_HI);cv_text(168,y+45,&FONT_XS,"% AMOUNT",C_GRAY);
 }
 cv_text(10,201,&FONT_XS,"K1 ROUTE   K2 OUTPUT",C_GRAY);cv_text(10,216,&FONT_XS,"K3 DEST     K4 AMOUNT",C_GRAY);
 char cpu[12],xr[12];number(meter_cpu,cpu);number(meter_xruns,xr);cv_text(10,231,&FONT_T,"CPU",C_GRAY);cv_text(36,231,&FONT_T,cpu,C_WHITE);cv_text(70,231,&FONT_T,"XRUN",C_GRAY);cv_text(104,231,&FONT_T,xr,C_WHITE);
 char row[12],amount[12];number(matrix_row+1,row);number(st.matrix_amount[matrix_row],amount);
 matrix_view(0,"K1 ROUTE",row,1,matrix_row+1);matrix_view(1,"K2 LFO OUTPUT",pw_matrix_source_name(st.matrix_source[matrix_row]),0,0);matrix_view(2,"K3 DESTINATION",pw_matrix_dest_name(st.matrix_dest[matrix_row]),0,0);matrix_view(3,"K4 AMOUNT %",amount,1,st.matrix_amount[matrix_row]);
}
static void draw_notes(void){
 unsigned base=page==37?16:0;uint8_t notes[16],enabled[16];
 uint32_t lock=plat_lock();for(unsigned i=0;i<16;i++){notes[i]=core_bloom_seed(base+i);enabled[i]=core_bloom_enabled(base+i);}plat_unlock(lock);
 cv_begin(240,240,C_BLACK);cv_text(8,7,&FONT_B,"STEP NOTES",C_WHITE);
 cv_text(166,9,&FONT_XS,page==37?"17-32":"1-16",C_GRAY);
 cv_text(8,29,&FONT_XS,bloom_roots[st.bloom_root],C_HI);cv_text(32,29,&FONT_XS,bloom_scales[st.bloom_scale],C_GRAY);
 for(unsigned row=0;row<4;row++)for(unsigned col=0;col<4;col++){
  unsigned i=row*4+col,index=base+i;int note=pw_bloom_offset(notes[i],st.bloom_scale,st.bloom_offset)+st.bloom_root;while(note<0)note+=12;
  int x=8+col*57,y=48+row*36;uint16_t color=row==note_row?C_HI:C_WHITE;
  cv_rect(x,y,53,32,row==note_row?C_HI:C_LINE);cv_rect(x+1,y+1,51,30,C_LINE);
  if(!enabled[i]){cv_text(x+7,y+8,&FONT_S,"OFF",color);continue;}
  const char *name=bloom_roots[note%12];char letter[2]={name[0],0};
  cv_text(x+18,y+4,&FONT_M,letter,index<st.bloom_length?color:C_DIM);
  if(name[1])cv_text(x+35,y+6,&FONT_XS,"#",index<st.bloom_length?color:C_DIM);
 }
 for(unsigned col=0;col<4;col++){char k[3]={'K',(char)('1'+col),0};cv_text(25+57*col,196,&FONT_XS,k,C_GRAY);}
 char rowtext[12];number(note_row+1,rowtext);cv_text(8,215,&FONT_XS,"SELECT ROW",C_HI);cv_text(87,215,&FONT_XS,rowtext,C_HI);
 unsigned i=4*note_row+note_column;
 char steptext[12];number(base+i+1,steptext);cv_text(124,215,&FONT_XS,"STEP",C_GRAY);cv_text(155,215,&FONT_XS,steptext,C_GRAY);
 int pitch=pw_bloom_offset(notes[i],st.bloom_scale,st.bloom_offset)+(int)st.bloom_root+12*((int)st.bloom_octave+1+(int)st.panel_octave);
 while(pitch<0)pitch+=12;while(pitch>127)pitch-=12;
 char octave[12];number(pitch/12-1,octave);cv_text(189,215,&FONT_XS,enabled[i]?bloom_roots[pitch%12]:"OFF",C_WHITE);if(enabled[i])cv_text(209,215,&FONT_XS,octave,C_WHITE);
 cv_text(8,230,&FONT_T,"BELOW LOWEST NOTE = OFF",C_GRAY);
}
static int32_t ui_wave_sample(uint32_t index){uint32_t lock=plat_lock();int32_t v=core_wave_sample(index);plat_unlock(lock);return v;}
#define cv_begin(...) ((void)0)
#define cv_rect(...) ((void)0)
#define cv_text(...) ((void)0)
#define cv_line(...) ((void)0)
static void draw_page(void){
 static const char *stages[]={"IDLE","SUSTAIN","ATTACK","DECAY","RELEASE"};
 cv_begin(240,240,C_BLACK);cv_text(10,8,&FONT_B,"POLY WAVETABLE TEST62",C_WHITE);
 char count[12];number(shown_voices,count);cv_text(160,10,&FONT_XS,count,C_HI);
 cv_text(181,10,&FONT_XS,stages[shown_stage<=4?shown_stage:0],C_HI);
 cv_text(10,31,&FONT_XS,page==39?"RIKOSHET / STEREO + FILTER":page==35?"BLOOM / GLOBAL RATCHET":page==34?"BLOOM / BRANCHES + PATH":page==32?(st.reverb_model==PW_REVERB_CLOUDS?"CLOUDS / AMBIENT REVERB":"GP5 / STEREO REVERB"):page==33?(st.reverb_model==PW_REVERB_CLOUDS?"CLOUDS / DAMP + MOTION":"GP5 / TONE + MOTION"):page==29?(preset_index>=25?"AKWF / SOUND DESIGN":preset_index>10?"AKWF / NEW FACTORY SOUNDS":"AKWF / FACTORY PRESETS"):page==27?"BLOOM / 32 STEP INTERNAL CLOCK":page==28?"BLOOM / OUTPUT QUANTIZER":page==26?"GAIN / RECOVERED DELAY RETURN":page==23?"VOICE LIMIT / SHARED ALLOCATION":page==22?"SUSTAIN PEDAL / REPEATED NOTES":page==19?"ENVELOPE TRIGGER / NEW NOTES":page==18?"DYNAMICS / RECOVERED VELOCITY":page==17?(st.fx_reverb?"DELAY PAUSED / REVERB SELECTED":"STEREO + PING-PONG DELAY"):page==16?"UNISON / DETUNE + WAVE SPREAD":page==15?"STEREO OUTPUT + RECOVERED CHORUS":page==14?"WAVETABLE ENGINE / AUTO MIP":page==13?"PERFORMANCE / TUNING + MIDI":page==11?"AMP ENVELOPE / CURVE SHAPE":page==12?"ENVELOPE 2 / CURVE SHAPE":page==10?"ENVELOPE 2 -> PITCH / PER NOTE":page==9?"LFO NOTE PHASE / RETRIGGER":page==8?"VELOCITY + NOTE TRACKING / REF 48":page==7?"ENVELOPES + LFO -> WAVE / PER NOTE":page==6?"RECOVERED SINE LFO / PER VOICE":page==5?"ENVELOPE 2 -> CUTOFF / PER NOTE":page==4?"ENVELOPE 2 / INDEPENDENT ADSR":page==3?"AMP ADSR -> CUTOFF / PER NOTE":page==2?"RECOVERED SVF / PER VOICE":page==1?"AMP ENVELOPE / INDEPENDENT ADSR":"RECOVERED CORE / UP TO 8 VOICES",C_DIM);
 if(page==30||page==31){
  char row[12],amount[12];number(matrix_row+1,row);number(st.matrix_amount[matrix_row],amount);
  matrix_view(0,"K1 ROUTE",row,1,matrix_row+1);
  matrix_view(1,"K2 LFO OUTPUT",pw_matrix_source_name(st.matrix_source[matrix_row]),0,0);
  matrix_view(2,"K3 DESTINATION",pw_matrix_dest_name(st.matrix_dest[matrix_row]),0,0);
  matrix_view(3,"K4 AMOUNT %",amount,1,st.matrix_amount[matrix_row]);return;
 }
 if(page==39){
  numeric(8,52,"K1 SPREAD %",st.delay_spread);numeric(124,52,"K2 CROSS %",st.delay_cross);numeric(8,102,"K3 LOW CUT Hz",st.delay_lowcut_hz);numeric(124,102,"K4 HIGH CUT Hz",st.delay_highcut_hz);
 }else if(page==38){
  card(8,52,"K1 CLOCK",st.delay_sync?"SYNC":"FREE");card(124,52,"K2 DIVISION",pw_delay_division_name(st.delay_division));numeric(8,102,"K3 SEQ BPM",st.sequencer_bpm);numeric(124,102,"K4 FREE ms",st.delay_time_ms);
 }else if(page==32){
  card(8,52,"K1 EFFECT",fx_modes[st.fx_reverb]);card(124,52,"K2 REVERB",pw_reverb_model_name(st.reverb_model));
  numeric(8,102,st.reverb_model==PW_REVERB_CLOUDS?"K3 CLOUD DECAY ms":"K3 DECAY %",st.reverb_time_ms);numeric(124,102,st.reverb_model==PW_REVERB_CLOUDS?"K4 MIX %":"K4 DRY/WET %",st.reverb_mix);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,156,&FONT_XS,"CHORUS + SELECTED EFFECT",C_GRAY);
  cv_text(15,176,&FONT_XS,st.fx_reverb?"REVERB ACTIVE / DELAY PAUSED":"TURN K1 TO SELECT REVERB",C_GRAY);
 }else if(page==33){
  int clouds=st.reverb_model==PW_REVERB_CLOUDS;
  numeric(8,52,clouds?"K1 SPACE %":"K1 TONE %",st.reverb_size);numeric(124,52,"K2 HIGH DAMP %",st.reverb_damping);
  if(clouds)numeric(8,102,"K3 MOTION RATE",st.reverb_speed);else reverb_rate_card(8,102);
  numeric(124,102,"K4 DEPTH %",st.reverb_depth);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,156,&FONT_XS,clouds?"SPACE / CLOUDS DIFFUSION":"TONE / LOW-PASS CUTOFF",C_GRAY);
  cv_text(15,176,&FONT_XS,clouds?"RATE 25 = .13/.19 Hz":"RATE / INPUT MODULATION",C_GRAY);
 }else if(page==29){
  numeric(8,52,"K1 PRESET",preset_index);numeric(124,52,"K2 LEVEL",st.level);
  card(8,102,"K3 WAVE BANK",banks[st.bank]);numeric(124,102,"K4 POSITION",st.position);
  cv_text(12,155,&FONT_M,pw_preset_name(preset_index),C_HI);
  cv_text(12,187,&FONT_XS,pw_preset_category(preset_index),C_GRAY);
 }else if(page==27||page==28||page==34||page==35){
  if(page==27){numeric(8,52,"K1 GATE %",st.bloom_gate_percent);numeric(124,52,"K2 LENGTH",st.bloom_length);numeric(8,102,"K3 BPM",st.sequencer_bpm);numeric(124,102,"K4 MUTATE %",st.bloom_mutation);}
  else if(page==35){card(8,52,"K1 RATCHET",ratchet_labels[st.bloom_ratchet]);numeric(124,52,"K2 CHANCE %",st.bloom_ratchet_chance);numeric(8,102,"K3 GATE %",st.bloom_gate_percent);numeric(124,102,"K4 BPM",st.sequencer_bpm);}
  else if(page==34){numeric(8,52,"K1 BRANCHES",st.bloom_branches);numeric(124,52,"K2 PATH",st.bloom_path);numeric(8,102,"K3 DENSITY %",st.bloom_density);numeric(124,102,"K4 MUTATE %",st.bloom_mutation);}
  else{card(8,52,"K1 SCALE",bloom_scales[st.bloom_scale]);card(124,52,"K2 ROOT",bloom_roots[st.bloom_root]);numeric(8,102,"K3 OCTAVE",st.bloom_octave);numeric(124,102,"K4 ROOT OFFSET",st.bloom_offset);}
  unsigned step=core_bloom_step();
  for(unsigned i=0;i<16;i++){unsigned index=i+(step>=16&&step<32?16:0);int x=10+(int)i*14,h=4+(int)core_bloom_pattern(index)*24/60;cv_rect(x,180-h,11,h,index>=st.bloom_length?C_LINE:st.bloom_mode&&st.playing&&index==step?C_HI:C_GRAY);}
  cv_text(12,187,&FONT_T,st.bloom_mode?(st.playing?"RUNNING  /  REC NEW SEED":"PLAY START  /  REC NEW SEED"):"PLAY START / STOP",C_HI);
 }else if(page==26){
  numeric(8,52,"K1 SYNTH GAIN dB",st.gain_db);card(124,52,"K2 DELAY STYLE",st.delay_style?"SOURCE":"LEGACY");
  numeric(8,102,"K3 RETURN dB",st.delay_return_db);numeric(124,102,"K4 LEVEL %",st.level);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,155,&FONT_XS,"-40 dB = MUTE / GAIN BEFORE FX",C_GRAY);
  cv_text(15,176,&FONT_XS,st.delay_style?"ADDITIVE ECHO / WATCH LEVEL":"RETURN dB APPLIES IN SOURCE",C_GRAY);
 }else if(page==23){
  numeric(8,52,"K1 MAX VOICES",st.voice_limit);numeric(124,52,"K2 UNISON",core_unison_count(&st));
  numeric(8,102,"K3 DETUNE",st.unison_detune);numeric(124,102,"K4 LEVEL",st.level);
  cv_text(12,158,&FONT_XS,core_worker_fault()?"WORKER FAULT / REBOOT":!core_parallel_enabled()?"SINGLE CORE / REC TO COMPARE":core_worker_status()==2?"DUAL CORE / REC TO COMPARE":"SINGLE CORE / WORKER UNAVAILABLE",C_HI);
  unsigned layers=core_unison_effective(&st);char effective[12];number(layers,effective);cv_text(12,176,&FONT_XS,"EFFECTIVE LAYERS",C_GRAY);cv_text(108,176,&FONT_XS,effective,C_HI);
  char active[12];number(core_worker_jobs(),active);cv_text(12,194,&FONT_XS,"CPU1 JOBS",C_GRAY);cv_text(80,194,&FONT_XS,active,C_HI);
 }else if(page==22){
  card(8,52,"K1 PEDAL RESPONSE",sustain_modes[st.sustain_mode]);numeric(124,52,"K2 AMP REL ms",(int)(pw_env_ms(PW_ENV_RELEASE,st.env.release)+.5f));
  numeric(8,102,"K3 ENV2 REL ms",(int)(pw_env_ms(PW_ENV_RELEASE,st.env2.release)+.5f));card(124,102,"K4 HOLD",st.sustain_hold?"ON":"OFF");
  cv_text(12,158,&FONT_XS,st.sustain_mode==1?"SYNTH: KEEP SUSTAINED SOUND":st.sustain_mode==2?"PIANO: START A FRESH STRIKE":"LEGACY: REATTACK CURRENT LEVEL",C_HI);
  cv_text(12,176,&FONT_XS,st.sustain_hold?"HOLD ON: PANEL + MIDI NOTES":"HOLD OFF / MIDI CC64 AVAILABLE",C_GRAY);
  cv_text(12,194,&FONT_XS,"TRIGGERED NOTES IGNORE HOLD",C_GRAY);
 }else if(page==19){
  card(8,52,"K1 TRIGGER MODE",st.trigger_mode==2?"DRONE":st.trigger_mode?"TRIGGERED":"GATED");numeric(124,52,"K2 AMP SUSTAIN %",(st.env.sustain*100+63)/127);
  numeric(8,102,"K3 ENV2 SUSTAIN %",(st.env2.sustain*100+(st.env2.sustain<0?-63:63))/127);numeric(124,102,"K4 LEVEL %",st.level);
  cv_text(12,158,&FONT_XS,st.trigger_mode==2?"LATCH NOTES / PLAY TO CLEAR":st.trigger_mode?"ATTACK -> DECAY -> RELEASE":"RELEASE FOLLOWS NOTE OFF",C_HI);
  cv_text(12,179,&FONT_XS,"MODE APPLIES TO NEW NOTES",C_GRAY);
 }else if(page==18){
  card(8,52,"K1 RESPONSE",st.velocity_mode?"SOURCE":"LEGACY");numeric(124,52,"K2 VELO TO AMP %",st.velocity_amount);
  card(8,102,"K3 MIDI VELOCITY",velocity_names[st.velocity_curve]);numeric(124,102,"K4 LEVEL %",st.level);
  velocity_graph(12,154,100,40,C_HI);
  cv_text(122,156,&FONT_T,"MIDI IN -> OUT",C_GRAY);
  cv_text(122,173,&FONT_T,"NEW NOTES ONLY",C_GRAY);
 }else if(page==17){
  static const char *modes[]={"OFF","STEREO","PING-PONG"};
  card(8,52,"K1 DELAY",modes[st.delay_mode]);if(st.delay_sync)card(124,52,"K2 DIVISION",pw_delay_division_name(st.delay_division));else numeric(124,52,"K2 TIME ms",st.delay_time_ms);
  numeric(8,102,"K3 FEEDBACK %",st.delay_feedback);if(st.delay_style)numeric(124,102,"K4 RETURN dB",st.delay_return_db);else numeric(124,102,"K4 MIX %",st.delay_mix);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,155,&FONT_XS,"RIKOSHET LITE / 1 TO 750 ms",C_GRAY);
  cv_text(15,176,&FONT_XS,st.delay_style?"SOURCE RETURN / PAN TO BOUNCE":"NORMALIZED WET / PAN TO BOUNCE",C_GRAY);
 }else if(page==16){
  numeric(8,52,"K1 UNISON VOICES",core_unison_count(&st));numeric(124,52,"K2 DETUNE AMOUNT",st.unison_detune);
  numeric(8,102,"K3 WAVE SPREAD %",st.unison_wave_spread);numeric(124,102,"K4 VOICE WIDTH %",st.stereo_spread);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,155,&FONT_XS,"LAYERS APPLY TO NEW NOTES",C_GRAY);
  unsigned layers=core_unison_effective(&st);char poly[12],limit_text[12];number(st.voice_limit/layers,poly);number(st.voice_limit,limit_text);
  cv_text(15,176,&FONT_XS,"NOTE GROUPS",C_GRAY);cv_text(85,176,&FONT_XS,poly,C_HI);cv_text(116,176,&FONT_XS,"LIMIT",C_GRAY);cv_text(151,176,&FONT_XS,limit_text,C_HI);
 }else if(page==15){
  static const char *chorus_modes[]={"OFF","MODE 1","MODE 2"},*pan_modes[]={"MONO","VOICE","PAIRS","PITCH"};
  card(8,52,"K1 CHORUS",chorus_modes[st.chorus_mode]);numeric(124,52,"K2 CHORUS MIX %",st.chorus_mix);
  card(8,102,"K3 STEREO MODE",pan_modes[st.stereo_mode]);numeric(124,102,"K4 SPREAD %",st.stereo_spread);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,155,&FONT_XS,"OUTPUT LEVEL / LEFT + RIGHT",C_GRAY);
  cv_text(15,174,&FONT_XS,"L",C_GRAY);cv_rect(27,175,(int)(core_output_level(0)*87.0f),8,C_HI);
  cv_text(125,174,&FONT_XS,"R",C_GRAY);cv_rect(137,175,(int)(core_output_level(1)*87.0f),8,C_HI);
 }else if(page==14){
  card(8,52,"K1 WAVE ENGINE",st.wave_quality?"AUTO":"LEGACY");numeric(124,52,"K2 WAVE POS %",st.position);
  card(8,102,"K3 BANK",banks[st.bank]);numeric(124,102,"K4 LEVEL %",st.level);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,155,&FONT_XS,"VOICE TABLE SAMPLES / LIVE",C_GRAY);
  for(unsigned i=0;i<4&&!preparing_cards;i++){int x=15+(int)i*52;char value[12];unsigned mip=core_voice_mip(i);if(core_voice_pitch(i)< -1500.0f){value[0]='-';value[1]='-';value[2]=0;}else number(mip?(int)(2048u>>mip):256,value);cv_text(x,177,&FONT_XS,value,C_HI);}
 }else if(page==13){
  numeric(8,52,"K1 COARSE st",st.coarse_tune);numeric(124,52,"K2 FINE ct",st.fine_tune);
  numeric(8,102,"K3 BEND RANGE st",2+st.bend_range_offset);numeric(124,102,"K4 KEY OCTAVE",st.panel_octave);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,155,&FONT_XS,"CC71 RESO / CC74 CUTOFF",C_GRAY);
  unsigned pedals=0;for(unsigned p=0;p<3;p++)for(unsigned c=0;c<16;c++)pedals+=core_sustain(p,c);
  char count[12];number((int)pedals,count);cv_text(15,176,&FONT_XS,"PEDALS",C_GRAY);cv_text(61,176,&FONT_XS,count,C_HI);
  cv_text(90,176,&FONT_XS,"OCT: KEYS / SEQ STEPS",C_GRAY);
 }else if(page==11||page==12){
  int attack=64+(page==11?st.amp_attack_shape:st.env2_attack_shape),decay=64+(page==11?st.amp_decay_shape:st.env2_decay_shape);
  pw_env_params ep=page==11?st.env:st.env2;
  numeric(8,52,"K1 ATTACK SHAPE",attack);numeric(124,52,"K2 DECAY SHAPE",decay);
  numeric(8,102,"K3 ATTACK ms",(int)(pw_env_ms(PW_ENV_ATTACK,ep.attack)+.5f));numeric(124,102,"K4 DECAY ms",(int)(pw_env_ms(PW_ENV_DECAY,ep.decay)+.5f));
  cv_rect(8,152,224,44,C_LINE);cv_text(15,153,&FONT_XS,"ATTACK / DECAY + RELEASE",C_GRAY);
  float ra=pw_env_ratio((unsigned)attack),rd=pw_env_ratio((unsigned)decay);
  for(int x=0;x<100&&!preparing_cards;x++){
   float t=(float)x/99.0f;
   float up=(1.0f+ra)*(1.0f-fm_expf(-t*fm_logf((1.0f+ra)/ra)));
   float down=(1.0f+rd)*fm_expf(-t*fm_logf((1.0f+rd)/rd))-rd;
   cv_rect(15+x,190-(int)(up*20.0f),1,2,C_HI);cv_rect(123+x,190-(int)(down*20.0f),1,2,C_HI);
  }
 }else if(page==10){
  numeric(8,52,"K1 ENV2 PITCH ct",st.env2_pitch*10);numeric(124,52,"K2 ATTACK ms",(int)(pw_env_ms(PW_ENV_ATTACK,st.env2.attack)+.5f));
  numeric(8,102,"K3 DECAY ms",(int)(pw_env_ms(PW_ENV_DECAY,st.env2.decay)+.5f));numeric(124,102,"K4 SUSTAIN %",(st.env2.sustain*100+(st.env2.sustain<0?-63:63))/127);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,153,&FONT_XS,"VOICE PITCH / -24 TO +24 st",C_GRAY);
  for(unsigned i=0;i<4&&!preparing_cards;i++){int y=171+(int)i*6;float pitch=core_voice_pitch(i);cv_rect(15,y,208,3,C_DIM);cv_rect(119,y,1,3,C_WHITE);if(pitch> -1500.0f)cv_rect(119+(int)(pitch*.04f),y,2,3,C_HI);}
 }else if(page==9){
  static const char *modes[]={"KEEP","POLY","MONO","OFF"};
  card(8,52,"K1 RETRIGGER",modes[st.lfo_phase_mode]);numeric(124,52,"K2 SPREAD deg",st.lfo_spread);
  rate_card(8,102,"K3 LFO RATE Hz");numeric(124,102,"K4 CUTOFF st",st.lfo.cutoff);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,153,&FONT_XS,"VOICE PHASES / ONE FULL CYCLE",C_GRAY);
  for(unsigned i=0;i<4&&!preparing_cards;i++){int y=171+(int)i*6,x=15+(int)((core_voice_phase(i)>>16)*207u/65536u);cv_rect(15,y,208,3,C_DIM);cv_rect(x,y,2,3,C_HI);}
 }else if(page==8){
  numeric(8,52,"K1 VELO WAVE %",st.tracking.velocity_wave);numeric(124,52,"K2 PITCH WAVE %",st.tracking.pitch_wave);
  numeric(8,102,"K3 VELO CUT st",st.tracking.velocity_cutoff);numeric(124,102,"K4 PITCH CUT %",st.tracking.pitch_cutoff);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,153,&FONT_XS,"WAVE POS",C_GRAY);cv_text(127,153,&FONT_XS,"CUTOFF",C_GRAY);
  for(unsigned i=0;i<4&&!preparing_cards;i++){
   float wave=core_voice_wave(i),cutoff=core_voice_cutoff(i);int y=171+(int)i*6;
   cv_rect(15,y,94,3,C_DIM);cv_rect(127,y,96,3,C_DIM);
   if(wave>=0.0f)cv_rect(15,y,(int)(wave*.94f),3,C_HI);
   if(cutoff>=0.0f)cv_rect(127,y,(int)(cutoff*(96.0f/127.0f)),3,C_WHITE);
  }
 }else if(page==7){
  numeric(8,52,"K1 AMP WAVE %",st.wave_env_depth);numeric(124,52,"K2 ENV2 WAVE %",st.wave_env2_depth);
  numeric(8,102,"K3 LFO WAVE %",st.lfo.wave);numeric(124,102,"K4 BASE WAVE %",st.position);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,153,&FONT_XS,"VOICE WAVE POSITIONS / 0-100%",C_GRAY);
  for(unsigned i=0;i<4&&!preparing_cards;i++){float pos=core_voice_wave(i);cv_rect(15,171+(int)i*6,208,3,C_DIM);if(pos>=0.0f)cv_rect(15,171+(int)i*6,(int)(pos*2.08f),3,C_HI);}
 }else if(page==6){
  rate_card(8,52,"K1 LFO RATE Hz");numeric(124,52,"K2 WAVE DEPTH %",st.lfo.wave);
  numeric(8,102,"K3 CUTOFF st",st.lfo.cutoff);numeric(124,102,"K4 LFO PITCH ct",st.lfo.pitch+st.lfo_pitch_cents);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,153,&FONT_XS,"LFO CYCLE / LFO PHASE OPTIONS",C_GRAY);
  for(unsigned j=0;j<107&&!preparing_cards;j++){
   pw_lfo a={(uint32_t)j*(UINT32_MAX/107u)},b={(uint32_t)(j+1)*(UINT32_MAX/107u)};
   cv_line(14+(int)j*2,181-(int)(pw_lfo_value(&a)*10.0f),16+(int)j*2,181-(int)(pw_lfo_value(&b)*10.0f),C_HI);
  }
  /* No live LFO phase cursor. */
 }else if(page==3||page==5){
  static const char *modes[]={"OFF","LOWPASS","BANDPASS","HIGHPASS"};
  numeric(8,52,page==5?"K1 ENV2 DEPTH st":"K1 AMP DEPTH st",page==5?st.filter_env2_depth:st.filter_env_depth);
  numeric(124,52,"K2 BASE Hz",(int)(pw_filter_hz(st.filter.cutoff)+0.5f));
  numeric(8,102,"K3 RESONANCE %",st.filter.resonance);card(124,102,"K4 FILTER",modes[st.filter.mode]);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,153,&FONT_XS,"VOICE CUTOFFS / BOTH ENVELOPES",C_GRAY);
  for(unsigned i=0;i<4&&!preparing_cards;i++){float cutoff=core_voice_cutoff(i);cv_rect(15,171+(int)i*6,208,3,C_DIM);if(cutoff>=0.0f)cv_rect(15,171+(int)i*6,(int)(cutoff*(208.0f/127.0f)),3,C_HI);}
 }else if(page==2){
  static const char *modes[]={"OFF","LOWPASS","BANDPASS","HIGHPASS"};
  numeric(8,52,"K1 CUTOFF Hz",(int)(pw_filter_hz(st.filter.cutoff)+0.5f));
  numeric(124,52,"K2 RESONANCE %",st.filter.resonance);
  card(8,102,"K3 FILTER",modes[st.filter.mode]);numeric(124,102,"K4 MIX %",st.filter.mix);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,156,&FONT_XS,"LFO Depths  ENV Envelopes",C_GRAY);
  cv_text(15,175,&FONT_XS,st.filter.mode?"FILTER ON / PER VOICE":"BYPASS / TEST4 SOUND",C_HI);
 }else if(page==1||page==4){
  const pw_env_params *ep=page==4||page==12?&st.env2:&st.env;
  numeric(8,52,"K1 ATTACK ms",(int)(pw_env_ms(PW_ENV_ATTACK,ep->attack)+0.5f));
  numeric(124,52,"K2 DECAY ms",(int)(pw_env_ms(PW_ENV_DECAY,ep->decay)+0.5f));
  numeric(8,102,"K3 SUSTAIN %",(ep->sustain*100+(ep->sustain<0?-63:63))/127);
  numeric(124,102,"K4 RELEASE ms",(int)(pw_env_ms(PW_ENV_RELEASE,ep->release)+0.5f));
  cv_rect(8,152,224,44,C_LINE);cv_text(15,156,&FONT_XS,page==4?"ENVELOPE 2 LEVEL":"AMP ENVELOPE LEVEL",C_GRAY);
  if(!preparing_cards&&page==4){float v=core_env2_level();cv_rect(119,173,1,14,C_GRAY);cv_rect(v<0?119+(int)(v*104):119,175,(int)((v<0?-v:v)*104),10,C_HI);}else if(!preparing_cards)cv_rect(15,175,(int)(core_env_level()*208.0f),10,C_HI);
 }else{
  numeric(8,52,"K1 NOTE",st.note);numeric(124,52,"K2 WAVE POS %",st.position);
  card(8,102,"K3 BANK",banks[st.bank]);numeric(124,102,"K4 LEVEL %",st.level);
  cv_rect(8,152,224,44,C_LINE);cv_text(15,155,&FONT_XS,"BASE WAVE SHAPE",C_GRAY);
  for(int j=0;j<107&&!preparing_cards;j++){
   int y1=181-(ui_wave_sample((uint32_t)j*255u/107u)*10)/32768;
   int y2=181-(ui_wave_sample((uint32_t)(j+1)*255u/107u)*10)/32768;
   cv_line(14+j*2,y1,16+j*2,y2,C_HI);
  }
 }
 const char *hint=page==29?"PRESET Browse  PLAY Audition":page==27||page==28||page==34||page==35?"SEQ Next page  SAVE Seed":page==13||page==14||page==18||page==22||page==23||page==26?"GLO Next page  HOME Main":page==6||page==9?"LFO Next page  HOME Main":(filter_page()||page==15||page==17||page==39||page==16||page==32||page==33)?"FX Next page  HOME Main":page==0||page==7||page==8?"HOME Next page  GLO Global":page==1||page==11||page==19?"ENV/EDIT Next  HOME Main":"ENV Next page  HOME Main";
 cv_text(10,208,&FONT_XS,hint,C_GRAY);
 char cpu[12],xruns[12];number((int)meter_cpu,cpu);number((int)meter_xruns,xruns);
 cv_text(10,224,&FONT_XS,"CPU",C_GRAY);cv_text(35,224,&FONT_XS,cpu,C_GRAY);cv_text(57,224,&FONT_XS,"%",C_GRAY);
 cv_text(92,224,&FONT_XS,"XRUN",C_GRAY);cv_text(129,224,&FONT_XS,xruns,C_GRAY);
 char octave[12];number(st.panel_octave,octave);cv_text(172,224,&FONT_XS,"OCT",C_GRAY);cv_text(199,224,&FONT_XS,octave,C_HI);
}
#undef cv_begin
#undef cv_rect
#undef cv_text
#undef cv_line
#include "panel_overlay_draw.h"
#include "update_overlay.h"
#include "large_panel.h"
static void draw(void){
 if(update_state==1||update_state==3||(update_state==2&&(uint32_t)(plat_ms()-update_at)<700u))draw_update_overlay();
 else if(splash_active){wt_splash_draw(splash_elapsed_ms());return;}
 else if(!pw_tables_ready()){
  cv_begin(240,240,C_BLACK);cv_text(10,20,&FONT_B,"WAVETABLE BANK",C_WHITE);
  cv_text(10,50,&FONT_B,"MISSING OR INVALID",C_HI);
  cv_text(10,96,&FONT_XS,"INSTALL MATCHING DATA BANK",C_GRAY);
  cv_text(10,116,&FONT_XS,"THEN RESTART",C_GRAY);
  cv_text(10,158,&FONT_XS,"AUDIO MUTED",C_WHITE);
  cv_text(10,196,&FONT_XS,"OCT- + OCT+ : BOOTLOADER",C_GRAY);
 }
 else if(overlay_visible(&overlay,old_buttons&~(1u<<B_ARP),plat_ms())&&overlay.kind==OV_PANEL)draw_overlay();
 else if(note_page())draw_notes();
 else{draw_page();draw_large_panel();if(overlay_visible(&overlay,old_buttons&~(1u<<B_ARP),plat_ms()))draw_overlay();}
 cv_blit(0,0);
}
void app_update_status(unsigned state,uint32_t elapsed_ms){
 if(state==1){if(update_state!=1||update_elapsed/50!=elapsed_ms/50)redraw=1;update_state=1;update_elapsed=elapsed_ms;}
 else if(state==0&&update_state==1){update_state=2;update_at=plat_ms();redraw=1;}
 else if(state==2){update_state=3;update_elapsed=5000;core_panic();draw_update_overlay();cv_blit(0,0);}
}
void app_ui_init(void){
 /* Host boot enters here without the hardware's early app_splash hook.
    Returning from OTA also enters here: it must never restart the splash. */
 if(!splash_started)app_splash();
 if(splash_ui_seen&&splash_active)splash_finish();
 splash_ui_seen=1;
 if(!splash_active)lcd_fill(0,0,240,240,C_BLACK);
 redraw=1;last_draw_ms=plat_ms()-33u;
}
void app_frame(void){
 if(splash_active){
  unsigned elapsed=splash_elapsed_ms();
  if(elapsed>=WT_SPLASH_DURATION_MS)splash_finish();
  else if(elapsed/100u!=splash_progress_step){splash_progress_step=elapsed/100u;redraw=1;}
 }
 input();
 /* One-shot startup/bank-load transition: do not leave the loading warning
    permanently visible after the data become ready. */
 const unsigned tables_ready=pw_tables_ready();
 if(tables_ready!=tables_ready_latched){tables_ready_latched=(uint8_t)tables_ready;redraw=1;}
 /* TEST70: sample diagnostics without triggering LCD traffic. The display
    updates only for user controls, overlay expiry, or the firmware updater. */
 if((uint32_t)(plat_ms()-meter_ms)>=500){meter_ms=plat_ms();battery_pct=plat_battery_pct();meter_cpu=plat_cpu_pct();meter_xruns=plat_xruns();}
 if(overlay.active&&!overlay_visible(&overlay,old_buttons&~(1u<<B_ARP),plat_ms())){overlay.active=0;redraw=1;}
 if(update_state==2&&(uint32_t)(plat_ms()-update_at)>=700u){update_state=0;redraw=1;}
 if(redraw&&(uint32_t)(plat_ms()-last_draw_ms)>=33u){
  shown_voices=core_voice_count();shown_note=core_active_note();shown_stage=core_env_stage();shown_pending=core_pending_count();
  redraw=0;last_draw_ms=plat_ms();draw();
 }
}

#ifdef PLAT_HOST
void app_host_reboot(void){
 splash_active=splash_started=splash_ui_seen=0;
 splash_start_ticks=splash_progress_step=splash_blocked_keys=0;
}
int app_expect(const char *name,int *value){
 if(!strcmp(name,"splash_active"))*value=splash_active;
 else if(!strcmp(name,"splash_elapsed_ms"))*value=(int)splash_elapsed_ms();
 else if(!strcmp(name,"blocked_keys"))*value=(int)splash_blocked_keys;
 else if(!strcmp(name,"update_state"))*value=(int)update_state;
 else if(!strcmp(name,"held_voices"))*value=(int)core_held_count();
 else if(!strcmp(name,"tables_busy"))*value=pw_tables_busy();
 else if(!strcmp(name,"tables_bank"))*value=(int)pw_tables_bank();
 else if(!strcmp(name,"tables_ready"))*value=pw_tables_ready();
 else if(!strcmp(name,"matrix_route"))*value=(int)matrix_row+1;
 else if(!strcmp(name,"overlay_on"))*value=overlay_visible(&overlay,old_buttons&~(1u<<B_ARP),plat_ms());
 else if(!strcmp(name,"overlay_kind"))*value=overlay.kind;
 else if(!strcmp(name,"matrix_dest"))*value=st.matrix_dest[matrix_row];
 else if(!strcmp(name,"matrix_wave_override"))*value=pw_matrix_wave_depth_assigned(&st);
 else if(!strcmp(name,"matrix_source"))*value=st.matrix_source[matrix_row];
 else if(!strcmp(name,"matrix_amount"))*value=st.matrix_amount[matrix_row];
 else if(!strcmp(name,"play_led"))*value=!!(panel_transport_mask(old_buttons,selected_button,st.playing)&(1u<<B_PLAY));
 else if(!strcmp(name,"fx_reverb"))*value=st.fx_reverb;
 else if(!strcmp(name,"reverb_model"))*value=st.reverb_model;
 else if(!strcmp(name,"reverb_mix"))*value=st.reverb_mix;
 else if(!strcmp(name,"reverb_time_ms"))*value=st.reverb_time_ms;
 else if(!strcmp(name,"reverb_size"))*value=st.reverb_size;
 else if(!strcmp(name,"reverb_damping"))*value=st.reverb_damping;
 else if(!strcmp(name,"reverb_speed"))*value=st.reverb_speed;
 else if(!strcmp(name,"reverb_depth"))*value=st.reverb_depth;
 else if(!strcmp(name,"bank"))*value=st.bank;
 else if(!strcmp(name,"page"))*value=page;
 else if(!strcmp(name,"panel_octave"))*value=st.panel_octave;
 else if(!strcmp(name,"trigger_mode"))*value=st.trigger_mode;
 else if(!strcmp(name,"bloom_branches"))*value=st.bloom_branches;
 else if(!strcmp(name,"note_row"))*value=note_row;
 else if(!strncmp(name,"seed",4)){unsigned i=0;const char *p=name+4;if(!*p)return 0;while(*p){if(*p<'0'||*p>'9')return 0;i=i*10+(*p++-'0');if(i>=PW_BLOOM_STEPS)return 0;}*value=core_bloom_seed(i);}
 else if(!strcmp(name,"bloom_density"))*value=st.bloom_density;
 else if(!strcmp(name,"step_enabled"))*value=core_bloom_enabled((page==37?16:0)+4*note_row+note_column);
 else if(!strcmp(name,"bloom_offset"))*value=st.bloom_offset;
 else if(!strcmp(name,"bloom_scale"))*value=st.bloom_scale;
 else if(!strcmp(name,"bloom_gate"))*value=st.bloom_gate_percent;
 else if(!strcmp(name,"bloom_length"))*value=st.bloom_length;
 else if(!strcmp(name,"bloom_ratchet"))*value=st.bloom_ratchet;
 else if(!strcmp(name,"bloom_ratchet_chance"))*value=st.bloom_ratchet_chance;
 else if(!strcmp(name,"bloom_path"))*value=st.bloom_path;
 else if(!strcmp(name,"bloom_mode"))*value=st.bloom_mode;
 else if(!strcmp(name,"bloom_step"))*value=core_bloom_step();
 else if(!strcmp(name,"bloom_note"))*value=core_bloom_note();
 else if(!strcmp(name,"delay_spread"))*value=st.delay_spread;
 else if(!strcmp(name,"delay_cross"))*value=st.delay_cross;
 else if(!strcmp(name,"delay_lowcut_hz"))*value=st.delay_lowcut_hz;
 else if(!strcmp(name,"delay_highcut_hz"))*value=st.delay_highcut_hz;
 else if(!strcmp(name,"delay_sync"))*value=st.delay_sync;
 else if(!strcmp(name,"delay_division"))*value=st.delay_division;
 else if(!strcmp(name,"voice_limit"))*value=st.voice_limit;
 else if(!strcmp(name,"preset"))*value=(int)preset_index;
 else if(!strcmp(name,"preset_page"))*value=page==29;
 else if(!strcmp(name,"playing"))*value=st.playing;
 else if(!strcmp(name,"voices"))*value=core_primary_count();
 else if(!strcmp(name,"dual_enabled"))*value=core_parallel_enabled();
 else if(!strcmp(name,"default_match")){core_settings initial;pw_preset_default(&initial);initial.bloom_mode=1;*value=!memcmp(&st,&initial,sizeof(st));}
 else return 0;return 1;
}
#endif
