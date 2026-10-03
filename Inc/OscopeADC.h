#ifndef OSCOPE_ADC_H
#define OSCOPE_ADC_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Configuration
 * -------------------------------------------------------------------------- */

#define OSCOPE_ADC_SAMPLE_COUNT    300U


/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/*
 * Read one complete block of ADC samples.
 *
 * Return value:
 *
 * HAL_OK
 * HAL_ERROR
 * HAL_BUSY
 * HAL_TIMEOUT
 */

HAL_StatusTypeDef OscopeADC_ReadSamples(
    uint16_t *destination,
    uint16_t destination_size
);


/*
 * Read one ADC sample.
 *
 * This is useful for the live numeric display.
 */

HAL_StatusTypeDef OscopeADC_ReadOne(
    uint16_t *value
);


/*
 * Return the number of samples in one block.
 */

uint16_t OscopeADC_GetBlockSize(void);


#ifdef __cplusplus
}
#endif

#endif
