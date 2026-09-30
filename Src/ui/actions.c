#include "actions.h"

#include "touchCalibration.h"
#include "MessageBox.h"
#include "TempPage.h"
#include "OscopePage.h"

#include "lvgl.h"

#include "ui/ui.h"
#include "ui/screens.h"


/* -------------------------------------------------------------------------- */
/* Calibration                                                                */
/* -------------------------------------------------------------------------- */

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


/* -------------------------------------------------------------------------- */
/* Error Hide / Show                                                          */
/* -------------------------------------------------------------------------- */

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

    if (lv_obj_has_state(obj, LV_STATE_CHECKED))
    {
        MessageBox_SetHidden(true);
    }
    else
    {
        MessageBox_SetHidden(false);
    }
}


/* -------------------------------------------------------------------------- */
/* Main -> Temp                                                               */
/* -------------------------------------------------------------------------- */

void action_go_to_temp_page(lv_event_t *e)
{
    if (e == NULL)
    {
        return;
    }

    if (lv_event_get_code(e) != LV_EVENT_RELEASED)
    {
        return;
    }

    lv_scr_load(objects.temp);

    TempPage_OnEnter();
}


/* -------------------------------------------------------------------------- */
/* Temp -> Main                                                               */
/* -------------------------------------------------------------------------- */

void action_back_to_main(lv_event_t *e)
{
    if (e == NULL)
    {
        return;
    }

    if (lv_event_get_code(e) != LV_EVENT_RELEASED)
    {
        return;
    }

    lv_scr_load(objects.main);
}


/* -------------------------------------------------------------------------- */
/* Main -> Oscilloscope                                                       */
/* -------------------------------------------------------------------------- */

void action_go_to_oscope_page(lv_event_t *e)
{
    if (e == NULL)
    {
        return;
    }

    if (lv_event_get_code(e) != LV_EVENT_RELEASED)
    {
        return;
    }

    lv_scr_load(objects.osilloscop);

    OscopePage_OnEnter();
}


/* -------------------------------------------------------------------------- */
/* Oscilloscope -> Main                                                       */
/* -------------------------------------------------------------------------- */

void action_exit_from_oscope_page(lv_event_t *e)
{
    if (e == NULL)
    {
        return;
    }

    if (lv_event_get_code(e) != LV_EVENT_RELEASED)
    {
        return;
    }

    OscopePage_OnExit();

    lv_scr_load(objects.main);
}


/* -------------------------------------------------------------------------- */
/* Oscilloscope -> Increase Speed                                             */
/* -------------------------------------------------------------------------- */

void action_increase_btn(lv_event_t *e)
{
    if (e == NULL)
    {
        return;
    }

    if (lv_event_get_code(e) != LV_EVENT_RELEASED)
    {
        return;
    }

    OscopePage_IncreaseSpeed();
}


/* -------------------------------------------------------------------------- */
/* Oscilloscope -> Decrease Speed                                             */
/* -------------------------------------------------------------------------- */

void action_decrease_btn(lv_event_t *e)
{
    if (e == NULL)
    {
        return;
    }

    if (lv_event_get_code(e) != LV_EVENT_RELEASED)
    {
        return;
    }

    OscopePage_DecreaseSpeed();
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

    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
    {
        return;
    }

    action_error_hide(e);
}
