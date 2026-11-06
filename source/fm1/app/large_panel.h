/* Normal-page layout; full-screen control overlays remain unchanged. */
#define PANEL_AMBER RGB(255,182,0)
static void panel_center(int x,int y,int width,const felucca_font_t *f,const char *v,uint16_t color){
 cv_text(x+(width-text_w(f,v))/2,y,f,v,color);
}
/* Large, readable WHITE tile labels are more important than amber values.
   Split long labels over two lines rather than shrinking every label to 10 px. */
static void panel_label(int x,int y,const char *s){
 const felucca_font_t *f=&FONT_B;
 if(text_w(f,s)<=100){panel_center(x,y,108,f,s,C_WHITE);return;}
 unsigned n=0;while(s[n]&&n<31)n++;
 unsigned split=0;int best=9999;
 for(unsigned i=1;i<n;i++)if(s[i]==' '){
  char one[32],two[32];unsigned k;
  for(k=0;k<i;k++)one[k]=s[k];
  one[k]=0;
  for(k=0;i+1+k<n;k++)two[k]=s[i+1+k];
  two[k]=0;
  int w1=text_w(f,one),w2=text_w(f,two);
  int score=(w1>w2?w1:w2)+((w1>w2?w1-w2:w2-w1)/8);
  if(w1<=102&&w2<=102&&score<best){best=score;split=i;}
 }
 if(split){
  char one[32],two[32];unsigned k;
  for(k=0;k<split;k++)one[k]=s[k];
  one[k]=0;
  for(k=0;split+1+k<n;k++)two[k]=s[split+1+k];
  two[k]=0;
  panel_center(x,y,108,f,one,C_WHITE);
  panel_center(x,y+17,108,f,two,C_WHITE);return;
 }
 f=&FONT_XS;if(text_w(f,s)>100)f=&FONT_T;
 panel_center(x,y+7,108,f,s,C_WHITE);
}
static void panel_value(int x,int y,const char *value){
 const felucca_font_t *f=&FONT_L; /* 31 px instead of former 48 px amber. */
 if(text_w(f,value)>98)f=&FONT_M;
 if(text_w(f,value)>98)f=&FONT_B;
 if(text_w(f,value)>98)f=&FONT_XS;
 if(text_w(f,value)<=98){panel_center(x,y,108,f,value,PANEL_AMBER);return;}
 /* Last resort: fit long value without running into adjacent tile. */
 panel_center(x,y,108,&FONT_T,value,PANEL_AMBER);
}
static void panel_status(void){
 cv_rect(0,0,240,25,C_BLACK);cv_line(4,24,235,24,PANEL_AMBER);
 cv_text(6,5,&FONT_XS,st.playing?"PLAY":"STOP",PANEL_AMBER);
 char n[16];number(shown_voices,n);unsigned i=0;while(n[i])i++;n[i++]='/';number(st.voice_limit,n+i);
 cv_text(91,5,&FONT_XS,n,C_WHITE);
 cv_rect(161,6,24,12,C_WHITE);cv_rect(162,7,22,10,C_BLACK);cv_rect(185,9,2,6,C_WHITE);
 if(battery_pct>=0&&battery_pct<=100)cv_rect(164,9,battery_pct*18/100,6,PANEL_AMBER);
 if(battery_pct==101){n[0]='U';n[1]='S';n[2]='B';n[3]=0;}
 else if(battery_pct<0){n[0]='-';n[1]='-';n[2]=0;}
 else{number(battery_pct,n);i=0;while(n[i])i++;n[i++]='%';n[i]=0;}
 cv_text(190,5,&FONT_XS,n,C_WHITE);
}
static void draw_large_panel(void){
 if(page==30||page==31){
  cv_begin(240,240,C_BLACK);panel_status();panel_center(0,30,240,&FONT_B,page==30?"LFO MATRIX 1/2":"LFO MATRIX 2/2",C_WHITE);
  unsigned first=page==30?0:2;
  for(unsigned i=0;i<2;i++)for(unsigned col=0;col<2;col++){
   unsigned row=first+i;int x=col?124:8,y=i?141:52;
   cv_rect(x,y,108,83,row==matrix_row?PANEL_AMBER:C_GRAY);cv_rect(x+1,y+1,106,81,RGB(24,24,27));
   panel_center(x,y+4,108,&FONT_B,col?"DESTINATION":"LFO OUTPUT",C_WHITE);
   const char *v=col?pw_matrix_dest_name(st.matrix_dest[row]):pw_matrix_source_name(st.matrix_source[row]);
   const felucca_font_t *f=&FONT_B;if(text_w(f,v)>98)f=&FONT_XS;if(text_w(f,v)>98)f=&FONT_T;
   panel_center(x,y+29,108,f,v,PANEL_AMBER);
   char amount[12];number(st.matrix_amount[row],amount);unsigned n=0;while(amount[n])n++;amount[n++]='%';amount[n]=0;
   panel_center(x,y+60,108,&FONT_XS,col?amount:pw_matrix_source_rate(st.matrix_source[row]),C_WHITE);
  }return;
 }
 cv_begin(240,240,C_BLACK);panel_status();
 const char *title=overlay_page_name();if(page==0)title="WAVE";
 const felucca_font_t *tf=&FONT_B;if(text_w(tf,title)>224)tf=&FONT_XS;
 panel_center(4,30,232,tf,title,C_WHITE);
 for(unsigned k=0;k<4;k++){
  int x=k%2?124:8,y=k<2?52:141;
  cv_rect(x,y,108,83,PANEL_AMBER);cv_rect(x+1,y+1,106,81,RGB(24,24,27));
  const char *label=views[k].label?views[k].label:"K?";
  panel_label(x,y+4,label);
  panel_value(x,y+43,views[k].value);
 }
 if(page==29){cv_rect(0,25,240,25,C_BLACK);const char *name=pw_preset_name(preset_index);tf=&FONT_B;if(text_w(tf,name)>228)tf=&FONT_XS;panel_center(4,30,232,tf,name,C_WHITE);}
 if((page==17||page==38)&&st.delay_sync){
  unsigned effective;unsigned ms=(unsigned)(pw_delay_sync_ms(st.sequencer_bpm,st.delay_division,&effective)+.5f);
  char text[32]="SEQ ",n[12];unsigned i=4;const char *d=pw_delay_division_name(effective);while(*d)text[i++]=*d++;text[i++]=' ';number(ms,n);unsigned j=0;while(n[j])text[i++]=n[j++];text[i++]='m';text[i++]='s';text[i]=0;
  panel_center(0,226,240,&FONT_T,text,PANEL_AMBER);
 }
}
