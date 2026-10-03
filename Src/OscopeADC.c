#include "OscopeADC.h"


/* --------------------------------------------------------------------------
 * External ADC handle
 * -------------------------------------------------------------------------- */

extern ADC_HandleTypeDef hadc1;


/* --------------------------------------------------------------------------
 * Read one continuous block of ADC samples
 * -------------------------------------------------------------------------- */

HAL_StatusTypeDef OscopeADC_ReadSamples(
    uint16_t *destination,
    uint16_t destination_size
)
{
    uint16_t i;

    HAL_StatusTypeDef status;


    /*
     * Validate destination.
     */

    if (destination == NULL)
    {
        return HAL_ERROR;
    }

    if (destination_size < OSCOPE_ADC_SAMPLE_COUNT)
    {
        return HAL_ERROR;
    }


    /*
     * Start ADC ONCE.
     *
     * ADC is configured in main.c as:
     *
     * ContinuousConvMode = ENABLE
     * Software trigger
     *
     * Therefore ADC continuously converts
     * ADC1_IN12 / PC2.
     */

    status =
        HAL_ADC_Start(&hadc1);

    if (status != HAL_OK)
    {
        return status;
    }


    /*
     * Read 300 consecutive conversions.
     *
     * We do NOT stop and restart the ADC between samples.
     *
     * This is important for waveform capture.
     */

    for (
        i = 0U;
        i < OSCOPE_ADC_SAMPLE_COUNT;
        i++
    )
    {
        /*
         * Wait for the next conversion.
         */

        status =
            HAL_ADC_PollForConversion(
                &hadc1,
                100
            );

        if (status != HAL_OK)
        {
            HAL_ADC_Stop(&hadc1);

            return status;
        }


        /*
         * Read ADC result.
         *
         * 12-bit:
         *
         * 0 ... 4095
         */

        destination[i] =
            (uint16_t)
            HAL_ADC_GetValue(&hadc1);
    }


    /*
     * Stop ADC after the complete waveform block.
     */

    HAL_ADC_Stop(&hadc1);


    return HAL_OK;
}


/* --------------------------------------------------------------------------
 * Read one ADC value
 * -------------------------------------------------------------------------- */

HAL_StatusTypeDef OscopeADC_ReadOne(
    uint16_t *value
)
{
    HAL_StatusTypeDef status;


    if (value == NULL)
    {
        return HAL_ERROR;
    }


    /*
     * Start continuous conversion.
     */

    status =
        HAL_ADC_Start(&hadc1);

    if (status != HAL_OK)
    {
        return status;
    }


    /*
     * Wait for conversion.
     */

    status =
        HAL_ADC_PollForConversion(
            &hadc1,
            100
        );

    if (status != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);

        return status;
    }


    /*
     * Read value.
     */

    *value =
        (uint16_t)
        HAL_ADC_GetValue(&hadc1);


    /*
     * Stop ADC.
     */

    HAL_ADC_Stop(&hadc1);


    return HAL_OK;
}


/* --------------------------------------------------------------------------
 * Get block size
 * -------------------------------------------------------------------------- */

uint16_t OscopeADC_GetBlockSize(void)
{
    return OSCOPE_ADC_SAMPLE_COUNT;
}
