#include <assert.h>
#include <stdio.h>
#include "../fm1/app/panel_overlay.h"
int main(void){
 panel_overlay o={0};overlay_event(&o,OV_PANEL,0,100);
 assert(overlay_visible(&o,0,799));assert(!overlay_visible(&o,0,800));
 overlay_event(&o,OV_K1,0,100);
 assert(overlay_visible(&o,0,2899));assert(!overlay_visible(&o,0,2900));
 overlay_event(&o,OV_K2,0,100);
 assert(overlay_visible(&o,0,2899));assert(!overlay_visible(&o,0,2900));
 overlay_event(&o,OV_K3,0,100);assert(overlay_visible(&o,0,1499));assert(!overlay_visible(&o,0,1500));
 overlay_event(&o,OV_K4,0,100);assert(overlay_visible(&o,0,1499));assert(!overlay_visible(&o,0,1500));
 overlay_event(&o,OV_K1,0,100);assert(overlay_visible(&o,1,10000));
 puts("PASS TEST62: K1/K2 2800ms, K3/K4 1400ms, buttons 700ms, held overlays persistent");
}
