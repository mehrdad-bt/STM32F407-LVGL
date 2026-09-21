#include "font_persian_14.h"
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

        lv_obj_set_style_text_color(
            objects.calibration_text,
            lv_color_hex(0x000000),
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
    }


    /* ---------------------------------------------------------------------- */
    /* temp_button_text                                                       */
    /* ---------------------------------------------------------------------- */

    if (objects.temp_button_text != NULL)
    {
        lv_label_set_text(
            objects.temp_button_text,
            "دما سنج"
        );

        lv_obj_set_style_text_font(
            objects.temp_button_text,
            &lv_font_dejavu_16_persian_hebrew,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );

        lv_obj_set_style_base_dir(
            objects.temp_button_text,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );

        lv_obj_set_style_text_color(
            objects.temp_button_text,
            lv_color_hex(0x000000),
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
    }


    /* ---------------------------------------------------------------------- */
    /* temp_exit_button_text                                                  */
    /* ---------------------------------------------------------------------- */

    if (objects.temp_exit_button_text != NULL)
    {
        lv_label_set_text(
            objects.temp_exit_button_text,
            "خروج"
        );

        lv_obj_set_style_text_font(
            objects.temp_exit_button_text,
            &lv_font_dejavu_16_persian_hebrew,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );

        lv_obj_set_style_base_dir(
            objects.temp_exit_button_text,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );

        lv_obj_set_style_text_color(
            objects.temp_exit_button_text,
            lv_color_hex(0xffffff),
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
            ""
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

        lv_obj_set_pos(
            objects.information_indicator,
            40,
            5
        );

        /*
         * Initially hidden.
         *
         * It will be shown when UART data is received.
         */

        lv_obj_add_flag(
            objects.information_indicator,
            LV_OBJ_FLAG_HIDDEN
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Error Hide/Show                                                        */
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

        lv_obj_set_pos(
            objects.error_hide_text,
            120,
            200
        );

        lv_obj_set_style_text_color(
            objects.error_hide_text,
            lv_color_hex(0xffffff),
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
    }
}
