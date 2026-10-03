#include "OscopePage.h"

#include "lvgl.h"

#include "ui/ui.h"
#include "ui/screens.h"

#include "OscopeADC.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


/* ============================================================
 * DISPLAY
 * ============================================================ */

#define OSCOPE_DISPLAY_FPS             60U

/*
 * 60 FPS is approximately 16.67 ms.
 * LVGL timer uses integer milliseconds.
 */
#define OSCOPE_DISPLAY_PERIOD_MS       16U


/* ============================================================
 * CHART
 * ============================================================ */

#define OSCOPE_CHART_WIDTH             299U
#define OSCOPE_CHART_HEIGHT            164U

#define OSCOPE_POINTS                  299U

#define OSCOPE_ADC_MIN                 0U
#define OSCOPE_ADC_MAX                 4095U
#define OSCOPE_ADC_CENTER              2048U


/* ============================================================
 * FREQUENCY MEASUREMENT
 * ============================================================ */

/*
 * Minimum peak-to-peak amplitude (ADC counts) required
 * before a frequency is reported. Below this the signal is
 * considered noise / DC and "F:---" is shown.
 *
 * 100 counts is about 80 mV at 3.3 V full scale.
 */
#define FREQ_MIN_PP_COUNTS             100U

/*
 * Schmitt-trigger hysteresis = peak-to-peak / FREQ_HYST_DIVISOR
 * on each side of the mid level.
 */
#define FREQ_HYST_DIVISOR              8U


/* ============================================================
 * TIME / DIV
 * ============================================================ */

static const uint32_t time_div_us_table[] =
{
    1U,
    2U,
    5U,
    10U,
    20U,
    50U,
    100U,
    200U,
    500U,
    1000U,
    1500U
};

#define TIME_DIV_COUNT \
    (sizeof(time_div_us_table) / \
     sizeof(time_div_us_table[0]))


/* ============================================================
 * VOLT / DIV
 * ============================================================ */

static const float volt_div_table[] =
{
    0.1f,
    0.2f,
    0.5f,
    1.0f,
    2.0f
};

#define VOLT_DIV_COUNT \
    (sizeof(volt_div_table) / \
     sizeof(volt_div_table[0]))


/* ============================================================
 * SWEEP SPEED
 * ============================================================ */

/*
 * Index 0 = slowest
 * Index 8 = fastest
 */
static const uint32_t sweep_duration_ms_table[] =
{
    4000U,
    2500U,
    1500U,
    1000U,
    500U,
    250U,
    125U,
    64U,
    16U
};

#define SWEEP_SPEED_COUNT \
    (sizeof(sweep_duration_ms_table) / \
     sizeof(sweep_duration_ms_table[0]))

/*
 * Start from slowest.
 */
static uint32_t sweep_speed_index = 0U;


/* ============================================================
 * COLORS
 * ============================================================ */

/*
 * Waveform = yellow
 */
#define OSCOPE_WAVE_COLOR              0xFFFF00U

/*
 * Trigger = blue
 */
#define OSCOPE_TRIGGER_COLOR           0x0000FFU

/*
 * Information box
 */
#define OSCOPE_INFO_BG_COLOR           0x101820U
#define OSCOPE_INFO_BORDER_COLOR       0x5A6A7AU

/*
 * Information text
 */
#define OSCOPE_TIME_COLOR              0x66E0FFU
#define OSCOPE_VOLT_COLOR              0xA8FF66U
#define OSCOPE_TRIGGER_TEXT_COLOR      0x6699FFU
#define OSCOPE_SWEEP_COLOR             0xFFAA44U
#define OSCOPE_STATUS_COLOR            0xFFFFFFU
#define OSCOPE_FREQ_COLOR              0xFF66FFU


/* ============================================================
 * INFO FONT
 * ============================================================ */

#if LV_FONT_MONTSERRAT_10

#define OSCOPE_INFO_FONT \
    (&lv_font_montserrat_10)

#elif LV_FONT_MONTSERRAT_12

#define OSCOPE_INFO_FONT \
    (&lv_font_montserrat_12)

#else

#define OSCOPE_INFO_FONT \
    LV_FONT_DEFAULT

#endif


/* ============================================================
 * TRIGGER
 * ============================================================ */

#define TRIGGER_STEP                   64U
#define TRIGGER_MIN                    128U
#define TRIGGER_MAX                    3967U
#define TRIGGER_HYSTERESIS             32U
#define TRIGGER_PRE_PERCENT            30U


/* ============================================================
 * RUNTIME STATE
 * ============================================================ */

static bool oscope_active = false;

static bool oscope_running = false;


/*
 * Time/Div index.
 */
static uint32_t time_div_index = 4U;


/*
 * Volt/Div index.
 */
static uint32_t volt_div_index = 2U;


/*
 * Trigger level.
 */
static uint16_t trigger_level =
    OSCOPE_ADC_CENTER;


/* ============================================================
 * ADC DATA
 * ============================================================ */

/*
 * Safe DMA block.
 */
static uint16_t latest_block[
    OSCOPE_ADC_BLOCK_SIZE
];


/*
 * Frozen waveform used for the current visual sweep.
 */
static uint16_t sweep_min_values[
    OSCOPE_POINTS
];

static uint16_t sweep_max_values[
    OSCOPE_POINTS
];


/*
 * Waveform statistics.
 */
static uint16_t sweep_global_min = 0U;

