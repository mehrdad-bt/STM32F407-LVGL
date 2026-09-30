#include "OscopePage.h"

#include <stdint.h>

#include "OscopeADC.h"

#include "ui/ui.h"
#include "ui/screens.h"


/* -------------------------------------------------------------------------- */
/* Configuration                                                              */
/* -------------------------------------------------------------------------- */

#define OSCOPE_POINT_COUNT              300U


/*
 * LVGL refresh period.
 *
 * This does NOT control ADC sampling.
 * ADC always runs at the rate configured by TIM2.
 *
 * It only controls how often we redraw the chart.
 */
#define OSCOPE_DISPLAY_PERIOD_MIN_MS     10U
#define OSCOPE_DISPLAY_PERIOD_MAX_MS    100U
#define OSCOPE_DISPLAY_PERIOD_STEP_MS    10U
#define OSCOPE_DISPLAY_PERIOD_DEFAULT_MS 20U


/*
 * ADC range.
 *
 * 12-bit ADC = 0 ... 4095
 */
#define OSCOPE_ADC_Y_MIN                 0
#define OSCOPE_ADC_Y_MAX              4095


/* -------------------------------------------------------------------------- */
/* Internal variables                                                         */
/* -------------------------------------------------------------------------- */

static lv_chart_series_t *oscope_series = NULL;

static lv_timer_t *oscope_timer = NULL;


/*
 * Current chart refresh period.
 *
 * Smaller = display updates faster.
 */
static uint32_t oscope_display_period_ms =
    OSCOPE_DISPLAY_PERIOD_DEFAULT_MS;


/*
 * Prevent displaying the same DMA block repeatedly.
 */
static uint8_t displayed_half = 0xFF;


/* -------------------------------------------------------------------------- */
/* Update chart from ADC                                                      */
/* -------------------------------------------------------------------------- */

static void OscopePage_UpdateChart(void)
{
    const uint16_t *adc_block;

    uint16_t block_size;

    uint16_t start_index;

    uint16_t samples_to_display;

    uint16_t i;

    uint16_t adc_value;


    if (objects.chart_oscope == NULL)
    {
        return;
    }


    if (oscope_series == NULL)
    {
        return;
    }


    /*
     * Get latest completed DMA half.
     */
    adc_block = OscopeADC_GetLatestBlock();

    if (adc_block == NULL)
    {
        return;
    }


    block_size = OscopeADC_GetBlockSize();


    /*
     * Prevent updating the chart with the same DMA half
     * multiple times.
     */
    uint8_t current_half =
        OscopeADC_GetLatestHalf();


    if (current_half == displayed_half)
    {
        return;
    }


    displayed_half = current_half;


    /*
     * We want the most recent 300 samples
     * from the completed half-buffer.
     */
    if (block_size > OSCOPE_POINT_COUNT)
    {
        start_index =
            block_size - OSCOPE_POINT_COUNT;

        samples_to_display =
            OSCOPE_POINT_COUNT;
    }
    else
    {
        start_index = 0U;

        samples_to_display =
            block_size;
    }


    /*
     * Add ADC samples to LVGL chart.
     *
     * Because chart mode is SHIFT,
     * new samples enter from the right
     * and old samples move left.
     */
    for (i = 0U; i < samples_to_display; i++)
    {
        adc_value =
            adc_block[start_index + i];


        /*
         * Make sure value stays inside chart range.
         */
        if (adc_value > 4095U)
        {
            adc_value = 4095U;
        }


        lv_chart_set_next_value(
            objects.chart_oscope,
            oscope_series,
            (lv_coord_t)adc_value
        );
    }


    /*
     * Refresh chart.
     */
    lv_chart_refresh(
        objects.chart_oscope
    );
}


/* -------------------------------------------------------------------------- */
/* LVGL timer callback                                                        */
/* -------------------------------------------------------------------------- */

static void OscopePage_TimerCallback(lv_timer_t *timer)
{
    (void)timer;

    OscopePage_UpdateChart();
}


/* -------------------------------------------------------------------------- */
/* Initialize chart                                                           */
/* -------------------------------------------------------------------------- */

