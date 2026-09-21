#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void action_calibration(lv_event_t *e);
void action_error_hide(lv_event_t *e);

void event_handler_cb_main_msg_hide_switch(lv_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_LVGL_UI_EVENTS_H */
