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

#define OSCOPE_POINT_COUNT             300U

#define OSCOPE_ADC_MAX                4095U

#define OSCOPE_ADC_REFERENCE          3.3f

#define OSCOPE_HORIZONTAL_DIVS        10U

#define OSCOPE_VERTICAL_DIVS           8U

#define OSCOPE_TRIGGER_POSITION       25U


/* ==========================================================
 * Time / Div
 * ========================================================== */

/*
 * Sample rate = 200 kHz
 *
 * Samples:
 *
 * 20 us/div  -> 4 samples/div  -> 40 samples
 * 50 us/div  -> 10 samples/div -> 100 samples
 * 100 us/div -> 20 samples/div -> 200 samples
 * 200 us/div -> 40 samples/div -> 400 samples
 */
static const uint32_t time_div_us[] =
{
    20U,
    50U,
    100U,
    200U
};


#define TIME_DIV_COUNT \
    (sizeof(time_div_us) / sizeof(time_div_us[0]))


static uint32_t time_div_index = 2U;


/* ==========================================================
 * Volt / Div
 * ========================================================== */

static const float volt_div_values[] =
{
    0.1f,
    0.2f,
    0.5f,
    1.0f,
    2.0f
};


static const char *volt_div_labels[] =
{
    "0.1V",
    "0.2V",
    "0.5V",
    "1.0V",
    "2.0V"
};


#define VOLT_DIV_COUNT \
    (sizeof(volt_div_values) / sizeof(volt_div_values[0]))


static uint32_t volt_div_index = 2U;


/* ==========================================================
 * Trigger
 * ========================================================== */

#define TRIGGER_MIN_ADC      128U
#define TRIGGER_MAX_ADC     3967U
#define TRIGGER_STEP_ADC     128U

static uint16_t trigger_level_adc = 2048U;


/* ==========================================================
 * Run / Stop
 * ========================================================== */

static bool oscope_running = false;


/* ==========================================================
 * LVGL
 * ========================================================== */

static lv_timer_t *oscope_timer = NULL;

static lv_obj_t *oscope_info_label = NULL;

static lv_chart_series_t *oscope_series = NULL;


/* ==========================================================
 * Samples
 * ========================================================== */

static uint16_t oscope_samples[
    OSCOPE_ADC_BLOCK_SIZE
];

static uint16_t oscope_last_count = 0U;


/* ==========================================================
 * Forward declarations
 * ========================================================== */

static void OscopePage_ConfigureChart(void);

static void OscopePage_CreateInfoLabel(void);

static void OscopePage_UpdateInfoLabel(void);

static void OscopePage_UpdateRunStopButton(void);

static uint16_t OscopePage_GetVisibleSampleCount(void);

static float OscopePage_AdcToVoltage(
    uint16_t adc
);

static uint16_t OscopePage_AdcToChart(
    uint16_t adc
);

static int OscopePage_FindTrigger(
    const uint16_t *samples,
    uint16_t count
);

static void OscopePage_DrawSamples(
    const uint16_t *samples,
    uint16_t count
);

static void OscopePage_DrawLastSamples(void);

static void OscopePage_TimerCallback(
    lv_timer_t *timer
);


/* ==========================================================
 * ADC -> Voltage
 * ========================================================== */

static float OscopePage_AdcToVoltage(
    uint16_t adc
)
{
    return
        ((float)adc *
         OSCOPE_ADC_REFERENCE) /
        (float)OSCOPE_ADC_MAX;
}


/* ==========================================================
 * Time/Div -> number of source samples
 * ========================================================== */

static uint16_t OscopePage_GetVisibleSampleCount(void)
{
    uint32_t total_time_us;
    uint32_t samples;


    total_time_us =
        time_div_us[time_div_index] *
        OSCOPE_HORIZONTAL_DIVS;


    samples =
        (
            OscopeADC_GetSampleRate() *
            total_time_us
        ) / 1000000UL;


    if (samples < 20U)
    {
        samples = 20U;
    }


    if (samples >
        OSCOPE_ADC_BLOCK_SIZE)
    {
        samples =
            OSCOPE_ADC_BLOCK_SIZE;
    }


    return (uint16_t)samples;
}


/* ==========================================================
 * ADC -> Chart
 *
 * Vertical center = 1.65V
 * ========================================================== */

