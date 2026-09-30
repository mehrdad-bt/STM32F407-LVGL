#include "OscopeADC.h"

#include <string.h>


/* -------------------------------------------------------------------------- */
/* External CubeMX handles                                                    */
/* -------------------------------------------------------------------------- */

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;


/* -------------------------------------------------------------------------- */
/* DMA buffer                                                                 */
/* -------------------------------------------------------------------------- */

/*
 * ADC1 -> DMA2 -> this buffer
 *
 * uint16_t because ADC resolution is 12-bit.
 */
static uint16_t adc_dma_buffer[OSCOPE_ADC_BUFFER_SIZE];


/*
 * Which half of the DMA buffer is safe to read.
 *
 * 0 = first half
 * 1 = second half
 */
static volatile uint8_t latest_ready_half = 0U;


/*
 * ADC running state.
 */
static volatile uint8_t adc_running = 0U;


/* -------------------------------------------------------------------------- */
/* Start                                                                      */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef OscopeADC_Start(void)
{
    HAL_StatusTypeDef status;


    /*
     * Start ADC in DMA mode first.
     *
     * ADC waits for TIM2 TRGO because ADC external trigger
     * is configured in CubeMX.
     */
    status = HAL_ADC_Start_DMA(
        &hadc1,
        (uint32_t *)adc_dma_buffer,
        OSCOPE_ADC_BUFFER_SIZE
    );

    if (status != HAL_OK)
    {
        adc_running = 0U;
        return status;
    }


    /*
     * Reset state.
     */
    latest_ready_half = 0U;
    adc_running = 1U;


    /*
     * Start TIM2.
     *
     * TIM2 generates TRGO on update event.
     */
    status = HAL_TIM_Base_Start(&htim2);

    if (status != HAL_OK)
    {
        HAL_ADC_Stop_DMA(&hadc1);
        adc_running = 0U;

        return status;
    }


    return HAL_OK;
}


/* -------------------------------------------------------------------------- */
/* Stop                                                                       */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef OscopeADC_Stop(void)
{
    HAL_StatusTypeDef timer_status;
    HAL_StatusTypeDef adc_status;


    /*
     * Stop timer first so ADC no longer receives triggers.
     */
    timer_status = HAL_TIM_Base_Stop(&htim2);


    /*
     * Stop ADC + DMA.
     */
    adc_status = HAL_ADC_Stop_DMA(&hadc1);


    adc_running = 0U;


    if (timer_status != HAL_OK)
    {
        return timer_status;
    }


    return adc_status;
}


/* -------------------------------------------------------------------------- */
/* DMA half complete callback                                                  */
/* -------------------------------------------------------------------------- */

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1)
    {
        /*
         * First half has just been filled.
         *
         * DMA is now writing the second half,
         * so the first half is stable and can be read.
         */
        latest_ready_half = 0U;
    }
}


/* -------------------------------------------------------------------------- */
/* DMA complete callback                                                       */
/* -------------------------------------------------------------------------- */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1)
    {
        /*
         * Second half has just been filled.
         *
         * DMA is now back at the first half,
         * so the second half is stable and can be read.
         */
        latest_ready_half = 1U;
    }
}


/* -------------------------------------------------------------------------- */
/* Get latest completed block                                                 */
/* -------------------------------------------------------------------------- */

const uint16_t *OscopeADC_GetLatestBlock(void)
{
    if (latest_ready_half == 0U)
    {
        return &adc_dma_buffer[0];
    }

    return &adc_dma_buffer[OSCOPE_ADC_HALF_SIZE];
}


/* -------------------------------------------------------------------------- */
/* Get block size                                                              */
/* -------------------------------------------------------------------------- */

uint16_t OscopeADC_GetBlockSize(void)
{
    return OSCOPE_ADC_HALF_SIZE;
}


/* -------------------------------------------------------------------------- */
/* Get latest half                                                             */
/* -------------------------------------------------------------------------- */

uint8_t OscopeADC_GetLatestHalf(void)
{
    return latest_ready_half;
}
