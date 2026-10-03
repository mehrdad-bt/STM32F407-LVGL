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
 * 8192 samples per block.
 * Block duration = 8192 / sample rate
 *   700 kS/s -> 11.7 ms
 *     4 kS/s -> 2.05 s
 *
 * The DMA buffer is double-buffered internally, so two complete
 * 8192-sample halves are available without a third shadow copy.
 */
#define OSCOPE_ADC_BLOCK_SIZE       8192U

/*
 * Two complete DMA halves.
 *
 * 16384 x uint16_t = 32768 bytes = 32 KB.
 */
#define OSCOPE_ADC_DMA_BUFFER_SIZE  (OSCOPE_ADC_BLOCK_SIZE * 2U)

/*
 * Sample rate limits.
 *
 * ADC clock = 21 MHz, sampling time = 15 cycles, conversion = 12 cycles
 * -> 27 cycles = 1.286 us -> absolute maximum about 777 kS/s.
 * 700 kS/s keeps a safety margin.
 */
#define OSCOPE_ADC_MAX_SAMPLE_RATE_HZ      700000U
#define OSCOPE_ADC_MIN_SAMPLE_RATE_HZ      1000U

/*
 * Default rate (same as the value generated in MX_TIM2_Init).
 */
#define OSCOPE_ADC_DEFAULT_SAMPLE_RATE_HZ  500000U

/*
 * Old name kept for compatibility.
 */
#define OSCOPE_ADC_SAMPLE_RATE_HZ   OSCOPE_ADC_DEFAULT_SAMPLE_RATE_HZ

HAL_StatusTypeDef OscopeADC_Start(void);
void OscopeADC_Stop(void);

/*
 * Change the sampling rate (TIM2 period).
 *
 * If the ADC is running it is restarted, and the old data
 * is discarded (the first new block is valid after
 * 8192 / rate seconds).
 */
HAL_StatusTypeDef OscopeADC_SetSampleRate(
    uint32_t rate_hz
);

bool OscopeADC_GetLatestBlock(
    uint16_t *destination,
    uint32_t destination_size
);

/*
 * Actual rate (rounded to an integer timer period).
 */
uint32_t OscopeADC_GetSampleRate(void);

#ifdef __cplusplus
}
#endif

#endif /* OSCOPE_ADC_H */
