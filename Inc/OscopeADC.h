#ifndef OSCOPE_ADC_H
#define OSCOPE_ADC_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OSCOPE_ADC_DMA_BUFFER_SIZE   1024U
#define OSCOPE_ADC_BLOCK_SIZE         512U
#define OSCOPE_ADC_SAMPLE_RATE_HZ     200000UL

HAL_StatusTypeDef OscopeADC_Start(void);
HAL_StatusTypeDef OscopeADC_Stop(void);

bool OscopeADC_GetLatestBlock(
    uint16_t *destination,
    uint16_t destination_size
);

HAL_StatusTypeDef OscopeADC_ReadSamples(
    uint16_t *destination,
    uint16_t destination_size
);

HAL_StatusTypeDef OscopeADC_ReadOne(
    uint16_t *value
);

uint16_t OscopeADC_GetBlockSize(void);

uint32_t OscopeADC_GetSampleRate(void);

bool OscopeADC_IsRunning(void);

#ifdef __cplusplus
}
#endif

#endif /* OSCOPE_ADC_H */
