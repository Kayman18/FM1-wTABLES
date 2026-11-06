/* UI-only state: no allocations, audio calls or framebuffer ownership. */
#pragma once
#include <stdint.h>
enum {OV_PANEL,OV_K1,OV_K2,OV_K3,OV_K4,OV_PRESET,OV_MASTER,OV_SELECT,OV_ALGO};
typedef struct {uint32_t at,buttons,master;uint8_t active,kind,master_ready;} panel_overlay;
static void overlay_event(panel_overlay *o,unsigned kind,uint32_t buttons,uint32_t now){o->kind=(uint8_t)kind;o->buttons=buttons;o->at=now;o->active=1;}
static unsigned overlay_timeout(const panel_overlay *o){return (o->kind==OV_K1||o->kind==OV_K2)?2800u:(o->kind==OV_K3||o->kind==OV_K4)?1400u:700u;}
static int overlay_visible(const panel_overlay *o,uint32_t held,uint32_t now){return o->active&&(held||(uint32_t)(now-o->at)<overlay_timeout(o));}
static int overlay_master(panel_overlay *o,uint32_t raw,uint32_t held,uint32_t now){
 if(raw>4096)raw=4096;
 if(!o->master_ready){o->master=raw;o->master_ready=1;return 0;}
 unsigned delta=raw>o->master?raw-o->master:o->master-raw;
 if(delta<16&&!(raw==0&&o->master!=0)&&!(raw==4096&&o->master!=4096))return 0;
 o->master=raw;overlay_event(o,OV_MASTER,held,now);return 1;
}
