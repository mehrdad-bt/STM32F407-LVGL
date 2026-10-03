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
    return OSCOPE_ADC_SAMPLE_RATE_HZ;
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
