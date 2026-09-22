#include "TempPage.h"

#include "AHT10.h"

#include "lvgl.h"

#include "ui/ui.h"
#include "ui/screens.h"

#include "font_persian_24.h"
#include "font_persian_14.h"

#include "Animation.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>


/* -------------------------------------------------------------------------- */
/* Internal                                                                   */
/* -------------------------------------------------------------------------- */

static lv_timer_t *temp_page_timer = NULL;

static char last_temperature_text[32] = "";

static char last_humidity_text[32] = "";


/* -------------------------------------------------------------------------- */
/* Format Temperature                                                         */
/* -------------------------------------------------------------------------- */

static void TempPage_FormatTemperature(
    float temperature,
    char *buffer,
    size_t buffer_size
)
{
    int32_t value100;

    int32_t integer_part;

    int32_t fractional_part;


    /*
     * Convert temperature to hundredths.
     *
     * Example:
     * 23.47 -> 2347
     * -5.12 -> -512
     */

    if (temperature >= 0.0f)
    {
        value100 =
            (int32_t)(
                temperature * 100.0f +
                0.5f
            );
    }
    else
    {
        value100 =
            (int32_t)(
                temperature * 100.0f -
                0.5f
            );
    }


    integer_part =
        value100 / 100;


    fractional_part =
        value100 % 100;


    if (fractional_part < 0)
    {
        fractional_part =
            -fractional_part;
    }


    /*
     * Special case for values between -1.00 and 0.00
     * so that -0.50 is displayed correctly.
     */

    if (
        value100 < 0 &&
        integer_part == 0
    )
    {
        snprintf(
            buffer,
            buffer_size,
            "-0.%02ld C",
            (long)fractional_part
        );
    }
    else
    {
        snprintf(
            buffer,
            buffer_size,
            "%ld.%02ld C",
            (long)integer_part,
            (long)fractional_part
        );
    }
}


/* -------------------------------------------------------------------------- */
/* Format Humidity                                                            */
/* -------------------------------------------------------------------------- */

static void TempPage_FormatHumidity(
    float humidity,
    char *buffer,
    size_t buffer_size
)
{
    int32_t value100;

    int32_t integer_part;

    int32_t fractional_part;


    /*
     * Example:
     * 45.32 -> 4532
     */

    value100 =
        (int32_t)(
            humidity * 100.0f +
            0.5f
        );


    integer_part =
        value100 / 100;


    fractional_part =
        value100 % 100;


    if (fractional_part < 0)
    {
        fractional_part =
            -fractional_part;
    }


    snprintf(
        buffer,
        buffer_size,
        "%ld.%02ld %%",
        (long)integer_part,
        (long)fractional_part
    );
}


/* -------------------------------------------------------------------------- */
/* Update Labels                                                              */
/* -------------------------------------------------------------------------- */

