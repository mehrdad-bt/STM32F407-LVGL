#ifndef OSCOPE_ADC_H
#define OSCOPE_ADC_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ADC sample block delivered to OscopePage.
 *
 * 8192 samples at 500 kS/s = 16.384 ms of waveform data.
 *
 * IMPORTANT:
 * The DMA buffer is double-buffered internally, so two complete
 * 8192-sample halves are available without keeping a third shadow copy.
 */
#define OSCOPE_ADC_BLOCK_SIZE       8192U

/*
 * Two complete DMA halves.
 *
 * 16384 x uint16_t = 32768 bytes = 32 KB.
 */
#define OSCOPE_ADC_DMA_BUFFER_SIZE  (OSCOPE_ADC_BLOCK_SIZE * 2U)

#define OSCOPE_ADC_SAMPLE_RATE_HZ   500000U

HAL_StatusTypeDef OscopeADC_Start(void);
void OscopeADC_Stop(void);

bool OscopeADC_GetLatestBlock(
    uint16_t *destination,
    uint32_t destination_size
);

uint32_t OscopeADC_GetSampleRate(void);

#ifdef __cplusplus
}
#endif

#endif /* OSCOPE_ADC_H */
