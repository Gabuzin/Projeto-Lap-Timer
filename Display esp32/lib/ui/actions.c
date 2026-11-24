
#include "actions.h"

extern int setLapTimerWebEnabled(int enabled);

void action_config_page(lv_event_t *e)
{
    (void)e;
    loadScreen(SCREEN_ID_CONFIG);
}

void action_home(lv_event_t *e) {
    (void)e;
    loadScreen(SCREEN_ID_HOME);
}

void action_wifi_switch(lv_event_t *e) {
    lv_obj_t *sw = lv_event_get_target(e);

    if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        if (!setLapTimerWebEnabled(1)) {
            lv_obj_clear_state(sw, LV_STATE_CHECKED);
            lv_label_set_text(objects.gps_7, "Falha no Wi-Fi");
            return;
        }
        lv_label_set_text(objects.gps_7, "Wi-Fi: ligado");
    } else {
        setLapTimerWebEnabled(0);
        lv_label_set_text(objects.gps_7, "Wi-Fi: desligado");
    }
}