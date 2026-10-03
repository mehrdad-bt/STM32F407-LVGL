#ifndef EEZ_LVGL_UI_ACTIONS_H
#define EEZ_LVGL_UI_ACTIONS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void action_calibration(lv_event_t *e);
void action_error_hide(lv_event_t *e);

void action_go_to_temp_page(lv_event_t *e);
void action_back_to_main(lv_event_t *e);

void action_go_to_oscope_page(lv_event_t *e);
void action_exit_from_oscope_page(lv_event_t *e);

void action_time_increase(lv_event_t *e);
void action_time_decrease(lv_event_t *e);

void action_volt_increase(lv_event_t *e);
void action_volt_decrease(lv_event_t *e);

void action_trig_increase(lv_event_t *e);
void action_trig_decrease(lv_event_t *e);

void action_stop_run_oscope(lv_event_t *e);

void event_handler_cb_main_msg_hide_switch(lv_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_LVGL_UI_ACTIONS_H */
