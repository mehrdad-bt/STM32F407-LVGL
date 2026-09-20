#include "actions.h"
#include "touchCalibration.h"

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