static void OscopePage_InitChart(void)
{
    if (objects.chart_oscope == NULL)
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Line chart                                                             */
    /* ---------------------------------------------------------------------- */

    lv_chart_set_type(
        objects.chart_oscope,
        LV_CHART_TYPE_LINE
    );


    /* ---------------------------------------------------------------------- */
    /* Point count                                                             */
    /* ---------------------------------------------------------------------- */

    lv_chart_set_point_count(
        objects.chart_oscope,
        OSCOPE_POINT_COUNT
    );


    /* ---------------------------------------------------------------------- */
    /* ADC raw range                                                           */
    /* ---------------------------------------------------------------------- */

    lv_chart_set_range(
        objects.chart_oscope,
        LV_CHART_AXIS_PRIMARY_Y,
        OSCOPE_ADC_Y_MIN,
        OSCOPE_ADC_Y_MAX
    );


    /* ---------------------------------------------------------------------- */
    /* Shift mode                                                             */
    /* ---------------------------------------------------------------------- */

    lv_chart_set_update_mode(
        objects.chart_oscope,
        LV_CHART_UPDATE_MODE_SHIFT
    );


    /* ---------------------------------------------------------------------- */
    /* Remove visible points                                                  */
    /* ---------------------------------------------------------------------- */

    lv_obj_set_style_size(
        objects.chart_oscope,
        0,
        LV_PART_INDICATOR
    );


    /* ---------------------------------------------------------------------- */
    /* Create ADC series                                                       */
    /* ---------------------------------------------------------------------- */

    if (oscope_series == NULL)
    {
        oscope_series =
            lv_chart_add_series(
                objects.chart_oscope,
                lv_palette_main(LV_PALETTE_RED),
                LV_CHART_AXIS_PRIMARY_Y
            );
    }


    /* ---------------------------------------------------------------------- */
    /* Clear chart                                                             */
    /* ---------------------------------------------------------------------- */

    if (oscope_series != NULL)
    {
        lv_chart_set_all_value(
            objects.chart_oscope,
            oscope_series,
            0
        );
    }


    /* ---------------------------------------------------------------------- */
    /* Reset state                                                             */
    /* ---------------------------------------------------------------------- */

    displayed_half = 0xFF;


    oscope_display_period_ms =
        OSCOPE_DISPLAY_PERIOD_DEFAULT_MS;


    /* ---------------------------------------------------------------------- */
    /* Refresh                                                                 */
    /* ---------------------------------------------------------------------- */

    lv_chart_refresh(
        objects.chart_oscope
    );
}


/* -------------------------------------------------------------------------- */
/* Enter Oscilloscope                                                         */
/* -------------------------------------------------------------------------- */

void OscopePage_OnEnter(void)
{
    HAL_StatusTypeDef adc_status;


    /*
     * Delete existing UI timer.
     */
    if (oscope_timer != NULL)
    {
        lv_timer_del(oscope_timer);
        oscope_timer = NULL;
    }


    /*
     * Initialize chart.
     */
    OscopePage_InitChart();


    /*
     * Start ADC + DMA + TIM2.
     */
    adc_status = OscopeADC_Start();


    if (adc_status != HAL_OK)
    {
        /*
         * ADC failed to start.
         *
         * Leave chart empty for now.
         */
        return;
    }


    /*
     * Start LVGL refresh timer.
     */
    oscope_timer =
        lv_timer_create(
            OscopePage_TimerCallback,
            oscope_display_period_ms,
            NULL
        );
}


/* -------------------------------------------------------------------------- */
/* Exit Oscilloscope                                                          */
/* -------------------------------------------------------------------------- */

void OscopePage_OnExit(void)
{
    /*
     * Stop LVGL update timer.
     */
    if (oscope_timer != NULL)
    {
        lv_timer_del(oscope_timer);
        oscope_timer = NULL;
    }


    /*
     * Stop ADC + DMA + TIM2.
     */
    OscopeADC_Stop();


    /*
     * Reset state.
     */
    displayed_half = 0xFF;
}


/* -------------------------------------------------------------------------- */
/* Increase display update speed                                              */
/* -------------------------------------------------------------------------- */

void OscopePage_IncreaseSpeed(void)
{
    /*
     * Smaller display period = faster redraw.
     */
    if (oscope_display_period_ms >
        OSCOPE_DISPLAY_PERIOD_MIN_MS)
    {
        oscope_display_period_ms -=
            OSCOPE_DISPLAY_PERIOD_STEP_MS;


        if (oscope_display_period_ms <
            OSCOPE_DISPLAY_PERIOD_MIN_MS)
        {
            oscope_display_period_ms =
                OSCOPE_DISPLAY_PERIOD_MIN_MS;
        }
    }


    /*
     * Apply immediately.
     */
    if (oscope_timer != NULL)
    {
        lv_timer_set_period(
            oscope_timer,
            oscope_display_period_ms
        );
    }
}


/* -------------------------------------------------------------------------- */
/* Decrease display update speed                                              */
/* -------------------------------------------------------------------------- */

void OscopePage_DecreaseSpeed(void)
{
    /*
     * Larger display period = slower redraw.
     */
    if (oscope_display_period_ms <
        OSCOPE_DISPLAY_PERIOD_MAX_MS)
    {
        oscope_display_period_ms +=
            OSCOPE_DISPLAY_PERIOD_STEP_MS;


        if (oscope_display_period_ms >
            OSCOPE_DISPLAY_PERIOD_MAX_MS)
        {
            oscope_display_period_ms =
                OSCOPE_DISPLAY_PERIOD_MAX_MS;
        }
    }


    /*
     * Apply immediately.
     */
    if (oscope_timer != NULL)
    {
        lv_timer_set_period(
            oscope_timer,
            oscope_display_period_ms
        );
    }
}