static uint16_t sweep_global_max = 0U;


/* ============================================================
 * FREQUENCY RESULT
 * ============================================================ */

/*
 * Last measured frequency in Hz.
 * Valid only when measured_freq_valid == true.
 */
static uint32_t measured_freq_hz = 0U;

static bool measured_freq_valid = false;


/* ============================================================
 * CHART SERIES
 * ============================================================ */

static lv_chart_series_t *oscope_min_series =
    NULL;

static lv_chart_series_t *oscope_max_series =
    NULL;


/* ============================================================
 * CUSTOM UI
 * ============================================================ */

static lv_obj_t *trigger_line =
    NULL;


/*
 * Information box inside the chart.
 */
static lv_obj_t *oscope_info_box =
    NULL;


/*
 * Information labels.
 */
static lv_obj_t *oscope_time_label =
    NULL;

static lv_obj_t *oscope_volt_label =
    NULL;

static lv_obj_t *oscope_trigger_label =
    NULL;

static lv_obj_t *oscope_sweep_label =
    NULL;

static lv_obj_t *oscope_status_label =
    NULL;

static lv_obj_t *oscope_freq_label =
    NULL;


/* ============================================================
 * DISPLAY TIMER
 * ============================================================ */

static lv_timer_t *oscope_display_timer =
    NULL;


/* ============================================================
 * SWEEP STATE
 * ============================================================ */

static bool sweep_active =
    false;


static uint32_t sweep_drawn_points =
    0U;


static uint32_t sweep_points_per_frame =
    1U;


static uint32_t sweep_total_frames =
    1U;


static uint32_t sweep_frame_counter =
    0U;


/* ============================================================
 * SCALE ADC FOR VOLT/DIV
 * ============================================================ */

static uint16_t scale_adc_for_display(
    uint16_t adc_value
)
{
    float volt_div;
    float half_range_v;

    float input_v;
    float center_v;

    float scaled;


    volt_div =
        volt_div_table[
            volt_div_index
        ];


    /*
     * Four vertical divisions above/below center.
     */
    half_range_v =
        volt_div *
        4.0f;


    /*
     * ADC -> voltage.
     */
    input_v =
        (
            (float)adc_value *
            3.3f
        )
        /
        4095.0f;


    /*
     * Screen center = 1.65 V.
     */
    center_v =
        1.65f;


    /*
     * Convert into chart coordinate 0..4095.
     */
    scaled =
        (
            (
                input_v -
                center_v
            )
            /
            half_range_v
        )
        *
        2047.5f
        +
        2047.5f;


    if (
        scaled <
        0.0f
    )
    {
        scaled =
            0.0f;
    }


    if (
        scaled >
        4095.0f
    )
    {
        scaled =
            4095.0f;
    }


    return
        (uint16_t)scaled;
}


/* ============================================================
 * TIME WINDOW
 * ============================================================ */

static uint32_t get_view_sample_count(void)
{
    uint32_t total_time_us;

    uint64_t sample_count;


    /*
     * 10 horizontal divisions.
     */
    total_time_us =
        time_div_us_table[
            time_div_index
        ]
        *
        10U;


    /*
     * samples = time x sample rate.
     */
    sample_count =
        (
            (uint64_t)total_time_us *
            (uint64_t)
            OscopeADC_GetSampleRate()
        )
        /
        1000000ULL;


    if (
        sample_count <
        2ULL
    )
    {
        sample_count =
            2ULL;
    }


    /*
     * One oscilloscope block contains OSCOPE_ADC_BLOCK_SIZE samples.
     */
    if (
        sample_count >
        OSCOPE_ADC_BLOCK_SIZE
    )
    {
        sample_count =
            OSCOPE_ADC_BLOCK_SIZE;
    }


    return
        (uint32_t)sample_count;
}


/* ============================================================
 * TRIGGER SEARCH
 * ============================================================ */

static uint32_t find_trigger_index(
    const uint16_t *samples,
    uint32_t sample_count
)
{
    uint32_t i;

    uint16_t level;

    uint16_t low_level;


    if (
        samples == NULL
    )
    {
        return 0U;
    }


    if (
        sample_count <
        2U
    )
    {
        return 0U;
    }


    level =
        trigger_level;


    /*
     * Hysteresis.
     */
    if (
        level >
        TRIGGER_HYSTERESIS
    )
    {
        low_level =
            level -
            TRIGGER_HYSTERESIS;
    }
    else
    {
        low_level =
            0U;
    }


    /*
     * Rising edge trigger.
     */
    for (
        i = 1U;
        i < sample_count;
        i++
    )
    {
        if (
            samples[i - 1U] <=
            low_level
            &&
            samples[i] >=
            level
        )
        {
            return i;
        }
    }


    /*
     * No trigger found.
     */
    return
        sample_count / 2U;
}


/* ============================================================
 * FREQUENCY MEASUREMENT
 *
 * Method:
 *   1. Find min / max of the whole ADC block.
 *   2. If peak-to-peak is too small -> no signal.
 *   3. Mid level = (min + max) / 2.
 *      Schmitt trigger with hysteresis = pp / 8.
 *   4. Detect every rising crossing of the upper threshold.
 *      Each crossing position is linearly interpolated
 *      between two samples (sub-sample accuracy).
 *   5. frequency = (crossings - 1) * fs
 *                  / (last_position - first_position)
 *
 * Using many periods gives high accuracy.
 * Minimum measurable frequency is about 2 / 16.384 ms = 122 Hz
 * (two rising edges must fall inside one block).
 * ============================================================ */

