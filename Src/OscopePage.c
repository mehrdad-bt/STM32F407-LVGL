#include "OscopePage.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "lvgl.h"

#include "ui/ui.h"
#include "ui/screens.h"

#include "OscopeADC.h"


/* ==========================================================
 * Configuration
 * ========================================================== */

/*
 * تعداد نمونه‌ای که ADC در هر بلوک می‌گیرد.
 * باید با OscopeADC.h هماهنگ باشد.
 */
#define OSCOPE_POINT_COUNT             300U

/*
 * تعداد تقسیمات افقی و عمودی اسیلوسکوپ
 */
#define OSCOPE_HORIZONTAL_DIVS         10U
#define OSCOPE_VERTICAL_DIVS           8U

/*
 * نرخ تقریبی نمونه‌برداری فعلی.
 *
 * در حالت فعلی ADC با Polling کار می‌کند و نرخ دقیق سخت‌افزاری
 * مانند DMA + Timer نداریم. این مقدار برای تبدیل Time/Div
 * به تعداد نمونه استفاده می‌شود.
 *
 * با تنظیم فعلی ADC حدود 200 kSample/s را به عنوان مقدار کاری
 * در نظر می‌گیریم.
 */
#define OSCOPE_SAMPLE_RATE_HZ          200000UL

/*
 * تعداد نمونه‌های قبل از Trigger
 *
 * Trigger در حدود 25% عرض صفحه قرار می‌گیرد.
 */
#define OSCOPE_PRETRIGGER_PERCENT      25U


/* ==========================================================
 * Time / Division
 * ========================================================== */

/*
 * Time/Div به میکروثانیه
 *
 * با نرخ تقریبی 200kHz:
 *
 * 20 us/div  -> حدود 40 نمونه
 * 50 us/div  -> حدود 100 نمونه
 * 100 us/div -> حدود 200 نمونه
 * 150 us/div -> حدود 300 نمونه
 *
 * فعلاً بالاتر از 150us/div نمی‌رویم چون با 300 نمونه
 * موجود، اطلاعات کافی برای نمایش واقعی آن نداریم.
 */
static const uint32_t time_div_values_us[] =
{
    20U,
    50U,
    100U,
    150U
};

#define TIME_DIV_COUNT \
    (sizeof(time_div_values_us) / sizeof(time_div_values_us[0]))

static uint32_t time_div_index = 2U; /* default = 100us/div */


/* ==========================================================
 * Volt / Division
 * ========================================================== */

static const float volt_div_values[] =
{
    0.1f,
    0.2f,
    0.5f,
    1.0f,
    2.0f
};

#define VOLT_DIV_COUNT \
    (sizeof(volt_div_values) / sizeof(volt_div_values[0]))

static uint32_t volt_div_index = 2U; /* default = 0.5V/div */


/* ==========================================================
 * Trigger
 * ========================================================== */

/*
 * Trigger بر حسب ADC Count
 *
 * ADC:
 * 0V    -> 0
 * 3.3V  -> حدود 4095
 */
#define ADC_MAX_VALUE                  4095U
#define ADC_REFERENCE_VOLTAGE          3.3f

/*
 * حدود Trigger
 */
#define TRIGGER_MIN_ADC                128U
#define TRIGGER_MAX_ADC                3967U
#define TRIGGER_STEP_ADC               124U

static uint16_t trigger_level_adc = 2048U;


/* ==========================================================
 * Run / Stop
 * ========================================================== */

static bool oscope_running = true;


/* ==========================================================
 * LVGL objects
 * ========================================================== */

static lv_timer_t *oscope_timer = NULL;

static lv_obj_t *oscope_value_label = NULL;


/* ==========================================================
 * Waveform buffers
 * ========================================================== */

static uint16_t oscope_samples[OSCOPE_POINT_COUNT];

static uint16_t oscope_last_samples[OSCOPE_POINT_COUNT];

static uint16_t oscope_last_count = 0U;


/* ==========================================================
 * Forward declarations
 * ========================================================== */

