#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_calibration(lv_event_t * e);
extern void action_error_hide(lv_event_t * e);
extern void action_go_to_temp_page(lv_event_t * e);
extern void action_back_to_main(lv_event_t * e);
extern void action_go_to_oscope_page(lv_event_t * e);
extern void action_exit_from_oscope_page(lv_event_t * e);
extern void action_time_increase(lv_event_t * e);
extern void action_time_decrease(lv_event_t * e);
extern void action_volt_increase(lv_event_t * e);
extern void action_volt_decrease(lv_event_t * e);
extern void action_trig_increase(lv_event_t * e);
extern void action_trig_decrease(lv_event_t * e);
extern void action_stop_run_oscope(lv_event_t * e);
extern void action_sweep_increase(lv_event_t * e);
extern void action_sweep_decrease(lv_event_t * e);
extern void action_auto_button(lv_event_t * e);

/* Used by screens.c (msg_hide_switch), defined in actions.c */
extern void event_handler_cb_main_msg_hide_switch(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/
