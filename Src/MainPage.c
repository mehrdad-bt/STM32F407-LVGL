#include "MainPage.h"

#include "lvgl.h"

#include "ui/ui.h"
#include "ui/screens.h"

#include "font_persian_14.h"
#include "font_persian_24.h"

#include "MessageBox.h"
#include "Animation.h"
#include "RTC_Task.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


/* -------------------------------------------------------------------------- */
/* Private variables                                                          */
/* -------------------------------------------------------------------------- */

static bool main_uart_link_active = false;
static bool main_transfer_animation_active = false;

static float main_voltage = 0.0f;
static float main_current = 0.0f;

static lv_obj_t *main_time_label = NULL;
static lv_obj_t *main_date_label = NULL;


/* -------------------------------------------------------------------------- */
/* Main page date/time positions                                              */
/* -------------------------------------------------------------------------- */

#define MAIN_TIME_X          120
#define MAIN_TIME_Y           5

#define MAIN_DATE_X           240
#define MAIN_DATE_Y           5


/* -------------------------------------------------------------------------- */
/* Private function prototypes                                                */
/* -------------------------------------------------------------------------- */

static void MainPage_ShowInformationIndicator(void);

static void MainPage_HideInformationIndicator(void);

static void MainPage_CreateDateTimeLabels(void);

static void MainPage_GregorianToJalali(
    int gy,
    int gm,
    int gd,
    int *jy,
    int *jm,
    int *jd
);


/* -------------------------------------------------------------------------- */
/* Information indicator                                                      */
/* -------------------------------------------------------------------------- */

static void MainPage_ShowInformationIndicator(void)
{
    if (objects.information_indicator == NULL)
    {
        return;
    }

    lv_obj_clear_flag(
        objects.information_indicator,
        LV_OBJ_FLAG_HIDDEN
    );
}


static void MainPage_HideInformationIndicator(void)
{
    if (objects.information_indicator == NULL)
    {
        return;
    }

    lv_obj_add_flag(
        objects.information_indicator,
        LV_OBJ_FLAG_HIDDEN
    );
}


/* -------------------------------------------------------------------------- */
/* Gregorian -> Jalali                                                        */
/* -------------------------------------------------------------------------- */