static void OscopePage_ConfigureChart(void);
static void OscopePage_UpdateLabel(void);
static void OscopePage_UpdateRunStopButtonText(void);

static uint16_t OscopePage_GetVisibleSampleCount(void);

static uint16_t OscopePage_MapSampleToChart(uint16_t adc_value);

static int OscopePage_FindTriggerIndex(
    const uint16_t *samples,
    uint16_t count
);

static void OscopePage_RenderSamples(
    const uint16_t *samples,
    uint16_t count
);

static void OscopePage_RenderLastSamples(void);

static void OscopePage_TimerCallback(lv_timer_t *timer);


/* ==========================================================
 * ADC -> Voltage
 * ========================================================== */

static float OscopePage_AdcToVoltage(uint16_t adc)
{
    return ((float)adc * ADC_REFERENCE_VOLTAGE) /
           (float)ADC_MAX_VALUE;
}


/* ==========================================================
 * Get visible sample count from Time/Div
 * ========================================================== */

static uint16_t OscopePage_GetVisibleSampleCount(void)
{
    uint32_t time_per_div_us;
    uint32_t total_time_us;
    uint32_t samples;

    time_per_div_us = time_div_values_us[time_div_index];

    total_time_us =
        time_per_div_us * OSCOPE_HORIZONTAL_DIVS;

    /*
     * samples =
     *
     * sample_rate * total_time
     *
     * Hz * us / 1,000,000
     */
    samples =
        (OSCOPE_SAMPLE_RATE_HZ * total_time_us) /
        1000000UL;

    if (samples < 10U)
    {
        samples = 10U;
    }

    if (samples > OSCOPE_POINT_COUNT)
    {
        samples = OSCOPE_POINT_COUNT;
    }

    return (uint16_t)samples;
}


/* ==========================================================
 * Convert ADC sample to chart coordinate
 * according to Volt/Div
 * ========================================================== */

static uint16_t OscopePage_MapSampleToChart(uint16_t adc_value)
{
    float volt_div;
    float total_voltage_range;
    float center_voltage;
    float sample_voltage;

    float normalized;
    float chart_value;

    /*
     * 8 vertical divisions
     */
    volt_div = volt_div_values[volt_div_index];

    total_voltage_range =
        volt_div * (float)OSCOPE_VERTICAL_DIVS;

    center_voltage = ADC_REFERENCE_VOLTAGE / 2.0f;

    sample_voltage = OscopePage_AdcToVoltage(adc_value);

    /*
     * تبدیل نسبت به مرکز
     */
    normalized =
        (sample_voltage - center_voltage) /
        total_voltage_range;

    /*
     * center of chart = 2047.5
     */
    chart_value =
        2047.5f + (normalized * (float)ADC_MAX_VALUE);

    if (chart_value < 0.0f)
    {
        chart_value = 0.0f;
    }

    if (chart_value > (float)ADC_MAX_VALUE)
    {
        chart_value = (float)ADC_MAX_VALUE;
    }

    return (uint16_t)chart_value;
}


/* ==========================================================
 * Find rising trigger edge
 * ========================================================== */

static int OscopePage_FindTriggerIndex(
    const uint16_t *samples,
    uint16_t count
)
{
    uint16_t i;

    if (samples == NULL || count < 2U)
    {
        return -1;
    }

    /*
     * Rising edge:
     *
     * sample[i-1] < trigger
     * sample[i]   >= trigger
     */
    for (i = 1U; i < count; i++)
    {
        if (samples[i - 1U] < trigger_level_adc &&
            samples[i] >= trigger_level_adc)
        {
            return (int)i;
        }
    }

    return -1;
}


/* ==========================================================
 * Configure chart
 * ========================================================== */

static void OscopePage_ConfigureChart(void)
{
    if (objects.chart_oscope == NULL)
    {
        return;
    }

    lv_chart_set_type(
        objects.chart_oscope,
        LV_CHART_TYPE_LINE
    );

    lv_chart_set_point_count(
        objects.chart_oscope,
        OSCOPE_POINT_COUNT
    );

    lv_chart_set_range(
        objects.chart_oscope,
        LV_CHART_AXIS_PRIMARY_Y,
        0,
        ADC_MAX_VALUE
    );

    lv_chart_set_update_mode(
        objects.chart_oscope,
        LV_CHART_UPDATE_MODE_SHIFT
    );

    /*
     * حذف نقطه‌های کوچک روی waveform
     */
    lv_obj_set_style_size(
        objects.chart_oscope,
        0,
        LV_PART_INDICATOR
    );
}


