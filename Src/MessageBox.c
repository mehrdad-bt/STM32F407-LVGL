#include "MessageBox.h"

#include "font_persian_14.h"
#include "font_persian_24.h"


#include "lvgl.h"

#include "ui/ui.h"
#include "ui/screens.h"


/* -------------------------------------------------------------------------- */
/* Defines                                                                    */
/* -------------------------------------------------------------------------- */

#define MESSAGE_BOX_BLINK_PERIOD_MS    1500U


/* -------------------------------------------------------------------------- */
/* Private Variables                                                          */
/* -------------------------------------------------------------------------- */

static bool message_box_forced_hidden = false;


/* -------------------------------------------------------------------------- */
/* Message Box Blink Timer                                                    */
/* -------------------------------------------------------------------------- */

static void MessageBox_Blink_Timer(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    if (objects.message_box == NULL)
    {
        return;
    }

    /*
     * If the switch requested the MessageBox to stay hidden,
     * the timer must not show it again.
     */
    if (message_box_forced_hidden)
    {
        lv_obj_add_flag(
            objects.message_box,
            LV_OBJ_FLAG_HIDDEN
        );

        return;
    }


    /*
     * Normal blinking
     */
    if (lv_obj_has_flag(
            objects.message_box,
            LV_OBJ_FLAG_HIDDEN))
    {
        /* Show */
        lv_obj_clear_flag(
            objects.message_box,
            LV_OBJ_FLAG_HIDDEN
        );
    }
    else
    {
        /* Hide */
        lv_obj_add_flag(
            objects.message_box,
            LV_OBJ_FLAG_HIDDEN
        );
    }
}


/* -------------------------------------------------------------------------- */
/* Public: Set Hidden                                                         */
/* -------------------------------------------------------------------------- */

void MessageBox_SetHidden(bool hidden)
{
    message_box_forced_hidden = hidden;

    if (objects.message_box == NULL)
    {
        return;
    }


    if (hidden)
    {
        /*
         * Switch ON
         * -> permanently hide MessageBox
         */
        lv_obj_add_flag(
            objects.message_box,
            LV_OBJ_FLAG_HIDDEN
        );
    }
    else
    {
        /*
         * Switch OFF
         * -> show MessageBox immediately
         *
         * Blink timer will continue from here.
         */
        lv_obj_clear_flag(
            objects.message_box,
            LV_OBJ_FLAG_HIDDEN
        );
    }
}


/* -------------------------------------------------------------------------- */
/* Init                                                                       */
/* -------------------------------------------------------------------------- */

void MessageBox_Init(void)
{
    if (objects.message_box == NULL)
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Message Box Text                                                       */
    /* ---------------------------------------------------------------------- */

    if (objects.message_box_text != NULL)
    {
        lv_label_set_text(
            objects.message_box_text,
            "ولتاژ غیر مجاز"
        );


        lv_obj_set_style_text_font(
            objects.message_box_text,
            &font_persian_24,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );


        lv_obj_set_style_base_dir(
            objects.message_box_text,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );


        lv_obj_set_style_text_align(
            objects.message_box_text,
            LV_TEXT_ALIGN_CENTER,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );


        lv_obj_set_style_text_color(
            objects.message_box_text,
            lv_color_hex(0xFFFFFF),
            LV_PART_MAIN | LV_STATE_DEFAULT
        );


        /*
         * Text area
         */
        lv_obj_set_width(
            objects.message_box_text,
            220
        );

        lv_obj_set_height(
            objects.message_box_text,
            50
        );


        /*
         * Position inside MessageBox
         */
        lv_obj_set_pos(
            objects.message_box_text,
            9,
            45
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Background                                                              */
    /* ---------------------------------------------------------------------- */

    lv_obj_set_style_bg_color(
        objects.message_box,
        lv_color_hex(0xE00000),
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    lv_obj_set_style_bg_opa(
        objects.message_box,
        LV_OPA_COVER,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    /* ---------------------------------------------------------------------- */
    /* Close Button                                                            */
    /* ---------------------------------------------------------------------- */

    lv_obj_t *close_btn =
        lv_msgbox_get_close_btn(
            objects.message_box
        );

    if (close_btn != NULL)
    {
        lv_obj_add_flag(
            close_btn,
            LV_OBJ_FLAG_HIDDEN
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Initial State                                                           */
    /* ---------------------------------------------------------------------- */

    message_box_forced_hidden = false;


    /*
     * Start visible.
     *
     * After 1.5 seconds the timer will hide it.
     */
    lv_obj_clear_flag(
        objects.message_box,
        LV_OBJ_FLAG_HIDDEN
    );


    /* ---------------------------------------------------------------------- */
    /* Blink Timer                                                             */
    /* ---------------------------------------------------------------------- */

    lv_timer_create(
        MessageBox_Blink_Timer,
        MESSAGE_BOX_BLINK_PERIOD_MS,
        NULL
    );
}
