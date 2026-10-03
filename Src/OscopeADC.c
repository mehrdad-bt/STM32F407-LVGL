#include "OscopeADC.h"

#include "main.h"

#include <string.h>


/* ============================================================
 * EXTERNAL CUBE/HAL HANDLES
 * ============================================================ */

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;


/* ============================================================
 * DMA STORAGE
 * ============================================================ */

/*
 * Two complete DMA halves.
 *
 * First half is written while the second half is being consumed,
 * then the roles are exchanged.
 *
 * Memory usage:
 * 16384 samples x 2 bytes = 32768 bytes = 32 KB.
 */
static uint16_t adc_dma_buffer[
    OSCOPE_ADC_DMA_BUFFER_SIZE
];


/* ============================================================
 * RUNTIME STATE
 * ============================================================ */

static volatile bool adc_running = false;

static volatile bool latest_block_valid = false;

/*
 * Index of the most recently completed DMA half.
 *
 * 0 = samples [0 ... 8191]
 * 1 = samples [8192 ... 16383]
 */
static volatile uint32_t latest_half_index = 0U;

/*
 * Current (actual) sample rate.
 */
static uint32_t sample_rate_hz =
    OSCOPE_ADC_DEFAULT_SAMPLE_RATE_HZ;


/* ============================================================
 * TIMER CLOCK
 * ============================================================ */

/*
 * TIM2 is on APB1.
 * When the APB1 prescaler is not 1, the timer clock is 2 x PCLK1.
 * (168 MHz system: PCLK1 = 42 MHz, TIM2 = 84 MHz)
 */
static uint32_t get_timer_clock_hz(void)
{
    uint32_t clk;


    clk =
        HAL_RCC_GetPCLK1Freq();


    if (
        (RCC->CFGR & RCC_CFGR_PPRE1) !=
        RCC_CFGR_PPRE1_DIV1
    )
    {
        clk *= 2U;
    }


    return clk;
}


/* ============================================================
 * START
 * ============================================================ */

HAL_StatusTypeDef OscopeADC_Start(void)
{
    HAL_StatusTypeDef adc_status;
    HAL_StatusTypeDef timer_status;


    if (
        adc_running
    )
    {
        return HAL_OK;
    }


    /*
     * Clear previous data state.
     */
    latest_block_valid =
        false;


    latest_half_index =
        0U;


    memset(
        adc_dma_buffer,
        0,
        sizeof(adc_dma_buffer)
    );


    /*
     * Arm ADC + DMA first.
     *
     * ADC1 is externally triggered by TIM2 TRGO.
     */
    adc_status =
        HAL_ADC_Start_DMA(
            &hadc1,
            (uint32_t *)adc_dma_buffer,
            OSCOPE_ADC_DMA_BUFFER_SIZE
        );


    if (
        adc_status != HAL_OK
    )
    {
        return adc_status;
    }


    /*
     * Start from counter = 0, otherwise a counter value
     * above the new ARR would run through a full 32-bit
     * wrap-around before the first trigger.
     */
    __HAL_TIM_SET_COUNTER(
        &htim2,
        0U
    );


    /*
     * Start TIM2 last so the first ADC trigger happens only after
     * ADC + DMA are fully armed.
     */
    timer_status =
        HAL_TIM_Base_Start(
            &htim2
        );


    if (
        timer_status != HAL_OK
    )
    {
        HAL_ADC_Stop_DMA(
            &hadc1
        );


        return timer_status;
    }


    adc_running =
        true;


    return HAL_OK;
}


/* ============================================================
 * STOP
 * ============================================================ */

void OscopeADC_Stop(void)
{
    if (
        !adc_running
    )
    {
        return;
    }


    /*
     * Stop the trigger source first.
     */
    HAL_TIM_Base_Stop(
        &htim2
    );


    /*
     * Then stop ADC + DMA.
     */
    HAL_ADC_Stop_DMA(
        &hadc1
    );


    adc_running =
        false;
}


/* ============================================================
 * SET SAMPLE RATE
 * ============================================================ */

