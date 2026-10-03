#include "OscopePage.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "lvgl.h"

#include "ui/ui.h"
#include "ui/screens.h"

#include "OscopeADC.h"


/* ==========================================================
 * General configuration
 * ========================================================== */

#define OSCOPE_ADC_MAX                  4095U
#define OSCOPE_ADC_REFERENCE            3.3f

#define OSCOPE_HORIZONTAL_DIVS          10U
#define OSCOPE_VERTICAL_DIVS             8U

#define OSCOPE_MAX_CHART_POINTS        300U

/*
 * Trigger position in the visible waveform.
 *
 * 25% = trigger approximately one quarter
 * from the left side.
 */
#define OSCOPE_TRIGGER_POSITION         25U

/*
 * Trigger hysteresis.
 */
#define OSCOPE_TRIGGER_HYSTERESIS      32U


/* ==========================================================
 * Chart geometry
 *
 * From EEZ screen.c:
 *
 * chart x = 11
 * chart y = 9
 * chart w = 299
 * chart h = 164
 * ========================================================== */

#define OSCOPE_CHART_X                  11
#define OSCOPE_CHART_Y                   9
#define OSCOPE_CHART_WIDTH            299
#define OSCOPE_CHART_HEIGHT           164


/* ==========================================================
 * Time / Division
 * ========================================================== */

static const uint32_t oscope_time_div_us[] =
{
    20U,
    50U,
    100U,
    200U
};

#define OSCOPE_TIME_DIV_COUNT \
    (sizeof(oscope_time_div_us) / \
     sizeof(oscope_time_div_us[0]))

static uint32_t oscope_time_index = 2U;


/* ==========================================================
 * Volt / Division
 * ========================================================== */

static const float oscope_volt_div_values[] =
{
    0.1f,
    0.2f,
    0.5f,
    1.0f,
    2.0f
};

static const char *oscope_volt_div_labels[] =
{
    "0.1V",
    "0.2V",
    "0.5V",
    "1.0V",
    "2.0V"
};

#define OSCOPE_VOLT_DIV_COUNT \
    (sizeof(oscope_volt_div_values) / \
     sizeof(oscope_volt_div_values[0]))

static uint32_t oscope_volt_index = 2U;


/* ==========================================================
 * Trigger
 * ========================================================== */

#define OSCOPE_TRIGGER_MIN             128U
#define OSCOPE_TRIGGER_MAX            3967U
#define OSCOPE_TRIGGER_STEP              64U

static uint16_t oscope_trigger_level = 2048U;


/* ==========================================================
 * Run / Stop
 * ========================================================== */

static bool oscope_running = false;


/* ==========================================================
 * Redraw request
 * ========================================================== */

static bool oscope_redraw_requested = false;


/* ==========================================================
 * LVGL objects
 * ========================================================== */

static lv_timer_t *oscope_timer = NULL;

static lv_obj_t *oscope_info_label = NULL;

static lv_obj_t *oscope_trigger_line = NULL;

static lv_chart_series_t *oscope_series = NULL;


/* ==========================================================
 * ADC samples
 * ========================================================== */

static uint16_t oscope_samples[
    OSCOPE_ADC_BLOCK_SIZE
];

static uint16_t oscope_last_count = 0U;


/* ==========================================================
 * Chart state
 * ========================================================== */

static uint16_t last_chart_point_count = 0U;


/* ==========================================================
 * Forward declarations
 * ========================================================== */

static void OscopePage_ConfigureChart(void);

static void OscopePage_CreateInfoLabel(void);

static void OscopePage_CreateTriggerLine(void);

static void OscopePage_UpdateTriggerLine(void);

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
 * Time/Div -> source sample count
 * ========================================================== */

static uint16_t OscopePage_GetVisibleSampleCount(void)
{
    uint32_t sample_rate;
    uint32_t total_time_us;
    uint32_t samples;


    sample_rate =
        OscopeADC_GetSampleRate();


    total_time_us =
        oscope_time_div_us[
            oscope_time_index
        ] *
        OSCOPE_HORIZONTAL_DIVS;


    samples =
        (
            sample_rate *
            total_time_us
        ) /
        1000000UL;


    if (samples < 2U)
    {
        samples = 2U;
    }


    if (
        samples >
        OSCOPE_ADC_BLOCK_SIZE
    )
    {
        samples =
            OSCOPE_ADC_BLOCK_SIZE;
    }


    return (uint16_t)samples;
}


/* ==========================================================
 * ADC -> Chart value
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
        OSCOPE_ADC_REFERENCE / 2.0f;


    total_range =
        oscope_volt_div_values[
            oscope_volt_index
        ] *
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
        ) /
        total_range;


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


    if (
        chart_value >
        (float)OSCOPE_ADC_MAX
    )
    {
        chart_value =
            (float)OSCOPE_ADC_MAX;
    }


    return (uint16_t)chart_value;
}


/* ==========================================================
 * Find rising trigger
 * ========================================================== */

