#include "actions.h"

#include "touchCalibration.h"
#include "MessageBox.h"
#include "TempPage.h"
#include "OscopePage.h"

#include "lvgl.h"

#include "ui/ui.h"
#include "ui/screens.h"


/* ============================================================
 * HELPER
 * ============================================================ */

static bool is_oscilloscope_button_event(
    lv_event_t *e
)
{
    if (e == NULL)
    {
        return false;
    }

    lv_event_code_t code =
        lv_event_get_code(e);

    if (
        code == LV_EVENT_PRESSED ||
        code == LV_EVENT_RELEASED
    )
    {
        return true;
    }

    return false;
}


/* ============================================================
 * CALIBRATION
 * ============================================================ */

void action_calibration(
    lv_event_t *e
)
{
    if (
        e == NULL ||
        lv_event_get_code(e) !=
        LV_EVENT_RELEASED
    )
    {
        return;
    }

    touch_calibration_start();
}


/* ============================================================
 * ERROR HIDE
 * ============================================================ */

void action_error_hide(
    lv_event_t *e
)
{
    lv_obj_t *obj;

    if (e == NULL)
    {
        return;
    }

    obj =
        lv_event_get_target(e);

    if (obj == NULL)
    {
        return;
    }

    if (
        lv_obj_has_state(
            obj,
            LV_STATE_CHECKED
        )
    )
    {
        MessageBox_SetHidden(
            true
        );
    }
    else
    {
        MessageBox_SetHidden(
            false
        );
    }
}


/* ============================================================
 * TEMPERATURE PAGE
 * ============================================================ */

void action_go_to_temp_page(
    lv_event_t *e
)
{
    if (
        e == NULL ||
        lv_event_get_code(e) !=
        LV_EVENT_RELEASED
    )
    {
        return;
    }

    lv_scr_load(
        objects.temp
    );

    TempPage_OnEnter();
}


/* ============================================================
 * BACK TO MAIN
 * ============================================================ */

void action_back_to_main(
    lv_event_t *e
)
{
    if (
        e == NULL ||
        lv_event_get_code(e) !=
        LV_EVENT_RELEASED
    )
    {
        return;
    }

    lv_scr_load(
        objects.main
    );
}


/* ============================================================
 * GO TO OSCILLOSCOPE
 * ============================================================ */

void action_go_to_oscope_page(
    lv_event_t *e
)
{
    if (
        e == NULL ||
        lv_event_get_code(e) !=
        LV_EVENT_RELEASED
    )
    {
        return;
    }

    lv_scr_load(
        objects.osilloscop
    );

    OscopePage_OnEnter();
}


/* ============================================================
 * EXIT OSCILLOSCOPE
 * ============================================================ */

void action_exit_from_oscope_page(
    lv_event_t *e
)
{
    if (
        e == NULL ||
        lv_event_get_code(e) !=
        LV_EVENT_RELEASED
    )
    {
        return;
    }

    OscopePage_OnExit();

    lv_scr_load(
        objects.main
    );
}


/* ============================================================
 * TIME
 * ============================================================ */

void action_time_increase(
    lv_event_t *e
)
{
    if (
        !is_oscilloscope_button_event(e)
    )
    {
        return;
    }

    OscopePage_TimeIncrease();
}


void action_time_decrease(
    lv_event_t *e
)
{
    if (
        !is_oscilloscope_button_event(e)
    )
    {
        return;
    }

    OscopePage_TimeDecrease();
}


/* ============================================================
 * VOLT
 * ============================================================ */

void action_volt_increase(
    lv_event_t *e
)
{
    if (
        !is_oscilloscope_button_event(e)
    )
    {
        return;
    }

    OscopePage_VoltIncrease();
}


void action_volt_decrease(
    lv_event_t *e
)
{
    if (
        !is_oscilloscope_button_event(e)
    )
    {
        return;
    }

    OscopePage_VoltDecrease();
}


/* ============================================================
 * TRIGGER
 * ============================================================ */

void action_trig_increase(
    lv_event_t *e
)
{
    if (
        !is_oscilloscope_button_event(e)
    )
    {
        return;
    }

    OscopePage_TriggerIncrease();
}


void action_trig_decrease(
    lv_event_t *e
)
{
    if (
        !is_oscilloscope_button_event(e)
    )
    {
        return;
    }

    OscopePage_TriggerDecrease();
}


/* ============================================================
 * STOP / RUN
 * ============================================================ */

void action_stop_run_oscope(
    lv_event_t *e
)
{
    if (
        !is_oscilloscope_button_event(e)
    )
    {
        return;
    }

    OscopePage_ToggleRunStop();
}


/* ============================================================
 * SWEEP INCREASE
 * ============================================================ */

void action_sweep_increase(
    lv_event_t *e
)
{
    if (
        !is_oscilloscope_button_event(e)
    )
    {
        return;
    }

    OscopePage_SweepIncrease();
}


/* ============================================================
 * SWEEP DECREASE
 * ============================================================ */

void action_sweep_decrease(
    lv_event_t *e
)
{
    if (
        !is_oscilloscope_button_event(e)
    )
    {
        return;
    }

    OscopePage_SweepDecrease();
}


/* ============================================================
 * MAIN MESSAGE HIDE SWITCH
 * ============================================================ */

void event_handler_cb_main_msg_hide_switch(
    lv_event_t *e
)
{
    if (
        e == NULL ||
        lv_event_get_code(e) !=
        LV_EVENT_VALUE_CHANGED
    )
    {
        return;
    }

    action_error_hide(e);
}
