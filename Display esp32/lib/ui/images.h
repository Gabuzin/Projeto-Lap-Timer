#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_satelite;
extern const lv_img_dsc_t img_config;
extern const lv_img_dsc_t img_bateria_cheia;
extern const lv_img_dsc_t img_bateria_carregando;
extern const lv_img_dsc_t img_bateria_vazia;
extern const lv_img_dsc_t img_bateria_1;
extern const lv_img_dsc_t img_bateria_2;
extern const lv_img_dsc_t img_bateria_3;
extern const lv_img_dsc_t img_bateria_4;
extern const lv_img_dsc_t img_bateria_5;
extern const lv_img_dsc_t img_bateria_6;
extern const lv_img_dsc_t img_home;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[12];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/