/* ==========================================================
 * Create value/settings label
 * ========================================================== */

static void OscopePage_CreateLabel(void)
{
    if (oscope_value_label != NULL)
    {
        return;
    }

    if (objects.osilloscop == NULL)
    {
        return;
    }

    oscope_value_label = lv_label_create(objects.osilloscop);

    if (oscope_value_label == NULL)
    {
        return;
    }

    lv_obj_set_pos(
        oscope_value_label,
        15,
        14
    );

    lv_obj_set_size(
        oscope_value_label,
        285,
        LV_SIZE_CONTENT
    );

    lv_obj_set_style_text_color(
        oscope_value_label,
        lv_color_white(),
        LV_PART_MAIN | LV_STATE_DEFAULT
    );

    lv_obj_set_style_text_font(
        oscope_value_label,
        &lv_font_montserrat_12,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );

    lv_label_set_text(
        oscope_value_label,
        "ADC:---- MIN:---- MAX:----\n"
        "T:100us V:0.5V TR:1.65V RUN"
    );
}


/* ==========================================================
 * Update value/settings label
 * ========================================================== */

static void OscopePage_UpdateLabel(void)
{
    char text[128];

    uint16_t latest = 0U;
    uint16_t min_value = ADC_MAX_VALUE;
    uint16_t max_value = 0U;

    uint16_t i;

    float trigger_voltage;

    const char *run_text;

    if (oscope_value_label == NULL)
    {
        return;
    }

    if (oscope_last_count > 0U)
    {
        latest = oscope_last_samples[oscope_last_count - 1U];

        for (i = 0U; i < oscope_last_count; i++)
        {
            if (oscope_last_samples[i] < min_value)
            {
                min_value = oscope_last_samples[i];
            }

            if (oscope_last_samples[i] > max_value)
            {
                max_value = oscope_last_samples[i];
            }
        }
    }
    else
    {
        min_value = 0U;
        max_value = 0U;
    }

    trigger_voltage =
        OscopePage_AdcToVoltage(trigger_level_adc);

    run_text =
        oscope_running ? "RUN" : "STOP";

    snprintf(
        text,
        sizeof(text),
        "ADC:%4u MIN:%4u MAX:%4u\n"
        "T:%luus V:%.1fV TR:%.2fV %s",
        latest,
        min_value,
        max_value,
        (unsigned long)time_div_values_us[time_div_index],
        (double)volt_div_values[volt_div_index],
        (double)trigger_voltage,
        run_text
    );

    lv_label_set_text(
        oscope_value_label,
        text
    );
}


/* ==========================================================
 * Update STOP/RUN button text
 * ========================================================== */

static void OscopePage_UpdateRunStopButtonText(void)
{
    lv_obj_t *label;

    if (objects.stop_run_btn == NULL)
    {
        return;
    }

    /*
     * در screen.c این label اسم ندارد.
     * اولین child دکمه همان label است.
     */
    label = lv_obj_get_child(
        objects.stop_run_btn,
        0
    );

    if (label == NULL)
    {
        return;
    }

    if (oscope_running)
    {
        lv_label_set_text(
            label,
            "STOP"
        );
    }
    else
    {
        lv_label_set_text(
            label,
            "RUN"
        );
    }
}


/* ==========================================================
 * Render samples
 * ========================================================== */