static int OscopePage_FindTrigger(
    const uint16_t *samples,
    uint16_t count
)
{
    uint16_t i;

    uint16_t low_level;
    uint16_t high_level;


    if (samples == NULL)
    {
        return -1;
    }


    if (count < 2U)
    {
        return -1;
    }


    /*
     * Hysteresis thresholds.
     */
    if (
        oscope_trigger_level >
        OSCOPE_TRIGGER_HYSTERESIS
    )
    {
        low_level =
            oscope_trigger_level -
            OSCOPE_TRIGGER_HYSTERESIS;
    }
    else
    {
        low_level = 0U;
    }


    if (
        oscope_trigger_level <
        (
            OSCOPE_ADC_MAX -
            OSCOPE_TRIGGER_HYSTERESIS
        )
    )
    {
        high_level =
            oscope_trigger_level +
            OSCOPE_TRIGGER_HYSTERESIS;
    }
    else
    {
        high_level =
            OSCOPE_ADC_MAX;
    }


    /*
     * Rising edge.
     */
    for (
        i = 1U;
        i < count;
        i++
    )
    {
        if (
            samples[i - 1U] <= low_level &&
            samples[i] >= high_level
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


    /*
     * Remove point markers.
     */
    lv_obj_set_style_size(
        objects.chart_oscope,
        0,
        LV_PART_INDICATOR
    );


    /*
     * Waveform line width.
     */
    lv_obj_set_style_line_width(
        objects.chart_oscope,
        2,
        LV_PART_ITEMS
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
 * Create information label
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
 * Create Trigger line
 * ========================================================== */

static void OscopePage_CreateTriggerLine(void)
{
    static lv_point_t points[2];


    if (oscope_trigger_line != NULL)
    {
        return;
    }


    if (objects.osilloscop == NULL)
    {
        return;
    }


    /*
     * Create a line as a sibling of the chart.
     *
     * It will sit exactly over the chart area.
     */
    oscope_trigger_line =
        lv_line_create(
            objects.osilloscop
        );


    if (oscope_trigger_line == NULL)
    {
        return;
    }


    points[0].x = 0;
    points[0].y = 0;

    points[1].x =
        OSCOPE_CHART_WIDTH - 1;

    points[1].y = 0;


    lv_line_set_points(
        oscope_trigger_line,
        points,
        2
    );


    /*
     * Trigger line style.
     */
    lv_obj_set_style_line_width(
        oscope_trigger_line,
        1,
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    lv_obj_set_style_line_color(
        oscope_trigger_line,
        lv_palette_main(
            LV_PALETTE_YELLOW
        ),
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    lv_obj_set_style_line_opa(
        oscope_trigger_line,
        LV_OPA_80,
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    /*
     * Exact chart position.
     */
    lv_obj_set_pos(
        oscope_trigger_line,
        OSCOPE_CHART_X,
        OSCOPE_CHART_Y
    );


    lv_obj_set_size(
        oscope_trigger_line,
        OSCOPE_CHART_WIDTH,
        OSCOPE_CHART_HEIGHT
    );


    OscopePage_UpdateTriggerLine();
}


/* ==========================================================
 * Update Trigger line position
 * ========================================================== */

static void OscopePage_UpdateTriggerLine(void)
{
    static lv_point_t points[2];

    uint16_t chart_value;

    int32_t y;


    if (oscope_trigger_line == NULL)
    {
        return;
    }


    /*
     * Use exactly the same vertical scaling
     * as waveform.
     */
    chart_value =
        OscopePage_AdcToChart(
            oscope_trigger_level
        );


    /*
     * LVGL chart:
     *
     * maximum value = top
     * minimum value = bottom
     */
    y =
        (
            (int32_t)(OSCOPE_ADC_MAX -
                      chart_value) *
            (OSCOPE_CHART_HEIGHT - 1)
        ) /
        OSCOPE_ADC_MAX;


    if (y < 0)
    {
        y = 0;
    }


    if (
        y >
        (OSCOPE_CHART_HEIGHT - 1)
    )
    {
        y =
            OSCOPE_CHART_HEIGHT - 1;
    }


    points[0].x = 0;
    points[0].y = (lv_coord_t)y;

    points[1].x =
        OSCOPE_CHART_WIDTH - 1;

    points[1].y = (lv_coord_t)y;


    lv_line_set_points(
        oscope_trigger_line,
        points,
        2
    );


    lv_obj_invalidate(
        oscope_trigger_line
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

    uint32_t sample_rate;

    uint16_t visible_samples;

    uint32_t visible_time_us;

    char text[160];

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


        for (
            i = 0U;
            i < oscope_last_count;
            i++
        )
        {
            if (
                oscope_samples[i] <
                min_value
            )
            {
                min_value =
                    oscope_samples[i];
            }


            if (
                oscope_samples[i] >
                max_value
            )
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
            (uint32_t)
            oscope_trigger_level *
            3300UL
        ) /
        OSCOPE_ADC_MAX;


    sample_rate =
        OscopeADC_GetSampleRate();


    visible_samples =
        OscopePage_GetVisibleSampleCount();


    if (sample_rate > 0U)
    {
        visible_time_us =
            (
                (uint32_t)
                visible_samples *
                1000000UL
            ) /
            sample_rate;
    }
    else
    {
        visible_time_us = 0U;
    }


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
        "T:%luus V:%s TR:%lumV %s\n"
        "%luS/s %luus",

        latest,
        min_value,
        max_value,

        (unsigned long)
        oscope_time_div_us[
            oscope_time_index
        ],

        oscope_volt_div_labels[
            oscope_volt_index
        ],

        (unsigned long)
        trigger_millivolts,

        run_text,

        (unsigned long)
        sample_rate,

        (unsigned long)
        visible_time_us
    );


    lv_label_set_text(
        oscope_info_label,
        text
    );
}


/* ==========================================================
 * Update RUN / STOP button
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
    uint16_t display_count;

    uint16_t display_index;
    uint16_t source_index;

    uint16_t chart_value;

    int trigger_index;

    int start_index;

    int maximum_start;

    int pretrigger_samples;

    uint32_t source_offset;


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


    /*
     * Exact number of ADC samples corresponding
     * to selected Time/Div.
     */
    visible_count =
        OscopePage_GetVisibleSampleCount();


    if (visible_count > count)
    {
        visible_count =
            count;
    }


    if (visible_count < 2U)
    {
        visible_count = 2U;
    }


    /*
     * Trigger.
     */
    trigger_index =
        OscopePage_FindTrigger(
            samples,
            count
        );


    pretrigger_samples =
        (
            (int)visible_count *
            OSCOPE_TRIGGER_POSITION
        ) / 100;


    maximum_start =
        (int)count -
        (int)visible_count;


    if (maximum_start < 0)
    {
        maximum_start = 0;
    }


    /*
     * Default to latest window.
     */
    start_index =
        maximum_start;


    /*
     * If trigger exists, center the visible window
     * around the trigger position.
     */
    if (trigger_index >= 0)
    {
        start_index =
            trigger_index -
            pretrigger_samples;


        if (start_index < 0)
        {
            start_index = 0;
        }


        if (
            start_index >
            maximum_start
        )
        {
            start_index =
                maximum_start;
        }
    }


    /*
     * Do not create more points than
     * necessary for the display.
     */
    display_count =
        visible_count;


    if (
        display_count >
        OSCOPE_MAX_CHART_POINTS
    )
    {
        display_count =
            OSCOPE_MAX_CHART_POINTS;
    }


    /*
     * Change point count only when necessary.
     */
    if (
        last_chart_point_count !=
        display_count
    )
    {
        lv_chart_set_point_count(
            objects.chart_oscope,
            display_count
        );


        last_chart_point_count =
            display_count;
    }


    /*
     * Clear previous waveform.
     */
    lv_chart_set_all_value(
        objects.chart_oscope,
        oscope_series,
        0
    );


    /*
     * Draw.
     */
    for (
        display_index = 0U;
        display_index < display_count;
        display_index++
    )
    {
        if (display_count <= 1U)
        {
            source_offset = 0U;
        }
        else
        {
            source_offset =
                (
                    (uint32_t)
                    display_index *
                    (uint32_t)
                    (visible_count - 1U)
                ) /
                (uint32_t)
                (display_count - 1U);
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
         * Known-good LVGL function.
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


    /*
     * Trigger line must remain above the chart.
     */
    OscopePage_UpdateTriggerLine();
}


/* ==========================================================
 * Draw latest block
 * ========================================================== */

static void OscopePage_DrawLastSamples(void)
{
    if (oscope_last_count == 0U)
    {
        return;
    }


    OscopePage_DrawSamples(
        oscope_samples,
        oscope_last_count
    );
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


    /*
     * Get newest completed DMA block.
     */
    received =
        OscopeADC_GetLatestBlock(
            oscope_samples,
            OSCOPE_ADC_BLOCK_SIZE
        );


    if (received)
    {
        oscope_last_count =
            OscopeADC_GetBlockSize();


        /*
         * New data has arrived.
         */
        oscope_redraw_requested =
            true;
    }


    /*
     * Redraw only when needed.
     */
    if (
        oscope_redraw_requested &&
        oscope_last_count > 0U
    )
    {
        OscopePage_DrawSamples(
            oscope_samples,
            oscope_last_count
        );


        oscope_redraw_requested =
            false;
    }


    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * Page Enter
 * ========================================================== */

void OscopePage_OnEnter(void)
{
    /*
     * Defaults
     */
    oscope_time_index = 2U;

    oscope_volt_index = 2U;

    oscope_trigger_level =
        2048U;


    oscope_last_count = 0U;

    oscope_redraw_requested =
        false;

    last_chart_point_count =
        0U;


    /*
     * Configure chart.
     */
    OscopePage_ConfigureChart();


    /*
     * Create information label.
     */
    OscopePage_CreateInfoLabel();


    /*
     * Create Trigger line.
     */
    OscopePage_CreateTriggerLine();


    /*
     * Ensure initial Trigger position is correct.
     */
    OscopePage_UpdateTriggerLine();


    /*
     * Delete old timer if necessary.
     */
    if (oscope_timer != NULL)
    {
        lv_timer_del(
            oscope_timer
        );

        oscope_timer = NULL;
    }


    /*
     * Start ADC + DMA + TIM2.
     */
    if (
        OscopeADC_Start() ==
        HAL_OK
    )
    {
        oscope_running =
            true;
    }
    else
    {
        oscope_running =
            false;
    }


    OscopePage_UpdateRunStopButton();

    OscopePage_UpdateInfoLabel();


    /*
     * UI refresh.
     */
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
    /*
     * Stop acquisition.
     */
    OscopeADC_Stop();


    /*
     * Delete timer.
     */
    if (oscope_timer != NULL)
    {
        lv_timer_del(
            oscope_timer
        );

        oscope_timer = NULL;
    }


    /*
     * Delete information label.
     */
    if (oscope_info_label != NULL)
    {
        lv_obj_del(
            oscope_info_label
        );

        oscope_info_label = NULL;
    }


    /*
     * Delete Trigger line.
     */
    if (oscope_trigger_line != NULL)
    {
        lv_obj_del(
            oscope_trigger_line
        );

        oscope_trigger_line = NULL;
    }


    oscope_running = false;
}


/* ==========================================================
 * TIME +
 * ========================================================== */

void OscopePage_TimeIncrease(void)
{
    if (
        oscope_time_index <
        (OSCOPE_TIME_DIV_COUNT - 1U)
    )
    {
        oscope_time_index++;
    }


    /*
     * Only request redraw.
     * Do not redraw inside button callback.
     */
    oscope_redraw_requested =
        true;


    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * TIME -
 * ========================================================== */

void OscopePage_TimeDecrease(void)
{
    if (oscope_time_index > 0U)
    {
        oscope_time_index--;
    }


    oscope_redraw_requested =
        true;


    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * VOLT +
 * ========================================================== */

void OscopePage_VoltIncrease(void)
{
    if (
        oscope_volt_index <
        (OSCOPE_VOLT_DIV_COUNT - 1U)
    )
    {
        oscope_volt_index++;
    }


    /*
     * Trigger line moves together with vertical scaling.
     */
    OscopePage_UpdateTriggerLine();


    oscope_redraw_requested =
        true;


    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * VOLT -
 * ========================================================== */

void OscopePage_VoltDecrease(void)
{
    if (oscope_volt_index > 0U)
    {
        oscope_volt_index--;
    }


    OscopePage_UpdateTriggerLine();


    oscope_redraw_requested =
        true;


    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * TRIGGER +
 * ========================================================== */

void OscopePage_TriggerIncrease(void)
{
    uint32_t new_level;


    new_level =
        (uint32_t)
        oscope_trigger_level +
        OSCOPE_TRIGGER_STEP;


    if (
        new_level >
        OSCOPE_TRIGGER_MAX
    )
    {
        new_level =
            OSCOPE_TRIGGER_MAX;
    }


    oscope_trigger_level =
        (uint16_t)new_level;


    /*
     * Move visible Trigger line immediately.
     */
    OscopePage_UpdateTriggerLine();


    /*
     * Redraw waveform around the new trigger
     * on the next timer cycle.
     */
    oscope_redraw_requested =
        true;


    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * TRIGGER -
 * ========================================================== */

void OscopePage_TriggerDecrease(void)
{
    int32_t new_level;


    new_level =
        (int32_t)
        oscope_trigger_level -
        (int32_t)
        OSCOPE_TRIGGER_STEP;


    if (
        new_level <
        OSCOPE_TRIGGER_MIN
    )
    {
        new_level =
            OSCOPE_TRIGGER_MIN;
    }


    oscope_trigger_level =
        (uint16_t)new_level;


    OscopePage_UpdateTriggerLine();


    oscope_redraw_requested =
        true;


    OscopePage_UpdateInfoLabel();
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


        oscope_running =
            false;
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
            oscope_running =
                true;

            oscope_redraw_requested =
                true;
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
