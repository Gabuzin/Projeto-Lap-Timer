#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void action_config_page(lv_event_t * e);
extern void action_home(lv_event_t * e);
extern void action_wifi_switch(lv_event_t *e);
#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/