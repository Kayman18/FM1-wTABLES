/* Driven by the platform's actual hold timer, including early cancellation. */
static void draw_update_overlay(void){
 cv_begin(240,240,C_BLACK);cv_rect(0,0,240,40,OV_PANEL_BG);
 cv_text(12,12,&FONT_S,"OCT- + OCT+",OV_AMBER);
 cv_text(12,53,&FONT_OH,"BOOTLOADER",C_WHITE);
 if(update_state==1){
  unsigned elapsed=update_elapsed>5000?5000:update_elapsed;
  unsigned left=(5000-elapsed+999)/1000;char value[12];number((int)left,value);
  cv_text(12,98,&FONT_OV,value,OV_AMBER);cv_text(76,135,&FONT_S,"SECONDS",C_GRAY);
  cv_rect(12,184,216,7,OV_PANEL_BG);cv_rect(12,184,(int)(elapsed*216/5000),7,OV_AMBER);
  cv_text(12,200,&FONT_XS,"KEEP BOTH OCTAVE BUTTONS HELD",C_WHITE);
  cv_text(12,224,&FONT_XS,"RELEASE EITHER BUTTON TO CANCEL",C_GRAY);
 }else if(update_state==2){
  cv_text(12,107,&FONT_L,"CANCELLED",OV_AMBER);
  cv_text(12,159,&FONT_S,"UPDATE MODE NOT ENTERED",C_WHITE);
  cv_text(12,207,&FONT_XS,"RETURNING TO CURRENT PANEL",C_GRAY);
 }else{
  cv_text(12,107,&FONT_L,"STARTING...",OV_AMBER);
  cv_text(12,159,&FONT_S,"FIRMWARE UPDATE MODE",C_WHITE);
  cv_text(12,207,&FONT_XS,"RELEASE BOTH OCTAVE BUTTONS",C_GRAY);
 }
}
