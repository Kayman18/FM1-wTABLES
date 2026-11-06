#pragma once
#include <stdint.h>
#include "plat.h"
#define APP_NAME "wTABLES"
#define APP_SPLASH_RGB RGB(34,42,52)
#define APP_SPLASH
#define APP_BLOCK 256u
#define APP_CANVAS_PX (240u*240u)
enum {OBJ_SETTINGS,OBJ_PANEL,OBJ_NOBJ};
#define APP_STORE_MAP {0xFC000u,0xDC000u}
void app_init(void);
void app_splash(void);
void app_render(int32_t*,uint32_t,uint32_t);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char*,const char*);

#define APP_UPDATE_STATUS
void app_update_status(unsigned state,uint32_t elapsed_ms); /* 0 cancel, 1 hold, 2 handoff */

#define APP_HOST_EXPECT
int app_expect(const char*,int*);

#ifdef PLAT_HOST
#define APP_HOST_REBOOT
void app_host_reboot(void);
#endif
