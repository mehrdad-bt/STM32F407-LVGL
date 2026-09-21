#include "actions.h"
#include "touchCalibration.h"
#include "MessageBox.h"


void action_calibration(lv_event_t *e)
{
    if (e == NULL)
    {
        return;
    }

    if (lv_event_get_code(e) != LV_EVENT_RELEASED)
    {
        return;
    }

    touch_calibration_start();
}


void action_error_hide(lv_event_t *e)
{
    if (e == NULL)
    {
        return;
    }

    lv_obj_t *obj = lv_event_get_target(e);

    if (obj == NULL)
    {
        return;
    }

    /*
     * Switch is managed by LVGL itself.
     *
     * CHECKED     = ON
     * NOT CHECKED = OFF
     */

    if (lv_obj_has_state(obj, LV_STATE_CHECKED))
    {
        /* Switch ON -> hide MessageBox */
        MessageBox_SetHidden(true);
    }
    else
    {
        /* Switch OFF -> show MessageBox */
        MessageBox_SetHidden(false);
    }
}


/* -------------------------------------------------------------------------- */
/* EEZ generated callback for msg_hide_switch                                 */
/* -------------------------------------------------------------------------- */

void event_handler_cb_main_msg_hide_switch(lv_event_t *e)
{
    if (e == NULL)
    {
        return;
    }

    /*
     * We only care about the actual Switch state change.
     */
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
    {
        return;
    }

    action_error_hide(e);
}
