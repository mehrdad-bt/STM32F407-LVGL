#include "PersianText.h"

#include "lvgl.h"
#include "ui/ui.h"
#include "ui/screens.h"

/* =========================================================
 * Persian text initialization
 * ========================================================= */

void PersianText_Init(void)
{
    /*
     * Make sure the object was created by EEZ.
     */
    if (objects.persian_text == NULL)
    {
        return;
    }

    /*
     * Position.
     */
    lv_obj_set_pos(
        objects.persian_text,
        70,
        80
    );

    /*
     * Size.
     */
    lv_obj_set_size(
        objects.persian_text,
        180,
        LV_SIZE_CONTENT
    );

    /*
     * Persian / Arabic compatible font.
     */
    lv_obj_set_style_text_font(
        objects.persian_text,
        &lv_font_dejavu_16_persian_hebrew,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );

    /*
     * Right-to-left direction.
     */
    lv_obj_set_style_base_dir(
        objects.persian_text,
        LV_BASE_DIR_RTL,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );

    /*
     * Center text horizontally.
     */
    lv_obj_set_style_text_align(
        objects.persian_text,
        LV_TEXT_ALIGN_CENTER,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );

    /*
     * Persian text.
     *
     * Source file must be saved as UTF-8.
     */
    lv_label_set_text(
        objects.persian_text,
        "سلام دنیا"
    );
}
