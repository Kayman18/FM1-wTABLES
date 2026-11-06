/* Included after the normal page renderer. Reuses its four current card values. */
#define OV_AMBER RGB(255,182,0)
#define OV_PANEL_BG RGB(33,32,38)
static const char *overlay_page_name(void){
 static const char *names[]={"POLY WAVETABLE","AMP ADSR","FILTER","AMP TO CUTOFF","ENVELOPE 2 ADSR","ENV2 TO CUTOFF","LFO DEPTHS","WAVE MODULATION","NOTE TRACKING","LFO PHASE","ENV2 TO PITCH","AMP CURVE SHAPE","ENV2 CURVE SHAPE","PERFORMANCE","WAVE ENGINE","STEREO / CHORUS","UNISON","DELAY","DYNAMICS","ENVELOPE TRIGGER","UNUSED","UNUSED","SUSTAIN PEDAL","VOICE LIMIT","UNUSED","UNUSED","GAIN / DELAY RETURN","BLOOM SEQUENCER","BLOOM QUANTIZER","SOUND PRESETS","LFO MATRIX 1/2","LFO MATRIX 2/2","REVERB","REVERB TONE","BLOOM BRANCHES","BLOOM RATCHET"};
 if(page==36)return "STEP NOTES 1/2";
 if(page==37)return "STEP NOTES 2/2";
 if(page==38)return "DELAY CLOCK";
 if(page==39)return "RIKOSHET TONE";
 return names[page<36?page:0];
}
static void overlay_text(int x,int y,const felucca_font_t *font,const char *s,uint16_t color){
 /* Every dynamic label fits the LCD; numeric fonts only see numeric strings. */
 if(text_w(font,s)>216)font=&FONT_L;
 if(text_w(font,s)>216)font=&FONT_M;
 if(text_w(font,s)>216)font=&FONT_B;
 cv_text(x,y,font,s,color);
}
static void overlay_trigger(void){
 static const char *names[]={"FX","SEL","ENV","LFO","EDIT","GLO","HOME","SAVE","ARP","SEQ","PLAY","REC","OCT-","OCT+"};
 unsigned count=0;for(unsigned i=0;i<NB;i++)if(overlay.buttons&(1u<<i))count++;
 const felucca_font_t *f=count>=7?&FONT_T:&FONT_XS;
 int x=12,y=8;cv_rect(0,0,240,40,OV_PANEL_BG);
 for(unsigned i=0;i<NB;i++)if(overlay.buttons&(1u<<i)){
  int w=text_w(f,names[i]);if(x+w+(x>12?(count>=7?7:10):0)>228){x=12;y+=13;}if(y>21)break;
  if(x>12){cv_text(x,y,f,"+",OV_AMBER);x+=count>=7?7:10;}
  cv_text(x,y,f,names[i],OV_AMBER);x+=w+(count>=7?3:5);
 }
 if(overlay.kind!=OV_PANEL){
  static const char *names2[]={"","K1","K2","K3","K4","PRESET","MASTER","SELECT","ALGO"};
  const char *name=names2[overlay.kind];int w=text_w(&FONT_XS,name);
  if(x+w+10>228){x=12;y+=13;}if(y<=21){if(x>12){cv_text(x,y,&FONT_XS,"/",OV_AMBER);x+=9;}cv_text(x,y,&FONT_XS,name,OV_AMBER);}
 }
}
static void overlay_footer(void){
 cv_rect(12,220,216,1,C_LINE);
 cv_text(12,227,&FONT_T,old_buttons?"HELD / LIVE":overlay.kind==OV_PANEL?"RELEASED / LAST PANEL":"LAST VALUE",C_GRAY);
 if(!old_buttons){unsigned elapsed=(uint32_t)(plat_ms()-overlay.at);unsigned timeout=overlay_timeout(&overlay);unsigned remain=elapsed<timeout?timeout-elapsed:0;cv_rect(176,229,(int)(52u*remain/timeout),3,OV_AMBER);}
}
static void overlay_bar(int value,int lo,int hi){
 int pos=hi>lo?(int)(((int64_t)value-lo)*216/(hi-lo)):0;pos=limit(pos,0,216);
 cv_rect(12,184,216,7,OV_PANEL_BG);cv_rect(12,184,pos,7,OV_AMBER);
 if(lo<0&&hi>0){int center=(int)((-(int64_t)lo)*216/(hi-lo));cv_rect(12+center,181,1,13,C_GRAY);}
 char a[12],b[12];number(lo,a);number(hi,b);cv_text(12,199,&FONT_XS,a,C_GRAY);cv_text(228-text_w(&FONT_XS,b),199,&FONT_XS,b,C_GRAY);
}
static void overlay_range(unsigned k,int *lo,int *hi){
 if(page==39){*lo=k==2?20:k==3?500:0;*hi=k==2?1000:k==3?9500:100;return;}
 if(page==38){*lo=k==2?40:k==3?1:0;*hi=k==0?1:k==1?3:k==2?240:PW_DELAY_MAX_MS;return;}
 if(page==27&&k==0){*lo=10;*hi=90;return;}
 if(page==35){*lo=k==2?10:k==3?40:0;*hi=k==0?5:k==1?100:k==2?90:240;return;}
 if(page==34){*lo=0;*hi=k==0?7:k==1?127:100;return;}
 if(page==32){int clouds=st.reverb_model==PW_REVERB_CLOUDS;*lo=k==2&&clouds?400:0;*hi=k==0?1:k==1?PW_REVERB_MODELS-1:k==2&&clouds?30000:100;return;}
 if(page==33){*lo=k==2||(k==0&&st.reverb_model==PW_REVERB_CLOUDS)?1:0;*hi=k==2?500:100;return;}
 if(page==30||page==31){*lo=k==3?-100:(page==30?1:3);*hi=k==3?100:(page==30?2:4);return;}

 static const int lows[30][4]={
 {0,0,0,0},{0,0,0,0},{0,0,0,0},{-127,0,0,0},{0,0,-100,0},{-127,0,0,0},
 {-100,-100,-127,-1200},{-100,-100,-100,0},{-100,-100,-127,-100},{0,0,-100,-127},
 {-1200,0,0,-100},{0,0,0,0},{0,0,0,0},{-60,-100,0,-2},{0,0,0,0},{0,0,0,-100},
 {1,0,-100,-100},{0,1,0,0},{0,0,0,0},{0,0,-100,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0},{-40,0,-40,0},{0,1,40,0},{0,0,0,-24},{0,0,0,0}};
 static const int highs[30][4]={
 {127,100,PW_TABLE_BANKS-1,100},{0,0,100,0},{0,100,3,100},{127,0,100,3},{0,0,100,0},{127,0,100,3},
 {100,100,127,1200},{100,100,100,100},{100,100,127,100},{3,90,100,127},
 {1200,0,0,100},{127,127,0,0},{127,127,0,0},{60,100,48,2},{1,100,PW_TABLE_BANKS-1,100},{2,100,3,100},
 {CORE_MAX_VOICES,100,100,100},{2,PW_DELAY_MAX_MS,90,100},{1,100,3,100},{1,100,100,100},{0,0,0,0},{0,0,0,0},{2,0,0,100},{CORE_MAX_VOICES,CORE_MAX_VOICES,100,100},{0,0,0,0},{0,0,0,0},{24,1,0,100},{1,32,240,100},{PW_BLOOM_SCALES-1,11,4,24},{PW_PRESET_COUNT-1,100,PW_TABLE_BANKS-1,100}};
 *lo=lows[page][k];*hi=highs[page][k];
 if(page==17&&k==3&&st.delay_style){*lo= -40;*hi=0;}
 if(page==22&&(k==1||k==2))*hi=(int)(pw_env_ms(PW_ENV_RELEASE,127)+.5f);
 if((page==1||page==4)&&k!=2)*hi=(int)(pw_env_ms(k==0?PW_ENV_ATTACK:k==1?PW_ENV_DECAY:PW_ENV_RELEASE,127)+.5f);
 if((page==11||page==12)&&k>=2)*hi=(int)(pw_env_ms(k==2?PW_ENV_ATTACK:PW_ENV_DECAY,127)+.5f);
 if(page==10&&(k==1||k==2))*hi=(int)(pw_env_ms(k==1?PW_ENV_ATTACK:PW_ENV_DECAY,127)+.5f);
 if((page==2&&k==0)||((page==3||page==5)&&k==1)){*lo=(int)(pw_filter_hz(0)+.5f);*hi=(int)(pw_filter_hz(127)+.5f);}
}
static void overlay_wave(void){
 cv_rect(12,150,216,52,OV_PANEL_BG);
 for(int i=0;i<107;i++){int a=176-ui_wave_sample((uint32_t)i*255u/107u)*21/32768,b=176-ui_wave_sample((uint32_t)(i+1)*255u/107u)*21/32768;cv_line(13+i*2,a,15+i*2,b,OV_AMBER);}
}
static void overlay_focus(unsigned k){
 overlay_view *v=&views[k];overlay_text(12,49,&FONT_M,overlay_page_name(),C_GRAY);
 overlay_text(12,79,&FONT_M,v->label+3,C_WHITE);
 overlay_text(12,110,v->numeric?&FONT_L:&FONT_M,v->value,OV_AMBER);
 if((page==0||page==14)&&k==2){overlay_wave();return;}
 if((page==30||page==31)&&(k==1||k==2)){cv_text(12,172,&FONT_S,k==1?pw_matrix_source_rate(st.matrix_source[matrix_row]):"SELECT DESTINATION",C_GRAY);return;}
 if(v->numeric){int lo,hi;overlay_range(k,&lo,&hi);overlay_bar(v->value_num,lo,hi);if(page==18&&k==1&&!st.velocity_mode){cv_rect(12,198,216,17,C_BLACK);cv_text(12,199,&FONT_XS,"SET RESPONSE TO SOURCE TO APPLY",C_GRAY);}}
 else{
  static const char *hold[]={"OFF","ON"};
  static const char *triggers[]={"GATED","TRIGGERED","DRONE"};
  static const char *delay_modes[]={"OFF","STEREO","PING-PONG"};
  static const char *filters[]={"OFF","LOWPASS","BANDPASS","HIGHPASS"};
  static const char *phase[]={"KEEP","POLY","MONO","OFF"};
  static const char *chorus[]={"OFF","MODE 1","MODE 2"};
  static const char *stereo[]={"MONO","VOICE","PAIRS","PITCH"};
  static const char *engine[]={"LEGACY","AUTO"},*response[]={"LEGACY","SOURCE"},*curve[]={"LINEAR","FIXED","HARD","SOFT"};
  const char **choices=0;unsigned n=0,chosen=0;
  if(page==32&&k==0){choices=fx_modes;n=2;chosen=st.fx_reverb;}
  else if(page==32&&k==1){n=PW_REVERB_MODELS;chosen=st.reverb_model;}
  else if(page==35&&k==0){choices=ratchet_labels;n=6;chosen=st.bloom_ratchet;}
  else if(page==28&&k==0){choices=bloom_scales;n=PW_BLOOM_SCALES;chosen=st.bloom_scale;}
  else if(page==28&&k==1){choices=bloom_roots;n=12;chosen=st.bloom_root;}
  else if(page==26&&k==1){choices=response;n=2;chosen=st.delay_style;}
  else if(page==22&&k==0){choices=sustain_modes;n=3;chosen=st.sustain_mode;}
  else if(page==22&&k==3){choices=hold;n=2;chosen=st.sustain_hold;}
  else if(page==19&&k==0){choices=triggers;n=3;chosen=st.trigger_mode;}
  else if(page==17&&k==0){choices=delay_modes;n=3;chosen=st.delay_mode;}
  else if((page==2&&k==2)||((page==3||page==5)&&k==3)){choices=filters;n=4;chosen=st.filter.mode;}
  else if(page==9&&k==0){choices=phase;n=4;chosen=st.lfo_phase_mode;}
  else if(page==15&&k==0){choices=chorus;n=3;chosen=st.chorus_mode;}
  else if(page==15&&k==2){choices=stereo;n=4;chosen=st.stereo_mode;}
  else if(page==14&&k==0){choices=engine;n=2;chosen=st.wave_quality;}
  else if(page==18&&k==0){choices=response;n=2;chosen=st.velocity_mode;}
  else if(page==18&&k==2){choices=curve;n=4;chosen=st.velocity_curve;}
  if(page==18&&k==2)velocity_graph(12,146,216,30,OV_AMBER);
  if(n>4){overlay_bar((int)chosen+1,1,(int)n);return;}
  for(unsigned i=0;i<n;i++){int w=216/(int)n,x=12+(int)i*w;cv_rect(x,182,w-3,25,i==chosen?OV_AMBER:OV_PANEL_BG);const felucca_font_t *f=text_w(&FONT_XS,choices[i])<w-7?&FONT_XS:&FONT_T;cv_text(x+3,189,f,choices[i],i==chosen?C_BLACK:C_GRAY);}
  if(!n)cv_text(12,190,&FONT_XS,"CURRENT PANEL VALUE",C_GRAY);
 }
}
static void overlay_small_value(int x,int y,int w,const char *label,const char *value){
 cv_rect(x,y,w,47,OV_PANEL_BG);cv_text(x+5,y+5,&FONT_T,label,C_GRAY);
 const felucca_font_t *f=text_w(&FONT_M,value)<w-8?&FONT_M:&FONT_B;cv_text(x+5,y+19,f,value,C_WHITE);
}
static void overlay_envelope(void){
 const pw_env_params *p=page==4?&st.env2:&st.env;
 overlay_text(12,51,&FONT_OH,page==4?"ENV2 ADSR":"AMP ADSR",C_WHITE);
 int a=28+p->attack*40/127,d=22+p->decay*35/127,r=20+p->release*30/127,s=page==4?135-p->sustain*28/127:163-p->sustain*55/127;
 int zero=page==4?135:165;cv_line(12,zero,228,zero,C_LINE);cv_line(12,zero,12+a,102,OV_AMBER);cv_line(12+a,102,12+a+d,s,OV_AMBER);if(st.trigger_mode==1)cv_line(12+a+d,s,228,zero,OV_AMBER);else{cv_line(12+a+d,s,228-r,s,OV_AMBER);cv_line(228-r,s,228,zero,OV_AMBER);}
 for(unsigned i=0;i<4;i++)overlay_small_value(12+(int)i*55,173,51,i==0?"A ms":i==1?"D ms":i==2?"S %":"R ms",views[i].value);
}
static void draw_overlay(void){
 cv_begin(240,240,C_BLACK);
 if(overlay.kind==OV_PANEL){
  /* Page buttons must show ONLY the final page title: no stale page frame,
     button sequence, card values, or status overlay. */
  const char *name=overlay_page_name();
  const felucca_font_t *font=&FONT_OH;
  if(text_w(font,name)>224)font=&FONT_L;
  if(text_w(font,name)>224)font=&FONT_M;
  if(text_w(font,name)>224)font=&FONT_B;
  cv_text((240-text_w(font,name))/2,99,font,name,C_WHITE);
  return;
 }
 overlay_trigger();
 if(overlay.kind>=OV_K1&&overlay.kind<=OV_K4&&!(page==29&&overlay.kind==OV_K1))overlay_focus(overlay.kind-OV_K1);
 else if(overlay.kind==OV_PRESET||(page==29&&overlay.kind==OV_K1)){
  char index[12];number(preset_index,index);cv_text(12,49,&FONT_M,"SOUND PRESET",C_GRAY);
  overlay_text(12,88,&FONT_L,pw_preset_name(preset_index),OV_AMBER);
  cv_text(12,144,&FONT_XS,pw_preset_category(preset_index),C_WHITE);
  cv_text(12,177,&FONT_XS,preset_index>=28?"VIRUS / BASS + PLUCK":preset_index>10?"ORIGINAL FM1 / SYNTH BANKS":preset_index?"TRK-EL-SON / FM1 ADAPTATION":"INITIAL SYNTH SETTINGS",C_GRAY);
  cv_text(12,201,&FONT_XS,"PRESET",C_GRAY);cv_text(59,201,&FONT_XS,index,C_WHITE);cv_text(85,201,&FONT_XS,"/",C_GRAY);char total[12];number(PW_PRESET_COUNT-1,total);cv_text(94,201,&FONT_XS,total,C_GRAY);
 }
 else if(overlay.kind==OV_MASTER){char value[12];int percent=(int)((plat_master()*100u+2048u)/4096u);number(percent,value);cv_text(12,51,&FONT_M,"OUTPUT",C_GRAY);cv_text(12,80,&FONT_S,"MASTER VOLUME %",C_WHITE);overlay_text(12,110,&FONT_L,value,OV_AMBER);overlay_bar(percent,0,100);}
 else if(page==23&&(overlay.buttons&(1u<<B_REC))){
  overlay_text(12,51,&FONT_M,"VOICE RENDERING",C_GRAY);
  overlay_text(12,94,&FONT_OH,core_parallel_enabled()?"DUAL CORE":"SINGLE CORE",OV_AMBER);
  cv_text(12,155,&FONT_XS,core_worker_fault()?"WORKER FAULT / REBOOT":core_parallel_enabled()?"CPU0: ODD / CPU1: EVEN":"CPU0: ALL ACTIVE VOICES",C_WHITE);
  cv_text(12,184,&FONT_XS,"REC TO COMPARE CPU LOAD",C_GRAY);
 }
 else if(page==1||page==4)overlay_envelope();
 else if((page==3||page==5)&&(overlay.buttons&(overlay.buttons-1u)))overlay_focus(0);
 else if(overlay.kind==OV_PANEL&&(overlay.buttons&((1u<<B_OCTDN)|(1u<<B_OCTUP)))){char value[12];number(st.panel_octave,value);cv_text(12,51,&FONT_M,"KEYS / SEQUENCER",C_GRAY);cv_text(12,80,&FONT_S,"OCTAVE / NEXT NOTES",C_WHITE);overlay_text(12,110,&FONT_L,value,OV_AMBER);overlay_bar(st.panel_octave,-2,2);}
 else{overlay_text(12,51,&FONT_OH,overlay_page_name(),C_WHITE);for(unsigned i=0;i<4;i++)overlay_small_value(12+(int)(i&1)*110,103+(int)(i>>1)*56,106,views[i].label,views[i].value);}
 overlay_footer();
}