static void MainPage_GregorianToJalali(
    int gy,
    int gm,
    int gd,
    int *jy,
    int *jm,
    int *jd
)
{
    static const int g_days_in_month[] =
    {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    static const int j_days_in_month[] =
    {
        31, 31, 31, 31, 31, 31,
        30, 30, 30, 30, 30, 29
    };

    int i;

    int gy_temp = gy - 1600;

    long g_day_no =
        365L * gy_temp +
        ((gy_temp + 3) / 4) -
        ((gy_temp + 99) / 100) +
        ((gy_temp + 399) / 400);

    for (i = 0; i < gm - 1; i++)
    {
        g_day_no += g_days_in_month[i];
    }

    if ((gm > 2) &&
        (((gy % 4 == 0) && (gy % 100 != 0)) ||
         (gy % 400 == 0)))
    {
        g_day_no++;
    }

    g_day_no += gd - 1;

    {
        long j_day_no = g_day_no - 79;
        long j_np = j_day_no / 12053;

        j_day_no %= 12053;

        *jy =
            979 +
            33 * j_np +
            4 * (j_day_no / 1461);

        j_day_no %= 1461;

        if (j_day_no >= 366)
        {
            *jy += (j_day_no - 1) / 365;

            j_day_no =
                (j_day_no - 1) % 365;
        }

        for (i = 0;
             i < 11 &&
             j_day_no >= j_days_in_month[i];
             i++)
        {
            j_day_no -= j_days_in_month[i];
        }

        *jm = i + 1;
        *jd = (int)j_day_no + 1;
    }
}


/* -------------------------------------------------------------------------- */
/* Create date/time labels                                                    */
/* -------------------------------------------------------------------------- */

static void MainPage_CreateDateTimeLabels(void)
{
    if (objects.main == NULL)
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Time label                                                             */
    /* ---------------------------------------------------------------------- */

    main_time_label = lv_label_create(objects.main);

    lv_obj_set_pos(
        main_time_label,
        MAIN_TIME_X,
        MAIN_TIME_Y
    );

    lv_obj_set_size(
        main_time_label,
        LV_SIZE_CONTENT,
        LV_SIZE_CONTENT
    );

    lv_obj_set_style_text_font(
        main_time_label,
        &lv_font_montserrat_20,
        LV_PART_MAIN
    );

    lv_obj_set_style_text_color(
        main_time_label,
		lv_color_hex(0xF4F9A8),
        LV_PART_MAIN
    );

    lv_label_set_text(
        main_time_label,
        "--:--:--"
    );


    /* ---------------------------------------------------------------------- */
    /* Date label                                                             */
    /* ---------------------------------------------------------------------- */

    main_date_label = lv_label_create(objects.main);

    lv_obj_set_pos(
        main_date_label,
        MAIN_DATE_X,
        MAIN_DATE_Y
    );

    lv_obj_set_size(
        main_date_label,
        LV_SIZE_CONTENT,
        LV_SIZE_CONTENT
    );

    /*
     * Use Montserrat for the date numbers.
     *
     * This avoids square characters caused by missing
     * Persian digit glyphs in the Persian font.
     */
    lv_obj_set_style_text_font(
        main_date_label,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );

    lv_obj_set_style_text_color(
        main_date_label,
		lv_color_hex(0xF4F9A8),
        LV_PART_MAIN
    );

    lv_label_set_text(
        main_date_label,
        "----/--/--"
    );
}


/* -------------------------------------------------------------------------- */
/* Update date/time                                                           */
/* -------------------------------------------------------------------------- */

void MainPage_UpdateDateTime(void)
{
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;

    uint8_t day;
    uint8_t month;
    uint8_t year;

    int jy;
    int jm;
    int jd;

    int gy;

    char time_text[16];
    char date_text[32];


    /*
     * Make sure labels have already been created.
     */
    if ((main_time_label == NULL) ||
        (main_date_label == NULL))
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Read time                                                              */
    /* ---------------------------------------------------------------------- */

    if (!RTC_Task_GetTime(
            &hours,
            &minutes,
            &seconds))
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Read date                                                              */
    /* ---------------------------------------------------------------------- */

    if (!RTC_Task_GetDate(
            &day,
            &month,
            &year))
    {
        return;
    }


    /*
     * RTC stores year as 00..99.
     *
     * Example:
     *
     * 26 -> 2026
     */
    gy = 2000 + year;


    /* ---------------------------------------------------------------------- */
    /* Gregorian -> Jalali                                                    */
    /* ---------------------------------------------------------------------- */

    MainPage_GregorianToJalali(
        gy,
        month,
        day,
        &jy,
        &jm,
        &jd
    );


    /* ---------------------------------------------------------------------- */
    /* Format time                                                            */
    /* ---------------------------------------------------------------------- */

    snprintf(
        time_text,
        sizeof(time_text),
        "%02u:%02u:%02u",
        hours,
        minutes,
        seconds
    );


    /* ---------------------------------------------------------------------- */
    /* Format Jalali date                                                     */
    /* ---------------------------------------------------------------------- */

    snprintf(
        date_text,
        sizeof(date_text),
        "%04d/%02d/%02d",
        jy,
        jm,
        jd
    );


    /* ---------------------------------------------------------------------- */
    /* Update labels                                                          */
    /* ---------------------------------------------------------------------- */

    lv_label_set_text(
        main_time_label,
        time_text
    );

    lv_label_set_text(
        main_date_label,
        date_text
    );
}


/* -------------------------------------------------------------------------- */
/* Main page initialization                                                   */
/* -------------------------------------------------------------------------- */

void MainPage_Init(void)
{
    /* ---------------------------------------------------------------------- */
    /* Calibration button text                                                */
    /* ---------------------------------------------------------------------- */

    if (objects.calibration_text != NULL)
    {
        lv_label_set_text(
            objects.calibration_text,
            "کالیبراسیون"
        );

        lv_obj_set_style_text_font(
            objects.calibration_text,
            &font_persian_14,
            LV_PART_MAIN
        );

        lv_obj_set_style_base_dir(
            objects.calibration_text,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN
        );

        lv_obj_set_style_text_color(
            objects.calibration_text,
            lv_color_black(),
            LV_PART_MAIN
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Temperature button text                                                */
    /* ---------------------------------------------------------------------- */

    if (objects.temp_button_text != NULL)
    {
        lv_label_set_text(
            objects.temp_button_text,
            "دماسنج"
        );

        lv_obj_set_style_text_font(
            objects.temp_button_text,
            &font_persian_14,
            LV_PART_MAIN
        );

        lv_obj_set_style_base_dir(
            objects.temp_button_text,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN
        );

        lv_obj_set_style_text_color(
            objects.temp_button_text,
            lv_color_black(),
            LV_PART_MAIN
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Information indicator                                                  */
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
            LV_PART_MAIN
        );

        lv_obj_set_style_base_dir(
            objects.information_indicator,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN
        );

        lv_obj_set_style_text_color(
            objects.information_indicator,
            lv_color_white(),
            LV_PART_MAIN
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
    /* Error show/hide text                                                   */
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
            LV_PART_MAIN
        );

        lv_obj_set_style_base_dir(
            objects.error_hide_text,
            LV_BASE_DIR_RTL,
            LV_PART_MAIN
        );

        lv_obj_set_style_text_color(
            objects.error_hide_text,
            lv_color_white(),
            LV_PART_MAIN
        );

        lv_obj_set_pos(
            objects.error_hide_text,
            120,
            200
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Message box                                                             */
    /* ---------------------------------------------------------------------- */

    MessageBox_Init();


    /* ---------------------------------------------------------------------- */
    /* Date / time                                                             */
    /* ---------------------------------------------------------------------- */

    MainPage_CreateDateTimeLabels();

    MainPage_UpdateDateTime();


    /* ---------------------------------------------------------------------- */
    /* Initial communication state                                             */
    /* ---------------------------------------------------------------------- */

    main_uart_link_active = false;

    main_transfer_animation_active = false;

    main_voltage = 0.0f;

    main_current = 0.0f;
}


/* -------------------------------------------------------------------------- */
/* UART packet received                                                       */
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
/* UART timeout                                                               */
/* -------------------------------------------------------------------------- */

void MainPage_OnUartTimeout(void)
{
    main_uart_link_active = false;
}


/* -------------------------------------------------------------------------- */
/* Main page task                                                             */
/* -------------------------------------------------------------------------- */

void MainPage_Run(void)
{
    /*
     * Communication graphics are shown only
     * while the main page is active.
     */
    if (lv_scr_act() != objects.main)
    {
        if (main_transfer_animation_active)
        {
            data_transfer_animation_stop();

            main_transfer_animation_active = false;
        }

        MainPage_HideInformationIndicator();

        return;
    }


    /* ---------------------------------------------------------------------- */
    /* UART active                                                            */
    /* ---------------------------------------------------------------------- */

    if (main_uart_link_active)
    {
        MainPage_ShowInformationIndicator();


        /*
         * Start TX/RX animation only once.
         */
        if (!main_transfer_animation_active)
        {
            data_transfer_animation_start(
                objects.main
            );

            main_transfer_animation_active = true;
        }
    }


    /* ---------------------------------------------------------------------- */
    /* UART inactive                                                          */
    /* ---------------------------------------------------------------------- */

    else
    {
        MainPage_HideInformationIndicator();


        if (main_transfer_animation_active)
        {
            data_transfer_animation_stop();

            main_transfer_animation_active = false;
        }
    }
}


/* -------------------------------------------------------------------------- */
/* Get voltage                                                                */
/* -------------------------------------------------------------------------- */

float MainPage_GetVoltage(void)
{
    return main_voltage;
}


/* -------------------------------------------------------------------------- */
/* Get current                                                                */
/* -------------------------------------------------------------------------- */

float MainPage_GetCurrent(void)
{
    return main_current;
}
