#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_HOME = 1,
    SCREEN_ID_CONFIG = 2,
    SCREEN_ID_PISTA = 3,
    _SCREEN_ID_LAST = 3
};

typedef struct _objects_t {
    lv_obj_t *home;
    lv_obj_t *config;
    lv_obj_t *pista;
    lv_obj_t *cabecalho;
    lv_obj_t *n_sat;
    lv_obj_t *gps;
    lv_obj_t *obj0;
    lv_obj_t *sat_icon;
    lv_obj_t *bat;
    lv_obj_t *n_sat_1;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *texto_gap;
    lv_obj_t *gap;
    lv_obj_t *obj3;
    lv_obj_t *texto_melhor_volta;
    lv_obj_t *melhor_volta;
    lv_obj_t *km_h;
    lv_obj_t *velocidade;
    lv_obj_t *angulo;
    lv_obj_t *obj4;
    lv_obj_t *tempo;
    lv_obj_t *volta_atual;
    lv_obj_t *cabecalho_2;
    lv_obj_t *n_sat_2;
    lv_obj_t *gps_4;
    lv_obj_t *obj5;
    lv_obj_t *sat_icon_1;
    lv_obj_t *bat_1;
    lv_obj_t *n_sat_3;
    lv_obj_t *gps_6;
    lv_obj_t *gps_7;
    lv_obj_t *cabecalho_3;
    lv_obj_t *n_sat_4;
    lv_obj_t *gps_5;
    lv_obj_t *obj6;
    lv_obj_t *sat_icon_2;
    lv_obj_t *bat_2;
    lv_obj_t *n_sat_5;
    lv_obj_t *obj7;
    lv_obj_t *roll;
} objects_t;

extern objects_t objects;

void create_screen_home();
void tick_screen_home();

void create_screen_config();
void tick_screen_config();

void create_screen_pista();
void tick_screen_pista();

void create_user_widget_cabe_alho(lv_obj_t *parent_obj, int startWidgetIndex);
void tick_user_widget_cabe_alho(int startWidgetIndex);

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/