static bool measure_frequency(
    const uint16_t *samples,
    uint32_t sample_count,
    uint32_t *freq_hz
)
{
    uint32_t i;

    uint16_t vmin;
    uint16_t vmax;

    uint32_t pp;
    uint32_t mid;
    uint32_t hyst;
    uint32_t high_th;
    uint32_t low_th;

    bool state_high;

    uint32_t crossings;

    float first_pos;
    float last_pos;
    float pos;

    float span;
    float freq;


    if (
        samples == NULL ||
        freq_hz == NULL ||
        sample_count < 4U
    )
    {
        return false;
    }


    /*
     * Min / max.
     */
    vmin =
        samples[0];

    vmax =
        samples[0];


    for (
        i = 1U;
        i < sample_count;
        i++
    )
    {
        if (
            samples[i] <
            vmin
        )
        {
            vmin =
                samples[i];
        }


        if (
            samples[i] >
            vmax
        )
        {
            vmax =
                samples[i];
        }
    }


    pp =
        (uint32_t)vmax -
        (uint32_t)vmin;


    if (
        pp <
        FREQ_MIN_PP_COUNTS
    )
    {
        return false;
    }


    mid =
        ((uint32_t)vmax +
         (uint32_t)vmin) / 2U;


    hyst =
        pp / FREQ_HYST_DIVISOR;


    if (
        hyst < 1U
    )
    {
        hyst = 1U;
    }


    high_th =
        mid + hyst;

    low_th =
        mid - hyst;


    /*
     * Initial Schmitt state from first sample.
     */
    state_high =
        (samples[0] >= mid);


    crossings =
        0U;

    first_pos =
        0.0f;

    last_pos =
        0.0f;


    for (
        i = 1U;
        i < sample_count;
        i++
    )
    {
        uint32_t s0;
        uint32_t s1;


        s0 =
            samples[i - 1U];

        s1 =
            samples[i];


        if (
            state_high
        )
        {
            if (
                s1 <= low_th
            )
            {
                state_high =
                    false;
            }
        }
        else
        {
            if (
                s1 >= high_th
            )
            {
                state_high =
                    true;


                /*
                 * Rising crossing of high_th,
                 * interpolated between sample i-1 and i.
                 * If s0 is already >= high_th (slow rise after
                 * hysteresis), clamp fraction to 0.
                 */
                if (
                    s1 > s0 &&
                    high_th > s0
                )
                {
                    pos =
                        (float)(i - 1U) +
                        (
                            (float)(high_th - s0) /
                            (float)(s1 - s0)
                        );
                }
                else
                {
                    pos =
                        (float)i;
                }


                if (
                    crossings == 0U
                )
                {
                    first_pos =
                        pos;
                }


                last_pos =
                    pos;


                crossings++;
            }
        }
    }


    /*
     * Need at least two rising edges (one full period).
     */
    if (
        crossings < 2U
    )
    {
        return false;
    }


    span =
        last_pos -
        first_pos;


    if (
        span <= 0.0f
    )
    {
        return false;
    }


    freq =
        (
            (float)(crossings - 1U) *
            (float)OscopeADC_GetSampleRate()
        )
        /
        span;


    if (
        freq < 1.0f
    )
    {
        return false;
    }


    *freq_hz =
        (uint32_t)(freq + 0.5f);


    return true;
}


/* ============================================================
 * CHART CONFIGURATION
 * ============================================================ */

static void configure_chart(void)
{
    if (
        objects.chart_oscope == NULL
    )
    {
        return;
    }


    /*
     * Line chart.
     */
    lv_chart_set_type(
        objects.chart_oscope,
        LV_CHART_TYPE_LINE
    );


    /*
     * One point per horizontal pixel.
     */
    lv_chart_set_point_count(
        objects.chart_oscope,
        OSCOPE_POINTS
    );


    /*
     * Internal range.
     */
    lv_chart_set_range(
        objects.chart_oscope,
        LV_CHART_AXIS_PRIMARY_Y,
        OSCOPE_ADC_MIN,
        OSCOPE_ADC_MAX
    );


    /*
     * Create the two envelope series only once.
     */
    if (
        oscope_min_series == NULL
    )
    {
        oscope_min_series =
            lv_chart_add_series(
                objects.chart_oscope,
                lv_color_hex(
                    OSCOPE_WAVE_COLOR
                ),
                LV_CHART_AXIS_PRIMARY_Y
            );
    }


    if (
        oscope_max_series == NULL
    )
    {
        oscope_max_series =
            lv_chart_add_series(
                objects.chart_oscope,
                lv_color_hex(
                    OSCOPE_WAVE_COLOR
                ),
                LV_CHART_AXIS_PRIMARY_Y
            );
    }


    /*
     * We manually update point arrays.
     */
    lv_chart_set_update_mode(
        objects.chart_oscope,
        LV_CHART_UPDATE_MODE_SHIFT
    );
}


/* ============================================================
 * CLEAR CHART
 * ============================================================ */

