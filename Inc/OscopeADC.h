#ifndef OSCOPE_ADC_H
#define OSCOPE_ADC_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define OSCOPE_ADC_BUFFER_SIZE   2048U
#define OSCOPE_ADC_HALF_SIZE     (OSCOPE_ADC_BUFFER_SIZE / 2U)

HAL_StatusTypeDef OscopeADC_Start(void);
HAL_StatusTypeDef OscopeADC_Stop(void);

/*
 * Returns pointer to the most recently completed
 * stable DMA half-buffer.
 *
 * The returned block contains OSCOPE_ADC_HALF_SIZE samples.
 */
const uint16_t *OscopeADC_GetLatestBlock(void);

/*
 * Returns the number of valid samples in the block.
 */
uint16_t OscopeADC_GetBlockSize(void);

/*
 * Returns 0 or 1 indicating which DMA half
 * is currently available for reading.
 */
uint8_t OscopeADC_GetLatestHalf(void);

#ifdef __cplusplus
}
#endif

#endif /* OSCOPE_ADC_H */