static uint16_t OscopePage_AdcToChart(
    uint16_t adc
)
{
    float sample_voltage;

    float center_voltage;

    float total_range;

    float min_voltage;

    float ratio;

    float chart_value;


    sample_voltage =
        OscopePage_AdcToVoltage(
            adc
        );


    center_voltage =
        OSCOPE_ADC_REFERENCE /
        2.0f;


    total_range =
        volt_div_values[volt_div_index] *
        (float)OSCOPE_VERTICAL_DIVS;


    if (total_range < 0.001f)
    {
        total_range =
            OSCOPE_ADC_REFERENCE;
    }


    min_voltage =
        center_voltage -
        (total_range / 2.0f);


    ratio =
        (
            sample_voltage -
            min_voltage
        ) / total_range;


    if (ratio <= 0.0f)
    {
        return 0U;
    }


    if (ratio >= 1.0f)
    {
        return OSCOPE_ADC_MAX;
    }


    chart_value =
        ratio *
        (float)OSCOPE_ADC_MAX;


    if (chart_value < 0.0f)
    {
        chart_value = 0.0f;
    }


    if (chart_value >
        (float)OSCOPE_ADC_MAX)
    {
        chart_value =
            (float)OSCOPE_ADC_MAX;
    }


    return (uint16_t)chart_value;
}


/* ==========================================================
 * Find trigger
 * ========================================================== */

static int OscopePage_FindTrigger(
    const uint16_t *samples,
    uint16_t count
)
{
    uint16_t i;


    if (samples == NULL)
    {
        return -1;
    }


    if (count < 2U)
    {
        return -1;
    }


    for (i = 1U;
         i < count;
         i++)
    {
        /*
         * Rising edge
         */
        if (
            samples[i - 1U] <
                trigger_level_adc
            &&
            samples[i] >=
                trigger_level_adc
        )
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
        OSCOPE_ADC_MAX
    );


    lv_chart_set_update_mode(
        objects.chart_oscope,
        LV_CHART_UPDATE_MODE_SHIFT
    );


    lv_obj_set_style_size(
        objects.chart_oscope,
        0,
        LV_PART_INDICATOR
    );


    if (oscope_series == NULL)
    {
        oscope_series =
            lv_chart_add_series(
                objects.chart_oscope,
                lv_palette_main(
                    LV_PALETTE_RED
                ),
                LV_CHART_AXIS_PRIMARY_Y
            );
    }
}


/* ==========================================================
 * Create info label
 * ========================================================== */

