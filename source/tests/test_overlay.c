#include <assert.h>
#include <stdio.h>
#include "../fm1/app/panel_overlay.h"
int main(void){
 panel_overlay o={0};assert(!overlay_master(&o,2048,0,0));assert(!o.active);
 for(unsigned i=2049;i<2064;i++)assert(!overlay_master(&o,i,0,i));
 assert(overlay_master(&o,2064,0,100));assert(o.kind==OV_MASTER);
 assert(overlay_visible(&o,0,799));assert(!overlay_visible(&o,0,800));
 overlay_event(&o,OV_PANEL,0x3fff,900);assert(o.buttons==0x3fff);assert(overlay_visible(&o,0x3fff,100000));
 overlay_event(&o,OV_K2,3,1000);assert(o.kind==OV_K2&&o.buttons==3);
 o.at=1010;assert(overlay_visible(&o,0,1709));assert(!overlay_visible(&o,0,1710));
 overlay_event(&o,OV_PRESET,0,0xffffff00u);assert(overlay_visible(&o,0,0x1bbu));assert(!overlay_visible(&o,0,0x1bcu));
 assert(overlay_master(&o,0,0,2000));assert(overlay_master(&o,4096,0,2001));
 for(unsigned mask=1;mask<16384;mask++){overlay_event(&o,OV_PANEL,mask,mask);assert(o.buttons==mask&&overlay_visible(&o,mask,mask+10000));}
 puts("PASS: all 16383 button masks, latest control, held state, exact 700ms timeout, clock wrap, master deadband and endpoints.");
}
