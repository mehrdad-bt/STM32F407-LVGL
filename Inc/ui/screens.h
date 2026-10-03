#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_TEMP = 2,
    SCREEN_ID_OSILLOSCOP = 3,
    _SCREEN_ID_LAST = 3
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *temp;
    lv_obj_t *osilloscop;
    lv_obj_t *calibration_button;
    lv_obj_t *calibration_text;
    lv_obj_t *oscope_button_main_page;
    lv_obj_t *oscope_button_main_page_text;
    lv_obj_t *temp_button;
    lv_obj_t *temp_button_text;
    lv_obj_t *message_box;
    lv_obj_t *message_box_text;
    lv_obj_t *information_indicator;
    lv_obj_t *msg_hide_switch;
    lv_obj_t *error_hide_text;
    lv_obj_t *temp_exit_button;
    lv_obj_t *temp_exit_button_text;
    lv_obj_t *temp_text_static;
    lv_obj_t *humid_text_static;
    lv_obj_t *temp_text_dynamic;
    lv_obj_t *humid_text_dynamic;
    lv_obj_t *temp_humid_header_text;
    lv_obj_t *chart_oscope;
    lv_obj_t *oscope_page_exit_button;
    lv_obj_t *oscope_page_exit_button_text;
    lv_obj_t *time_plus_btn;
    lv_obj_t *time_minus_btn;
    lv_obj_t *volt_plus_btn;
    lv_obj_t *volt_minus_btn;
    lv_obj_t *trig_plus_btn;
    lv_obj_t *trig_minus_btn;
    lv_obj_t *stop_run_btn;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void create_screen_temp();
void tick_screen_temp();

void create_screen_osilloscop();
void tick_screen_osilloscop();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/