static void OscopePage_RenderSamples(
    const uint16_t *samples,
    uint16_t count
)
{
    lv_chart_series_t *series;

    uint16_t visible_count;

    int trigger_index;

    int start_index;

    uint16_t i;

    if (samples == NULL)
    {
        return;
    }

    if (count == 0U)
    {
        return;
    }

    if (objects.chart_oscope == NULL)
    {
        return;
    }

    /*
     * اگر series قبلاً ساخته نشده باشد، آن را بساز.
     */
    series = lv_chart_get_series_next(
        objects.chart_oscope,
        NULL
    );

    if (series == NULL)
    {
        series = lv_chart_add_series(
            objects.chart_oscope,
            lv_palette_main(LV_PALETTE_RED),
            LV_CHART_AXIS_PRIMARY_Y
        );
    }

    if (series == NULL)
    {
        return;
    }

    visible_count =
        OscopePage_GetVisibleSampleCount();

    /*
     * پیدا کردن Trigger
     */
    trigger_index =
        OscopePage_FindTriggerIndex(
            samples,
            count
        );

    if (trigger_index >= 0)
    {
        /*
         * Trigger در 25 درصد اول صفحه قرار می‌گیرد.
         */
        start_index =
            trigger_index -
            (int)(
                ((uint32_t)visible_count *
                 OSCOPE_PRETRIGGER_PERCENT) /
                100U
            );

        /*
         * wrap
         */
        while (start_index < 0)
        {
            start_index += count;
        }

        while (start_index >= (int)count)
        {
            start_index -= count;
        }
    }
    else
    {
        /*
         * اگر Trigger پیدا نشد، از ابتدای بلوک استفاده می‌کنیم.
         */
        start_index = 0;
    }

    /*
     * تمام نقاط قبلی پاک شوند.
     */
    lv_chart_set_all_value(
        objects.chart_oscope,
        series,
        0
    );

    /*
     * نقاط جدید
     */
    for (i = 0U; i < visible_count; i++)
    {
        uint16_t source_index;
        uint16_t chart_value;

        source_index =
            (uint16_t)(
                (start_index + (int)i) %
                (int)count
            );

        chart_value =
            OscopePage_MapSampleToChart(
                samples[source_index]
            );

        lv_chart_set_next_value(
            objects.chart_oscope,
            series,
            (lv_coord_t)chart_value
        );
    }

    lv_chart_refresh(
        objects.chart_oscope
    );

    lv_obj_invalidate(
        objects.chart_oscope
    );
}


/* ==========================================================
 * Render last captured block
 * ========================================================== */

static void OscopePage_RenderLastSamples(void)
{
    if (oscope_last_count == 0U)
    {
        return;
    }

    OscopePage_RenderSamples(
        oscope_last_samples,
        oscope_last_count
    );

    OscopePage_UpdateLabel();
}


/* ==========================================================
 * Timer callback
 * ========================================================== */

static void OscopePage_TimerCallback(lv_timer_t *timer)
{
    HAL_StatusTypeDef status;

    uint16_t block_size;

    (void)timer;

    if (!oscope_running)
    {
        return;
    }

    block_size =
        OscopeADC_GetBlockSize();

    if (block_size == 0U)
    {
        return;
    }

    if (block_size > OSCOPE_POINT_COUNT)
    {
        block_size = OSCOPE_POINT_COUNT;
    }

    status =
        OscopeADC_ReadSamples(
            oscope_samples,
            block_size
        );

    if (status != HAL_OK)
    {
        return;
    }

    /*
     * آخرین بلوک را ذخیره می‌کنیم تا با تغییر تنظیمات
     * بتوانیم بدون ADC مجدد آن را redraw کنیم.
     */
    for (uint16_t i = 0U; i < block_size; i++)
    {
        oscope_last_samples[i] =
            oscope_samples[i];
    }

    oscope_last_count = block_size;

    OscopePage_RenderSamples(
        oscope_last_samples,
        oscope_last_count
    );

    OscopePage_UpdateLabel();
}


/* ==========================================================
 * Page Enter
 * ========================================================== */

