#include "font_persian_14.h"
#include "font_persian_24.h"
#include "PersianText.h"

#include "lvgl.h"
#include "ui/ui.h"
#include "ui/screens.h"


void PersianText_Init(void)
{
    /* ---------------------------------------------------------------------- */
    /* Calibration Text                                                       */
    /* ---------------------------------------------------------------------- */

    if (objects.calibration_text != NULL)
    {
        lv_label_set_text(
            objects.calibration_text,
            "کالیبراسیون"
        );

        lv_obj_set_style_text_font(
            objects.calibration_text,
            &lv_font_dejavu_16_persian_hebrew,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );

        lv_obj_set_style_base_dir(
            objects.calibration_text,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Information Indicator                                                  */
    /* ---------------------------------------------------------------------- */

    if (objects.information_indicator != NULL)
    {
        lv_label_set_text(
            objects.information_indicator,
            "در حال دریافت اطلاعات"
        );

        lv_obj_set_style_text_font(
            objects.information_indicator,
            &font_persian_14,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );

        lv_obj_set_style_base_dir(
            objects.information_indicator,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
        lv_obj_set_pos(objects.information_indicator, 140, 1);
    }

    /* ---------------------------------------------------------------------- */
    /* Error Hide/Show                                                  */
    /* ---------------------------------------------------------------------- */

    if (objects.error_hide_text != NULL)
    {
        lv_label_set_text(
            objects.error_hide_text,
            "نمایش/مخفی کردن خطا"
        );

        lv_obj_set_style_text_font(
            objects.error_hide_text,
            &font_persian_14,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );

        lv_obj_set_style_base_dir(
            objects.error_hide_text,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
        lv_obj_set_pos(objects.error_hide_text, 120, 200);
    }

}
