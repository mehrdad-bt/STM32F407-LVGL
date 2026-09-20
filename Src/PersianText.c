#include <font_persian_24.h>
#include "PersianText.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/screens.h"


void PersianText_Init(void)
{
    if (objects.calibration_text != NULL)
    {
        lv_label_set_text(objects.calibration_text, "کالیبراسیون");

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

    if (objects.persian_text != NULL)
        {
          lv_obj_set_pos(objects.persian_text, 40, 70);

          lv_obj_set_width(objects.persian_text, 240);

          lv_obj_set_height(objects.persian_text, 40);

            lv_obj_set_style_text_font(
                objects.persian_text,
                &font_persian_24,
                LV_PART_MAIN | LV_STATE_DEFAULT
            );

            lv_obj_set_style_base_dir(
                objects.persian_text,
                LV_BASE_DIR_RTL,
                LV_PART_MAIN | LV_STATE_DEFAULT
            );

            lv_obj_set_style_text_align(
                objects.persian_text,
                LV_TEXT_ALIGN_CENTER,
                LV_PART_MAIN | LV_STATE_DEFAULT
            );

            lv_obj_set_style_text_color(
                objects.persian_text,
                lv_color_hex(0x000000),
                LV_PART_MAIN | LV_STATE_DEFAULT
            );

            lv_label_set_text(
                objects.persian_text,
                "ولتاژ پایین تر از حد مجاز"
            );

            lv_obj_invalidate(objects.persian_text);
        }
  }

