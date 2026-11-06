#pragma once
#include "plat.h"
/* Button indicators use the full-bright path, never the dim/glow path.
 * During a press, the held buttons are dark; otherwise the selected control
 * remains dark. Keyboard LEDs keep their existing held-key behavior. */
static inline uint32_t panel_button_mask(uint32_t held,unsigned selected){
 const uint32_t all=(1u<<NB)-1u;
 uint32_t dark=held&all;
 if(!dark&&selected<NB)dark=1u<<selected;
 return all&~dark;
}
/* PLAY follows transport state rather than page-selection/held-button state. */
static inline uint32_t panel_transport_mask(uint32_t held,unsigned selected,unsigned playing){
 uint32_t mask=panel_button_mask(held,selected),play=1u<<B_PLAY;
 return playing?mask&~play:mask|play;
}
static inline uint32_t panel_hardware_mask(uint32_t logical){
 uint32_t play=1u<<B_PLAY,unused=1u<<B_ARP;
 return (logical&~(play|unused))|((logical&play)?LED_PLAY_GREEN:0u);
}