static void OscopePage_CreateInfoLabel(void)
{
    if (oscope_info_label != NULL)
    {
        return;
    }


    if (objects.osilloscop == NULL)
    {
        return;
    }


    oscope_info_label =
        lv_label_create(
            objects.osilloscop
        );


    if (oscope_info_label == NULL)
    {
        return;
    }


    lv_obj_set_pos(
        oscope_info_label,
        16,
        12
    );


    lv_obj_set_size(
        oscope_info_label,
        285,
        LV_SIZE_CONTENT
    );


    lv_obj_set_style_text_color(
        oscope_info_label,
        lv_color_white(),
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    lv_obj_set_style_text_font(
        oscope_info_label,
        &lv_font_montserrat_12,
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    lv_label_set_text(
        oscope_info_label,
        "ADC:---- MIN:---- MAX:----"
    );
}


/* ==========================================================
 * Update info label
 * ========================================================== */

static void OscopePage_UpdateInfoLabel(void)
{
    uint16_t latest;
    uint16_t min_value;
    uint16_t max_value;

    uint16_t i;

    uint32_t trigger_millivolts;

    char text[128];

    const char *run_text;


    if (oscope_info_label == NULL)
    {
        return;
    }


    latest = 0U;

    min_value =
        OSCOPE_ADC_MAX;

    max_value = 0U;


    if (oscope_last_count > 0U)
    {
        latest =
            oscope_samples[
                oscope_last_count - 1U
            ];


        for (i = 0U;
             i < oscope_last_count;
             i++)
        {
            if (oscope_samples[i] <
                min_value)
            {
                min_value =
                    oscope_samples[i];
            }


            if (oscope_samples[i] >
                max_value)
            {
                max_value =
                    oscope_samples[i];
            }
        }
    }
    else
    {
        min_value = 0U;
        max_value = 0U;
    }


    trigger_millivolts =
        (
            (uint32_t)trigger_level_adc *
            3300UL
        ) / OSCOPE_ADC_MAX;


    if (oscope_running)
    {
        run_text = "RUN";
    }
    else
    {
        run_text = "STOP";
    }


    snprintf(
        text,
        sizeof(text),

        "ADC:%4u MIN:%4u MAX:%4u\n"
        "T:%luus V:%s TR:%lumV %s",

        latest,
        min_value,
        max_value,

        (unsigned long)
            time_div_us[
                time_div_index
            ],

        volt_div_labels[
            volt_div_index
        ],

        (unsigned long)
            trigger_millivolts,

        run_text
    );


    lv_label_set_text(
        oscope_info_label,
        text
    );
}


/* ==========================================================
 * STOP / RUN button label
 * ========================================================== */

static void OscopePage_UpdateRunStopButton(void)
{
    lv_obj_t *label;


    if (objects.stop_run_btn == NULL)
    {
        return;
    }


    label =
        lv_obj_get_child(
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
 * Draw waveform
 * ========================================================== */

static void OscopePage_DrawSamples(
    const uint16_t *samples,
    uint16_t count
)
{
    uint16_t visible_count;

    uint16_t display_index;

    uint16_t source_index;

    uint16_t chart_value;

    int trigger_index;

    int start_index;

    int maximum_start;


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


    if (oscope_series == NULL)
    {
        OscopePage_ConfigureChart();
    }


    if (oscope_series == NULL)
    {
        return;
    }


    visible_count =
        OscopePage_GetVisibleSampleCount();


    if (visible_count > count)
    {
        visible_count = count;
    }


    /*
     * اگر Trigger نداریم،
     * آخرین window داده را نشان بده.
     */
    maximum_start =
        (int)count -
        (int)visible_count;


    if (maximum_start < 0)
    {
        maximum_start = 0;
    }


    start_index =
        maximum_start;


    /*
     * پیدا کردن rising edge
     */
    trigger_index =
        OscopePage_FindTrigger(
            samples,
            count
        );


    if (trigger_index >= 0)
    {
        int pretrigger_samples;


        pretrigger_samples =
            (
                (int)visible_count *
                OSCOPE_TRIGGER_POSITION
            ) / 100;


        start_index =
            trigger_index -
            pretrigger_samples;


        if (start_index < 0)
        {
            start_index = 0;
        }


        if (start_index >
            maximum_start)
        {
            start_index =
                maximum_start;
        }
    }


    /*
     * نمودار قبلی پاک شود.
     */
    lv_chart_set_all_value(
        objects.chart_oscope,
        oscope_series,
        0
    );


    /*
     * همیشه دقیقاً 300 نقطه برای LVGL تولید می‌کنیم.
     *
     * اگر source کمتر باشد، sampleها کشیده می‌شوند.
     * اگر source بیشتر باشد، decimation انجام می‌شود.
     */
    for (
        display_index = 0U;
        display_index < OSCOPE_POINT_COUNT;
        display_index++
    )
    {
        uint32_t source_offset;


        if (OSCOPE_POINT_COUNT <= 1U)
        {
            source_offset = 0U;
        }
        else
        {
            source_offset =
                (
                    (uint32_t)display_index *
                    (uint32_t)(visible_count - 1U)
                ) /
                (uint32_t)(OSCOPE_POINT_COUNT - 1U);
        }


        source_index =
            (uint16_t)(
                start_index +
                (int)source_offset
            );


        if (source_index >= count)
        {
            source_index =
                count - 1U;
        }


        chart_value =
            OscopePage_AdcToChart(
                samples[source_index]
            );


        /*
         * همان روش موفق قبلی:
         *
         * set_next_value
         */
        lv_chart_set_next_value(
            objects.chart_oscope,
            oscope_series,
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
 * Draw current block
 * ========================================================== */

static void OscopePage_DrawLastSamples(void)
{
    if (oscope_last_count == 0U)
    {
        OscopePage_UpdateInfoLabel();

        return;
    }


    OscopePage_DrawSamples(
        oscope_samples,
        oscope_last_count
    );


    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * Timer callback
 * ========================================================== */

static void OscopePage_TimerCallback(
    lv_timer_t *timer
)
{
    bool received;


    (void)timer;


    if (!oscope_running)
    {
        return;
    }


    received =
        OscopeADC_GetLatestBlock(
            oscope_samples,
            OSCOPE_ADC_BLOCK_SIZE
        );


    if (!received)
    {
        return;
    }


    oscope_last_count =
        OSCOPE_ADC_BLOCK_SIZE;


    OscopePage_DrawSamples(
        oscope_samples,
        oscope_last_count
    );


    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * ENTER
 * ========================================================== */

void OscopePage_OnEnter(void)
{
    /*
     * Default settings
     */
    time_div_index = 2U;

    volt_div_index = 2U;

    trigger_level_adc = 2048U;

    oscope_last_count = 0U;


    /*
     * Chart
     */
    OscopePage_ConfigureChart();


    if (oscope_series != NULL)
    {
        lv_chart_set_all_value(
            objects.chart_oscope,
            oscope_series,
            0
        );


        lv_chart_refresh(
            objects.chart_oscope
        );
    }


    /*
     * Info label
     */
    OscopePage_CreateInfoLabel();


    /*
     * Timer قبلی
     */
    if (oscope_timer != NULL)
    {
        lv_timer_del(
            oscope_timer
        );

        oscope_timer = NULL;
    }


    /*
     * شروع ADC + DMA + TIM2
     */
    if (OscopeADC_Start() == HAL_OK)
    {
        oscope_running = true;
    }
    else
    {
        oscope_running = false;
    }


    OscopePage_UpdateRunStopButton();

    OscopePage_UpdateInfoLabel();


    /*
     * UI refresh = 20ms
     */
    oscope_timer =
        lv_timer_create(
            OscopePage_TimerCallback,
            20,
            NULL
        );
}


/* ==========================================================
 * EXIT
 * ========================================================== */

void OscopePage_OnExit(void)
{
    OscopeADC_Stop();


    if (oscope_timer != NULL)
    {
        lv_timer_del(
            oscope_timer
        );

        oscope_timer = NULL;
    }


    if (oscope_info_label != NULL)
    {
        lv_obj_del(
            oscope_info_label
        );

        oscope_info_label = NULL;
    }


    oscope_running = false;
}


/* ==========================================================
 * TIME +
 * ========================================================== */

void OscopePage_TimeIncrease(void)
{
    if (time_div_index <
        TIME_DIV_COUNT - 1U)
    {
        time_div_index++;
    }


    OscopePage_DrawLastSamples();
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


    OscopePage_DrawLastSamples();
}


/* ==========================================================
 * VOLT +
 * ========================================================== */

void OscopePage_VoltIncrease(void)
{
    if (volt_div_index <
        VOLT_DIV_COUNT - 1U)
    {
        volt_div_index++;
    }


    OscopePage_DrawLastSamples();
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


    OscopePage_DrawLastSamples();
}


/* ==========================================================
 * TRIGGER +
 * ========================================================== */

void OscopePage_TriggerIncrease(void)
{
    uint32_t value;


    value =
        (uint32_t)trigger_level_adc +
        TRIGGER_STEP_ADC;


    if (value > TRIGGER_MAX_ADC)
    {
        value = TRIGGER_MAX_ADC;
    }


    trigger_level_adc =
        (uint16_t)value;


    OscopePage_DrawLastSamples();
}


/* ==========================================================
 * TRIGGER -
 * ========================================================== */

void OscopePage_TriggerDecrease(void)
{
    int32_t value;


    value =
        (int32_t)trigger_level_adc -
        (int32_t)TRIGGER_STEP_ADC;


    if (value < TRIGGER_MIN_ADC)
    {
        value = TRIGGER_MIN_ADC;
    }


    trigger_level_adc =
        (uint16_t)value;


    OscopePage_DrawLastSamples();
}


/* ==========================================================
 * RUN / STOP
 * ========================================================== */

void OscopePage_ToggleRunStop(void)
{
    HAL_StatusTypeDef status;


    if (oscope_running)
    {
        /*
         * STOP
         */
        status =
            OscopeADC_Stop();


        (void)status;


        oscope_running = false;
    }
    else
    {
        /*
         * RUN
         */
        status =
            OscopeADC_Start();


        if (status == HAL_OK)
        {
            oscope_running = true;
        }
    }


    OscopePage_UpdateRunStopButton();

    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * Compatibility
 * ========================================================== */

void OscopePage_IncreaseSpeed(void)
{
    OscopePage_TimeIncrease();
}


void OscopePage_DecreaseSpeed(void)
{
    OscopePage_TimeDecrease();
}