HAL_StatusTypeDef OscopeADC_SetSampleRate(
    uint32_t rate_hz
)
{
    uint32_t timer_clk;
    uint32_t period_ticks;
    bool was_running;


    if (
        rate_hz >
        OSCOPE_ADC_MAX_SAMPLE_RATE_HZ
    )
    {
        rate_hz =
            OSCOPE_ADC_MAX_SAMPLE_RATE_HZ;
    }


    if (
        rate_hz <
        OSCOPE_ADC_MIN_SAMPLE_RATE_HZ
    )
    {
        rate_hz =
            OSCOPE_ADC_MIN_SAMPLE_RATE_HZ;
    }


    timer_clk =
        get_timer_clock_hz();


    /*
     * Rounded timer period in timer ticks.
     * TIM2 is a 32-bit timer, so no prescaler is needed.
     */
    period_ticks =
        (
            timer_clk +
            (rate_hz / 2U)
        )
        /
        rate_hz;


    if (
        period_ticks <
        2U
    )
    {
        period_ticks =
            2U;
    }


    was_running =
        adc_running;


    /*
     * The timer and the ADC/DMA must be stopped while
     * the period is changed.
     */
    if (
        was_running
    )
    {
        OscopeADC_Stop();
    }


    __HAL_TIM_SET_AUTORELOAD(
        &htim2,
        period_ticks - 1U
    );


    __HAL_TIM_SET_COUNTER(
        &htim2,
        0U
    );


    htim2.Init.Prescaler =
        0U;


    htim2.Init.Period =
        period_ticks - 1U;


    /*
     * Actual rate after rounding.
     */
    sample_rate_hz =
        (
            timer_clk +
            (period_ticks / 2U)
        )
        /
        period_ticks;


    /*
     * Any stored block belongs to the old rate.
     */
    latest_block_valid =
        false;


    if (
        was_running
    )
    {
        return OscopeADC_Start();
    }


    return HAL_OK;
}


/* ============================================================
 * GET LATEST BLOCK
 * ============================================================ */

bool OscopeADC_GetLatestBlock(
    uint16_t *destination,
    uint32_t destination_size
)
{
    bool valid;
    uint32_t half_index;
    const uint16_t *source;


    if (
        destination == NULL
    )
    {
        return false;
    }


    if (
        destination_size <
        OSCOPE_ADC_BLOCK_SIZE
    )
    {
        return false;
    }


    /*
     * Read the latest completed half atomically with respect to
     * the DMA callbacks.
     *
     * The completed half is stable while DMA is filling the other
     * half, so no shadow buffer is required.
     */
    __disable_irq();


    valid =
        latest_block_valid;


    half_index =
        latest_half_index;


    if (
        half_index == 0U
    )
    {
        source =
            &adc_dma_buffer[0U];
    }
    else
    {
        source =
            &adc_dma_buffer[OSCOPE_ADC_BLOCK_SIZE];
    }


    if (
        valid
    )
    {
        memcpy(
            destination,
            source,
            OSCOPE_ADC_BLOCK_SIZE * sizeof(uint16_t)
        );
    }


    __enable_irq();


    return valid;
}


/* ============================================================
 * SAMPLE RATE
 * ============================================================ */

uint32_t OscopeADC_GetSampleRate(void)
{
    return sample_rate_hz;
}


/* ============================================================
 * HAL DMA CALLBACKS
 * ============================================================ */

void HAL_ADC_ConvHalfCpltCallback(
    ADC_HandleTypeDef *hadc
)
{
    if (
        hadc == NULL
    )
    {
        return;
    }


    if (
        hadc->Instance != ADC1
    )
    {
        return;
    }


    if (
        !adc_running
    )
    {
        return;
    }


    /*
     * First half is complete and stable.
     * DMA now continues into the second half.
     */
    latest_half_index =
        0U;


    latest_block_valid =
        true;
}


void HAL_ADC_ConvCpltCallback(
    ADC_HandleTypeDef *hadc
)
{
    if (
        hadc == NULL
    )
    {
        return;
    }


    if (
        hadc->Instance != ADC1
    )
    {
        return;
    }


    if (
        !adc_running
    )
    {
        return;
    }


    /*
     * Second half is complete and stable.
     * DMA now wraps back to the first half.
     */
    latest_half_index =
        1U;


    latest_block_valid =
        true;
}