static void TempPage_Update(void)
{
    AHT10_Data_t sensor_data;

    char temperature_text[32];

    char humidity_text[32];


    /* ---------------------------------------------------------------------- */
    /* Check objects                                                           */
    /* ---------------------------------------------------------------------- */

    if (objects.temp == NULL)
    {
        return;
    }


    if (objects.temp_text_dynamic == NULL)
    {
        return;
    }


    if (objects.humid_text_dynamic == NULL)
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Update only when Temp screen is active                                 */
    /* ---------------------------------------------------------------------- */

    if (lv_scr_act() != objects.temp)
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Get latest sensor data                                                 */
    /* ---------------------------------------------------------------------- */

    if (!AHT10_GetData(&sensor_data))
    {
        snprintf(
            temperature_text,
            sizeof(temperature_text),
            "--.-- C"
        );


        snprintf(
            humidity_text,
            sizeof(humidity_text),
            "--.-- %%"
        );
    }
    else
    {
        TempPage_FormatTemperature(
            sensor_data.temperature,
            temperature_text,
            sizeof(temperature_text)
        );


        TempPage_FormatHumidity(
            sensor_data.humidity,
            humidity_text,
            sizeof(humidity_text)
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Temperature                                                            */
    /* ---------------------------------------------------------------------- */

    if (
        strcmp(
            temperature_text,
            last_temperature_text
        ) != 0
    )
    {
        lv_label_set_text(
            objects.temp_text_dynamic,
            temperature_text
        );


        strncpy(
            last_temperature_text,
            temperature_text,
            sizeof(last_temperature_text) - 1
        );


        last_temperature_text[
            sizeof(last_temperature_text) - 1
        ] = '\0';
    }


    /* ---------------------------------------------------------------------- */
    /* Humidity                                                               */
    /* ---------------------------------------------------------------------- */

    if (
        strcmp(
            humidity_text,
            last_humidity_text
        ) != 0
    )
    {
        lv_label_set_text(
            objects.humid_text_dynamic,
            humidity_text
        );


        strncpy(
            last_humidity_text,
            humidity_text,
            sizeof(last_humidity_text) - 1
        );


        last_humidity_text[
            sizeof(last_humidity_text) - 1
        ] = '\0';
    }
}


/* -------------------------------------------------------------------------- */
/* LVGL Timer Callback                                                        */
/* -------------------------------------------------------------------------- */

static void TempPage_TimerCallback(
    lv_timer_t *timer
)
{
    (void)timer;

    TempPage_Update();
}


/* -------------------------------------------------------------------------- */
/* Init                                                                       */
/* -------------------------------------------------------------------------- */

void TempPage_Init(void)
{
    /* ---------------------------------------------------------------------- */
    /* Header                                                                  */
    /* ---------------------------------------------------------------------- */

    if (
        objects.temp_humid_header_text != NULL
    )
    {
        lv_label_set_text(
            objects.temp_humid_header_text,
            "در حال اندازه گیری دما و رطوبت"
        );


        lv_obj_set_style_text_font(
            objects.temp_humid_header_text,
            &font_persian_14,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );


        lv_obj_set_style_base_dir(
            objects.temp_humid_header_text,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Temperature static text                                                */
    /* ---------------------------------------------------------------------- */

    if (
        objects.temp_text_static != NULL
    )
    {
        lv_label_set_text(
            objects.temp_text_static,
            "دما:"
        );


        lv_obj_set_style_text_font(
            objects.temp_text_static,
            &font_persian_24,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );


        lv_obj_set_style_base_dir(
            objects.temp_text_static,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Humidity static text                                                   */
    /* ---------------------------------------------------------------------- */

    if (
        objects.humid_text_static != NULL
    )
    {
        lv_label_set_text(
            objects.humid_text_static,
            ":رطوبت"
        );


        lv_obj_set_style_text_font(
            objects.humid_text_static,
            &font_persian_24,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );


        lv_obj_set_style_base_dir(
            objects.humid_text_static,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Exit button                                                             */
    /* ---------------------------------------------------------------------- */

    if (
        objects.temp_exit_button_text != NULL
    )
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
    /* Dynamic temperature                                                     */
    /* ---------------------------------------------------------------------- */

    if (
        objects.temp_text_dynamic != NULL
    )
    {
        lv_obj_set_style_text_font(
            objects.temp_text_dynamic,
            &lv_font_montserrat_30,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Dynamic humidity                                                        */
    /* ---------------------------------------------------------------------- */

    if (
        objects.humid_text_dynamic != NULL
    )
    {
        lv_obj_set_style_text_font(
            objects.humid_text_dynamic,
            &lv_font_montserrat_30,
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Waiting animation                                                       */
    /* ---------------------------------------------------------------------- */

    if (objects.temp != NULL)
    {
        Animation_CreateWaitingDots(
            objects.temp,
            90,
            12
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Timer                                                                   */
    /* ---------------------------------------------------------------------- */

    if (temp_page_timer == NULL)
    {
        temp_page_timer =
            lv_timer_create(
                TempPage_TimerCallback,
                500,
                NULL
            );
    }


    TempPage_OnEnter();
}


/* -------------------------------------------------------------------------- */
/* Enter Page                                                                 */
/* -------------------------------------------------------------------------- */

void TempPage_OnEnter(void)
{
    /*
     * Reset cached strings.
     * This guarantees an update when entering the page.
     */

    last_temperature_text[0] = '\0';

    last_humidity_text[0] = '\0';


    TempPage_Update();
}