static void clear_chart(void)
{
    uint32_t i;


    if (
        oscope_min_series != NULL
    )
    {
        for (
            i = 0U;
            i < OSCOPE_POINTS;
            i++
        )
        {
            oscope_min_series
                ->y_points[i] =
                LV_CHART_POINT_NONE;
        }
    }


    if (
        oscope_max_series != NULL
    )
    {
        for (
            i = 0U;
            i < OSCOPE_POINTS;
            i++
        )
        {
            oscope_max_series
                ->y_points[i] =
                LV_CHART_POINT_NONE;
        }
    }


    if (
        objects.chart_oscope != NULL
    )
    {
        lv_chart_refresh(
            objects.chart_oscope
        );
    }
}


/* ============================================================
 * TRIGGER LINE
 * ============================================================ */

static void create_trigger_line(void)
{
    if (
        objects.osilloscop == NULL
    )
    {
        return;
    }


    if (
        trigger_line != NULL
    )
    {
        return;
    }


    trigger_line =
        lv_obj_create(
            objects.osilloscop
        );


    if (
        trigger_line == NULL
    )
    {
        return;
    }


    /*
     * Full chart width.
     */
    lv_obj_set_size(
        trigger_line,
        OSCOPE_CHART_WIDTH,
        2
    );


    /*
     * Same origin as chart.
     */
    lv_obj_set_pos(
        trigger_line,
        11,
        9
    );


    /*
     * BLUE trigger.
     */
    lv_obj_set_style_bg_color(
        trigger_line,
        lv_color_hex(
            OSCOPE_TRIGGER_COLOR
        ),
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    lv_obj_set_style_bg_opa(
        trigger_line,
        LV_OPA_COVER,
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    lv_obj_set_style_border_width(
        trigger_line,
        0,
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    lv_obj_set_style_radius(
        trigger_line,
        0,
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    lv_obj_clear_flag(
        trigger_line,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_clear_flag(
        trigger_line,
        LV_OBJ_FLAG_CLICKABLE
    );
}


/* ============================================================
 * UPDATE TRIGGER LINE
 * ============================================================ */

static void update_trigger_line(void)
{
    uint16_t scaled_adc;

    uint32_t y;


    if (
        trigger_line == NULL
    )
    {
        return;
    }


    /*
     * Trigger must follow Volt/Div.
     */
    scaled_adc =
        scale_adc_for_display(
            trigger_level
        );


    y =
        (
            (uint32_t)scaled_adc *
            (OSCOPE_CHART_HEIGHT - 1U)
        )
        /
        OSCOPE_ADC_MAX;


    if (
        y >=
        OSCOPE_CHART_HEIGHT
    )
    {
        y =
            OSCOPE_CHART_HEIGHT - 1U;
    }


    /*
     * Convert to screen Y.
     */
    y =
        (
            OSCOPE_CHART_HEIGHT - 1U
        )
        -
        y;


    lv_obj_set_y(
        trigger_line,
        9 +
        (lv_coord_t)y
    );
}


/* ============================================================
 * HELPER: CREATE ONE INFO LABEL
 * ============================================================ */

static lv_obj_t *create_info_label(
    lv_obj_t *parent,
    lv_coord_t x,
    lv_coord_t y,
    lv_coord_t w,
    lv_coord_t h,
    uint32_t color,
    const char *initial_text
)
{
    lv_obj_t *label;


    label =
        lv_label_create(
            parent
        );


    if (
        label == NULL
    )
    {
        return NULL;
    }


    lv_obj_set_pos(
        label,
        x,
        y
    );


    lv_obj_set_size(
        label,
        w,
        h
    );


    lv_obj_set_style_text_font(
        label,
        OSCOPE_INFO_FONT,
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    lv_obj_set_style_text_color(
        label,
        lv_color_hex(
            color
        ),
        LV_PART_MAIN |
        LV_STATE_DEFAULT
    );


    lv_label_set_text(
        label,
        initial_text
    );


    return label;
}


/* ============================================================
 * CREATE INFO BOX
 * ============================================================ */

static void create_info_box(void)
{
    if (
        objects.osilloscop == NULL
    )
    {
        return;
    }


    /* --------------------------------------------------------
     * Information box
     * -------------------------------------------------------- */

    if (
        oscope_info_box == NULL
    )
    {
        oscope_info_box =
            lv_obj_create(
                objects.osilloscop
            );


        if (
            oscope_info_box == NULL
        )
        {
            return;
        }


        /*
         * Box is placed inside chart,
         * upper-right corner.
         */
        lv_obj_set_pos(
            oscope_info_box,
            178,
            14
        );


        lv_obj_set_size(
            oscope_info_box,
            124,
            48
        );


        /*
         * No internal padding, so children coordinates
         * start exactly at the box corner and nothing
         * is clipped.
         */
        lv_obj_set_style_pad_all(
            oscope_info_box,
            0,
            LV_PART_MAIN |
            LV_STATE_DEFAULT
        );


        /*
         * Dark background.
         */
        lv_obj_set_style_bg_color(
            oscope_info_box,
            lv_color_hex(
                OSCOPE_INFO_BG_COLOR
            ),
            LV_PART_MAIN |
            LV_STATE_DEFAULT
        );


        lv_obj_set_style_bg_opa(
            oscope_info_box,
            LV_OPA_80,
            LV_PART_MAIN |
            LV_STATE_DEFAULT
        );


        /*
         * Border.
         */
        lv_obj_set_style_border_color(
            oscope_info_box,
            lv_color_hex(
                OSCOPE_INFO_BORDER_COLOR
            ),
            LV_PART_MAIN |
            LV_STATE_DEFAULT
        );


        lv_obj_set_style_border_width(
            oscope_info_box,
            1,
            LV_PART_MAIN |
            LV_STATE_DEFAULT
        );


        lv_obj_set_style_radius(
            oscope_info_box,
            3,
            LV_PART_MAIN |
            LV_STATE_DEFAULT
        );


        /*
         * Do not scroll.
         */
        lv_obj_clear_flag(
            oscope_info_box,
            LV_OBJ_FLAG_SCROLLABLE
        );


        lv_obj_clear_flag(
            oscope_info_box,
            LV_OBJ_FLAG_CLICKABLE
        );
    }


    /* --------------------------------------------------------
     * TIME
     * -------------------------------------------------------- */

    if (
        oscope_time_label == NULL
    )
    {
        oscope_time_label =
            create_info_label(
                oscope_info_box,
                3, 0, 58, 12,
                OSCOPE_TIME_COLOR,
                "TIME"
            );
    }


    /* --------------------------------------------------------
     * VOLT
     * -------------------------------------------------------- */

    if (
        oscope_volt_label == NULL
    )
    {
        oscope_volt_label =
            create_info_label(
                oscope_info_box,
                63, 0, 61, 12,
                OSCOPE_VOLT_COLOR,
                "VOLT"
            );
    }


    /* --------------------------------------------------------
     * TRIGGER
     * -------------------------------------------------------- */

    if (
        oscope_trigger_label == NULL
    )
    {
        oscope_trigger_label =
            create_info_label(
                oscope_info_box,
                3, 12, 58, 12,
                OSCOPE_TRIGGER_TEXT_COLOR,
                "TRIG"
            );
    }


    /* --------------------------------------------------------
     * SWEEP
     * -------------------------------------------------------- */

    if (
        oscope_sweep_label == NULL
    )
    {
        oscope_sweep_label =
            create_info_label(
                oscope_info_box,
                63, 12, 58, 12,
                OSCOPE_SWEEP_COLOR,
                "SW"
            );
    }


    /* --------------------------------------------------------
     * STATUS
     * -------------------------------------------------------- */

    if (
        oscope_status_label == NULL
    )
    {
        oscope_status_label =
            create_info_label(
                oscope_info_box,
                3, 24, 40, 12,
                OSCOPE_STATUS_COLOR,
                "STOP"
            );
    }


    /* --------------------------------------------------------
     * FREQUENCY
     * -------------------------------------------------------- */

    if (
        oscope_freq_label == NULL
    )
    {
        oscope_freq_label =
            create_info_label(
                oscope_info_box,
                45, 24, 76, 12,
                OSCOPE_FREQ_COLOR,
                "F:---"
            );
    }
}


/* ============================================================
 * UPDATE INFO BOX
 * ============================================================ */

static void update_info_box(void)
{
    char text[32];

    uint32_t time_div;

    uint32_t volt_div_x10;

    uint32_t trigger_mv;

    uint32_t sweep_ms;


    if (
        oscope_info_box == NULL
    )
    {
        return;
    }


    time_div =
        time_div_us_table[
            time_div_index
        ];


    /*
     * Store Volt/Div as tenths of a volt.
     * Avoid %f because newlib-nano in many STM32 builds
     * is linked without floating-point printf support.
     */
    volt_div_x10 =
        (uint32_t)(
            volt_div_table[
                volt_div_index
            ]
            *
            10.0f
        );


    trigger_mv =
        (
            (uint32_t)
            trigger_level
            *
            3300U
        )
        /
        4095U;


    sweep_ms =
        sweep_duration_ms_table[
            sweep_speed_index
        ];


    /* --------------------------------------------------------
     * TIME
     * -------------------------------------------------------- */

    if (
        oscope_time_label != NULL
    )
    {
        snprintf(
            text,
            sizeof(text),
            "T:%luus/d",
            (unsigned long)time_div
        );


        lv_label_set_text(
            oscope_time_label,
            text
        );
    }


    /* --------------------------------------------------------
     * VOLT
     * -------------------------------------------------------- */

    if (
        oscope_volt_label != NULL
    )
    {
        snprintf(
            text,
            sizeof(text),
            "V/d:%lu.%luV",
            (unsigned long)(volt_div_x10 / 10U),
            (unsigned long)(volt_div_x10 % 10U)
        );


        lv_label_set_text(
            oscope_volt_label,
            text
        );
    }


    /* --------------------------------------------------------
     * TRIGGER
     * -------------------------------------------------------- */

    if (
        oscope_trigger_label != NULL
    )
    {
        snprintf(
            text,
            sizeof(text),
            "TR:%lumV",
            (unsigned long)trigger_mv
        );


        lv_label_set_text(
            oscope_trigger_label,
            text
        );
    }


    /* --------------------------------------------------------
     * SWEEP
     *
     * Integer math only (no %f).
     * -------------------------------------------------------- */

    if (
        oscope_sweep_label != NULL
    )
    {
        if (
            sweep_ms >=
            1000U
        )
        {
            snprintf(
                text,
                sizeof(text),
                "SW:%lu.%lus",
                (unsigned long)(sweep_ms / 1000U),
                (unsigned long)((sweep_ms % 1000U) / 100U)
            );
        }
        else
        {
            snprintf(
                text,
                sizeof(text),
                "SW:%lums",
                (unsigned long)sweep_ms
            );
        }


        lv_label_set_text(
            oscope_sweep_label,
            text
        );
    }


    /* --------------------------------------------------------
     * STATUS
     * -------------------------------------------------------- */

    if (
        oscope_status_label != NULL
    )
    {
        lv_label_set_text(
            oscope_status_label,
            oscope_running
                ? "RUN"
                : "STOP"
        );


        lv_obj_set_style_text_color(
            oscope_status_label,
            oscope_running
                ? lv_color_hex(0x66FF66)
                : lv_color_hex(0xFFFFFF),
            LV_PART_MAIN |
            LV_STATE_DEFAULT
        );
    }


    /* --------------------------------------------------------
     * FREQUENCY
     *
     * < 1 kHz      -> "F:123Hz"
     * 1k .. 1 MHz  -> "F:12.34kHz"
     * Integer math only.
     * -------------------------------------------------------- */

    if (
        oscope_freq_label != NULL
    )
    {
        if (
            !measured_freq_valid
        )
        {
            snprintf(
                text,
                sizeof(text),
                "F:---"
            );
        }
        else if (
            measured_freq_hz <
            1000U
        )
        {
            snprintf(
                text,
                sizeof(text),
                "F:%luHz",
                (unsigned long)measured_freq_hz
            );
        }
        else if (
            measured_freq_hz <
            1000000U
        )
        {
            snprintf(
                text,
                sizeof(text),
                "F:%lu.%02lukHz",
                (unsigned long)(measured_freq_hz / 1000U),
                (unsigned long)((measured_freq_hz % 1000U) / 10U)
            );
        }
        else
        {
            snprintf(
                text,
                sizeof(text),
                "F:%lu.%02luMHz",
                (unsigned long)(measured_freq_hz / 1000000U),
                (unsigned long)((measured_freq_hz % 1000000U) / 10000U)
            );
        }


        lv_label_set_text(
            oscope_freq_label,
            text
        );
    }
}


/* ============================================================
 * CONFIGURE SWEEP TIMING
 * ============================================================ */

static void configure_sweep_timing(void)
{
    uint32_t duration_ms;

    uint32_t frames;


    /*
     * Sweep duration is independent from Time/Div.
     */
    duration_ms =
        sweep_duration_ms_table[
            sweep_speed_index
        ];


    /*
     * Convert duration to number of display frames.
     */
    frames =
        (
            duration_ms
            +
            OSCOPE_DISPLAY_PERIOD_MS
            -
            1U
        )
        /
        OSCOPE_DISPLAY_PERIOD_MS;


    if (
        frames <
        1U
    )
    {
        frames =
            1U;
    }


    sweep_total_frames =
        frames;


    /*
     * Spread 299 points over all frames.
     */
    sweep_points_per_frame =
        (
            OSCOPE_POINTS
            +
            frames
            -
            1U
        )
        /
        frames;


    if (
        sweep_points_per_frame <
        1U
    )
    {
        sweep_points_per_frame =
            1U;
    }
}


/* ============================================================
 * PREPARE NEW SWEEP
 * ============================================================ */

static bool prepare_new_sweep(void)
{
    uint32_t view_sample_count;

    uint32_t trigger_index;

    uint32_t pre_trigger;

    uint32_t start_index;

    uint32_t point;

    uint32_t i;

    uint32_t freq_hz;


    /*
     * Get newest safe DMA block.
     */
    if (
        !OscopeADC_GetLatestBlock(
            latest_block,
            OSCOPE_ADC_BLOCK_SIZE
        )
    )
    {
        return false;
    }


    /*
     * Frequency is measured from the WHOLE block,
     * independent of Time/Div and Volt/Div.
     */
    if (
        measure_frequency(
            latest_block,
            OSCOPE_ADC_BLOCK_SIZE,
            &freq_hz
        )
    )
    {
        measured_freq_hz =
            freq_hz;

        measured_freq_valid =
            true;
    }
    else
    {
        measured_freq_hz =
            0U;

        measured_freq_valid =
            false;
    }


    /*
     * Time/Div -> number of samples.
     */
    view_sample_count =
        get_view_sample_count();


    /*
     * Find trigger.
     */
    trigger_index =
        find_trigger_index(
            latest_block,
            OSCOPE_ADC_BLOCK_SIZE
        );


    /*
     * 30% before trigger.
     */
    pre_trigger =
        (
            view_sample_count
            *
            TRIGGER_PRE_PERCENT
        )
        /
        100U;


    if (
        trigger_index >
        pre_trigger
    )
    {
        start_index =
            trigger_index
            -
            pre_trigger;
    }
    else
    {
        start_index =
            0U;
    }


    /*
     * Stay inside DMA block.
     */
    if (
        start_index
        +
        view_sample_count
        >
        OSCOPE_ADC_BLOCK_SIZE
    )
    {
        start_index =
            OSCOPE_ADC_BLOCK_SIZE
            -
            view_sample_count;
    }


    sweep_global_min =
        4095U;


    sweep_global_max =
        0U;


    /* ========================================================
     * MORE ADC SAMPLES THAN SCREEN POINTS
     *
     * Min/Max Envelope
     * ======================================================== */

    if (
        view_sample_count >=
        OSCOPE_POINTS
    )
    {
        for (
            point = 0U;
            point < OSCOPE_POINTS;
            point++
        )
        {
            uint32_t bin_start;

            uint32_t bin_end;

            uint16_t bin_min =
                4095U;

            uint16_t bin_max =
                0U;


            bin_start =
                (
                    point
                    *
                    view_sample_count
                )
                /
                OSCOPE_POINTS;


            bin_end =
                (
                    (point + 1U)
                    *
                    view_sample_count
                )
                /
                OSCOPE_POINTS;


            if (
                bin_end <=
                bin_start
            )
            {
                bin_end =
                    bin_start + 1U;
            }


            if (
                bin_end >
                view_sample_count
            )
            {
                bin_end =
                    view_sample_count;
            }


            /*
             * Find minimum and maximum inside bin.
             */
            for (
                i = bin_start;
                i < bin_end;
                i++
            )
            {
                uint16_t value =
                    latest_block[
                        start_index
                        +
                        i
                    ];


                if (
                    value <
                    bin_min
                )
                {
                    bin_min =
                        value;
                }


                if (
                    value >
                    bin_max
                )
                {
                    bin_max =
                        value;
                }
            }


            /*
             * Apply Volt/Div scaling.
             */
            sweep_min_values[point] =
                scale_adc_for_display(
                    bin_min
                );


            sweep_max_values[point] =
                scale_adc_for_display(
                    bin_max
                );


            /*
             * Statistics.
             */
            if (
                bin_min <
                sweep_global_min
            )
            {
                sweep_global_min =
                    bin_min;
            }


            if (
                bin_max >
                sweep_global_max
            )
            {
                sweep_global_max =
                    bin_max;
            }
        }
    }


    /* ========================================================
     * FEWER ADC SAMPLES THAN SCREEN POINTS
     *
     * Stretch samples.
     * ======================================================== */

    else
    {
        for (
            point = 0U;
            point < OSCOPE_POINTS;
            point++
        )
        {
            uint32_t source_index;

            uint16_t value;


            source_index =
                (
                    point
                    *
                    (
                        view_sample_count
                        -
                        1U
                    )
                )
                /
                (
                    OSCOPE_POINTS
                    -
                    1U
                );


            value =
                latest_block[
                    start_index
                    +
                    source_index
                ];


            sweep_min_values[point] =
                scale_adc_for_display(
                    value
                );


            sweep_max_values[point] =
                scale_adc_for_display(
                    value
                );


            if (
                value <
                sweep_global_min
            )
            {
                sweep_global_min =
                    value;
            }


            if (
                value >
                sweep_global_max
            )
            {
                sweep_global_max =
                    value;
            }
        }
    }


    /*
     * Configure sweep.
     */
    configure_sweep_timing();


    /*
     * Reset progressive state.
     */
    sweep_drawn_points =
        0U;


    sweep_frame_counter =
        0U;


    sweep_active =
        true;


    /*
     * Start with an empty chart.
     */
    clear_chart();


    /*
     * Update info (includes frequency).
     */
    update_info_box();


    return true;
}


/* ============================================================
 * DRAW ONE SWEEP CHUNK
 * ============================================================ */

static void draw_sweep_chunk(void)
{
    uint32_t start_point;

    uint32_t end_point;

    uint32_t point;


    if (
        !sweep_active
    )
    {
        return;
    }


    start_point =
        sweep_drawn_points;


    end_point =
        start_point
        +
        sweep_points_per_frame;


    if (
        end_point >
        OSCOPE_POINTS
    )
    {
        end_point =
            OSCOPE_POINTS;
    }


    /*
     * Draw only this frame's section.
     */
    for (
        point = start_point;
        point < end_point;
        point++
    )
    {
        oscope_min_series
            ->y_points[point] =
            sweep_min_values[point];


        oscope_max_series
            ->y_points[point] =
            sweep_max_values[point];
    }


    /*
     * One chart refresh.
     */
    lv_chart_refresh(
        objects.chart_oscope
    );


    sweep_drawn_points =
        end_point;


    sweep_frame_counter++;


    /*
     * End of sweep.
     */
    if (
        sweep_drawn_points >=
        OSCOPE_POINTS
    )
    {
        sweep_active =
            false;
    }
}


/* ============================================================
 * DISPLAY TIMER CALLBACK
 * ============================================================ */

static void oscilloscope_display_timer_cb(
    lv_timer_t *timer
)
{
    (void)timer;


    if (
        !oscope_active
    )
    {
        return;
    }


    if (
        !oscope_running
    )
    {
        return;
    }


    /*
     * Start a fresh waveform when
     * the previous sweep is finished.
     */
    if (
        !sweep_active
    )
    {
        if (
            !prepare_new_sweep()
        )
        {
            return;
        }
    }


    /*
     * Draw next section.
     */
    draw_sweep_chunk();
}


/* ============================================================
 * RESET SWEEP
 * ============================================================ */

static void reset_sweep(void)
{
    sweep_active =
        false;


    sweep_drawn_points =
        0U;


    sweep_frame_counter =
        0U;


    update_info_box();


    /*
     * Execute timer as soon as possible.
     */
    if (
        oscope_display_timer != NULL
    )
    {
        lv_timer_ready(
            oscope_display_timer
        );
    }
}


/* ============================================================
 * ENTER PAGE
 * ============================================================ */

void OscopePage_OnEnter(void)
{
    if (
        objects.osilloscop == NULL ||
        objects.chart_oscope == NULL
    )
    {
        return;
    }


    oscope_active =
        true;


    /*
     * Reset frequency display.
     */
    measured_freq_hz =
        0U;

    measured_freq_valid =
        false;


    /*
     * Chart.
     */
    configure_chart();


    /*
     * Trigger.
     */
    create_trigger_line();


    /*
     * Information box.
     */
    create_info_box();


    /*
     * Trigger position.
     */
    update_trigger_line();


    /*
     * ADC/DMA.
     */
    if (
        !oscope_running
    )
    {
        if (
            OscopeADC_Start() ==
            HAL_OK
        )
        {
            oscope_running =
                true;
        }
    }


    /*
     * Create dedicated 60 FPS timer.
     */
    if (
        oscope_display_timer ==
        NULL
    )
    {
        oscope_display_timer =
            lv_timer_create(
                oscilloscope_display_timer_cb,
                OSCOPE_DISPLAY_PERIOD_MS,
                NULL
            );
    }
    else
    {
        lv_timer_set_period(
            oscope_display_timer,
            OSCOPE_DISPLAY_PERIOD_MS
        );
    }


    /*
     * Start first sweep.
     */
    reset_sweep();


    update_info_box();
}


/* ============================================================
 * EXIT PAGE
 * ============================================================ */

void OscopePage_OnExit(void)
{
    oscope_active =
        false;


    /*
     * Delete display timer.
     */
    if (
        oscope_display_timer != NULL
    )
    {
        lv_timer_del(
            oscope_display_timer
        );


        oscope_display_timer =
            NULL;
    }


    /*
     * Stop ADC/DMA.
     */
    if (
        oscope_running
    )
    {
        OscopeADC_Stop();


        oscope_running =
            false;
    }


    sweep_active =
        false;


    sweep_drawn_points =
        0U;
}


/* ============================================================
 * TIME +
 * ============================================================ */

void OscopePage_TimeIncrease(void)
{
    if (
        time_div_index + 1U <
        TIME_DIV_COUNT
    )
    {
        time_div_index++;
    }


    reset_sweep();


    update_trigger_line();
}


/* ============================================================
 * TIME -
 * ============================================================ */

void OscopePage_TimeDecrease(void)
{
    if (
        time_div_index >
        0U
    )
    {
        time_div_index--;
    }


    reset_sweep();


    update_trigger_line();
}


/* ============================================================
 * VOLT +
 * ============================================================ */

void OscopePage_VoltIncrease(void)
{
    if (
        volt_div_index + 1U <
        VOLT_DIV_COUNT
    )
    {
        volt_div_index++;
    }


    reset_sweep();


    update_trigger_line();
}


/* ============================================================
 * VOLT -
 * ============================================================ */

void OscopePage_VoltDecrease(void)
{
    if (
        volt_div_index >
        0U
    )
    {
        volt_div_index--;
    }


    reset_sweep();


    update_trigger_line();
}


/* ============================================================
 * TRIGGER +
 * ============================================================ */

void OscopePage_TriggerIncrease(void)
{
    uint32_t new_level;


    new_level =
        (uint32_t)
        trigger_level
        +
        TRIGGER_STEP;


    if (
        new_level >
        TRIGGER_MAX
    )
    {
        new_level =
            TRIGGER_MAX;
    }


    trigger_level =
        (uint16_t)new_level;


    update_trigger_line();


    reset_sweep();
}


/* ============================================================
 * TRIGGER -
 * ============================================================ */

void OscopePage_TriggerDecrease(void)
{
    uint32_t new_level;


    if (
        trigger_level >
        TRIGGER_STEP
    )
    {
        new_level =
            (uint32_t)
            trigger_level
            -
            TRIGGER_STEP;
    }
    else
    {
        new_level =
            0U;
    }


    if (
        new_level <
        TRIGGER_MIN
    )
    {
        new_level =
            TRIGGER_MIN;
    }


    trigger_level =
        (uint16_t)new_level;


    update_trigger_line();


    reset_sweep();
}


/* ============================================================
 * RUN / STOP
 * ============================================================ */

void OscopePage_ToggleRunStop(void)
{
    if (
        oscope_running
    )
    {
        OscopeADC_Stop();


        oscope_running =
            false;


        sweep_active =
            false;
    }
    else
    {
        if (
            OscopeADC_Start() ==
            HAL_OK
        )
        {
            oscope_running =
                true;


            reset_sweep();
        }
    }


    update_info_box();
}


/* ============================================================
 * SWEEP +
 *
 * Faster
 * ============================================================ */

void OscopePage_SweepIncrease(void)
{
    if (
        sweep_speed_index + 1U <
        SWEEP_SPEED_COUNT
    )
    {
        sweep_speed_index++;
    }


    /*
     * Start immediately with new speed.
     */
    reset_sweep();
}


/* ============================================================
 * SWEEP -
 *
 * Slower
 * ============================================================ */

void OscopePage_SweepDecrease(void)
{
    if (
        sweep_speed_index >
        0U
    )
    {
        sweep_speed_index--;
    }


    /*
     * Start immediately with new speed.
     */
    reset_sweep();
}


/* ============================================================
 * PERIODIC TASK
 * ============================================================ */

/*
 * The dedicated LVGL timer handles the oscilloscope.
 *
 * Tasks_Run() may still call this function every 5 ms.
 */
void OscopePage_Task(void)
{
    /*
     * Intentionally empty.
     */
}