void OscopePage_OnEnter(void)
{
    lv_chart_series_t *series;

    /*
     * تنظیمات اولیه
     */
    time_div_index = 2U;   /* 100us/div */
    volt_div_index = 2U;   /* 0.5V/div */

    trigger_level_adc = 2048U;

    oscope_running = true;

    oscope_last_count = 0U;

    /*
     * Chart
     */
    OscopePage_ConfigureChart();

    /*
     * اگر series وجود ندارد، آن را بساز.
     */
    series = lv_chart_get_series_next(
        objects.chart_oscope,
        NULL
    );

    if (series == NULL)
    {
        series = lv_chart_add_series(
            objects.chart_oscope,
            lv_palette_main(LV_PALETTE_RED),
            LV_CHART_AXIS_PRIMARY_Y
        );
    }

    if (series != NULL)
    {
        lv_chart_set_all_value(
            objects.chart_oscope,
            series,
            0
        );
    }

    /*
     * Label
     */
    OscopePage_CreateLabel();

    OscopePage_UpdateRunStopButtonText();
    OscopePage_UpdateLabel();

    /*
     * Timer
     *
     * ADC polling خودش زمان‌بر است، بنابراین timer را
     * خیلی سریع نمی‌گذاریم.
     */
    if (oscope_timer != NULL)
    {
        lv_timer_del(oscope_timer);
        oscope_timer = NULL;
    }

    oscope_timer =
        lv_timer_create(
            OscopePage_TimerCallback,
            30,
            NULL
        );
}


/* ==========================================================
 * Page Exit
 * ========================================================== */

void OscopePage_OnExit(void)
{
    if (oscope_timer != NULL)
    {
        lv_timer_del(oscope_timer);
        oscope_timer = NULL;
    }

    if (oscope_value_label != NULL)
    {
        lv_obj_del(oscope_value_label);
        oscope_value_label = NULL;
    }

    oscope_running = false;
}


/* ==========================================================
 * TIME +
 * ========================================================== */

void OscopePage_TimeIncrease(void)
{
    if (time_div_index + 1U >= TIME_DIV_COUNT)
    {
        time_div_index =
            TIME_DIV_COUNT - 1U;
    }
    else
    {
        time_div_index++;
    }

    OscopePage_RenderLastSamples();
}


/* ==========================================================
 * TIME -
 * ========================================================== */

void OscopePage_TimeDecrease(void)
{
    if (time_div_index > 0U)
    {
        time_div_index--;
    }

    OscopePage_RenderLastSamples();
}


/* ==========================================================
 * VOLT +
 * ========================================================== */

void OscopePage_VoltIncrease(void)
{
    if (volt_div_index + 1U >= VOLT_DIV_COUNT)
    {
        volt_div_index =
            VOLT_DIV_COUNT - 1U;
    }
    else
    {
        volt_div_index++;
    }

    OscopePage_RenderLastSamples();
}


/* ==========================================================
 * VOLT -
 * ========================================================== */

void OscopePage_VoltDecrease(void)
{
    if (volt_div_index > 0U)
    {
        volt_div_index--;
    }

    OscopePage_RenderLastSamples();
}


/* ==========================================================
 * TRIGGER +
 * ========================================================== */

void OscopePage_TriggerIncrease(void)
{
    uint32_t new_value;

    new_value =
        (uint32_t)trigger_level_adc +
        TRIGGER_STEP_ADC;

    if (new_value > TRIGGER_MAX_ADC)
    {
        new_value = TRIGGER_MAX_ADC;
    }

    trigger_level_adc =
        (uint16_t)new_value;

    OscopePage_RenderLastSamples();
}


/* ==========================================================
 * TRIGGER -
 * ========================================================== */

void OscopePage_TriggerDecrease(void)
{
    int32_t new_value;

    new_value =
        (int32_t)trigger_level_adc -
        (int32_t)TRIGGER_STEP_ADC;

    if (new_value < TRIGGER_MIN_ADC)
    {
        new_value = TRIGGER_MIN_ADC;
    }

    trigger_level_adc =
        (uint16_t)new_value;

    OscopePage_RenderLastSamples();
}


/* ==========================================================
 * RUN / STOP
 * ========================================================== */

void OscopePage_ToggleRunStop(void)
{
    oscope_running =
        !oscope_running;

    OscopePage_UpdateRunStopButtonText();
    OscopePage_UpdateLabel();
}
