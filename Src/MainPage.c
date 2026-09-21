#include "MainPage.h"

#include "lvgl.h"

#include "ui/ui.h"
#include "ui/screens.h"

#include "font_persian_14.h"

#include "MessageBox.h"
#include "Animation.h"


/* -------------------------------------------------------------------------- */
/* Private Variables                                                          */
/* -------------------------------------------------------------------------- */

static bool main_uart_link_active = false;

static bool main_transfer_animation_active = false;

static float main_voltage = 0.0f;

static float main_current = 0.0f;


/* -------------------------------------------------------------------------- */
/* Information Indicator                                                      */
/* -------------------------------------------------------------------------- */

static void MainPage_ShowInformationIndicator(void)
{
    if (
        objects.information_indicator ==
        NULL
    )
    {
        return;
    }


    lv_label_set_text(
        objects.information_indicator,
        "در حال دریافت اطلاعات"
    );


    lv_obj_clear_flag(
        objects.information_indicator,
        LV_OBJ_FLAG_HIDDEN
    );
}


static void MainPage_HideInformationIndicator(void)
{
    if (
        objects.information_indicator ==
        NULL
    )
    {
        return;
    }


    lv_obj_add_flag(
        objects.information_indicator,
        LV_OBJ_FLAG_HIDDEN
    );
}


/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

void MainPage_Init(void)
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
    /* Temp Button Text                                                       */
    /* ---------------------------------------------------------------------- */

    if (objects.temp_button_text != NULL)
    {
        lv_label_set_text(
            objects.temp_button_text,
            "دماسنج"
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
    /* Information Indicator                                                  */
    /* ---------------------------------------------------------------------- */

    if (
        objects.information_indicator != NULL
    )
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


    /* ---------------------------------------------------------------------- */
    /* Message Box                                                             */
    /* ---------------------------------------------------------------------- */

    MessageBox_Init();
}


/* -------------------------------------------------------------------------- */
/* UART Packet                                                                */
/* -------------------------------------------------------------------------- */

void MainPage_OnUartPacket(
    float voltage,
    float current
)
{
    main_voltage = voltage;

    main_current = current;

    main_uart_link_active = true;
}


/* -------------------------------------------------------------------------- */
/* UART Timeout                                                               */
/* -------------------------------------------------------------------------- */

void MainPage_OnUartTimeout(void)
{
    main_uart_link_active = false;
}


/* -------------------------------------------------------------------------- */
/* Run                                                                        */
/* -------------------------------------------------------------------------- */

void MainPage_Run(void)
{
    bool main_page_active;


    if (objects.main == NULL)
    {
        return;
    }


    main_page_active =
        (
            lv_scr_act() ==
            objects.main
        );


    /* ---------------------------------------------------------------------- */
    /* Communication Active                                                   */
    /* ---------------------------------------------------------------------- */

    if (
        main_page_active &&
        main_uart_link_active
    )
    {
        MainPage_ShowInformationIndicator();


        if (
            !main_transfer_animation_active
        )
        {
            data_transfer_animation_start(
                objects.main
            );


            main_transfer_animation_active =
                true;
        }
    }


    /* ---------------------------------------------------------------------- */
    /* Communication Inactive OR Other Page                                   */
    /* ---------------------------------------------------------------------- */

    else
    {
        MainPage_HideInformationIndicator();


        if (
            main_transfer_animation_active
        )
        {
            data_transfer_animation_stop();


            main_transfer_animation_active =
                false;
        }
    }
}


/* -------------------------------------------------------------------------- */
/* Get Voltage                                                                */
/* -------------------------------------------------------------------------- */

float MainPage_GetVoltage(void)
{
    return main_voltage;
}


/* -------------------------------------------------------------------------- */
/* Get Current                                                                */
/* -------------------------------------------------------------------------- */

float MainPage_GetCurrent(void)
{
    return main_current;
}
