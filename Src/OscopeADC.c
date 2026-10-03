#include "OscopeADC.h"

#include <string.h>

#include "main.h"


/* ==========================================================
 * External HAL handles
 * ========================================================== */

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;


/* ==========================================================
 * DMA buffer
 * ========================================================== */

/*
 * 1024 samples total
 *
 * Half  = 512 samples
 * Full  = 512 samples
 *
 * At 200 kHz:
 *
 * 512 samples = 2.56 ms
 */
static uint16_t adc_dma_buffer[
    OSCOPE_ADC_DMA_BUFFER_SIZE
];


/*
 * آخرین بلوک کامل و پایدار.
 *
 * DMA مستقیماً در این بافر نمی‌نویسد.
 */
static uint16_t latest_block[
    OSCOPE_ADC_BLOCK_SIZE
];


/* ==========================================================
 * State
 * ========================================================== */

static volatile bool latest_block_ready = false;

static volatile bool adc_running = false;


/* ==========================================================
 * Copy DMA half into stable buffer
 * ========================================================== */

static void copy_dma_block(
    const uint16_t *source
)
{
    uint16_t i;


    if (source == NULL)
    {
        return;
    }


    /*
     * این copy داخل interrupt انجام می‌شود.
     *
     * 512 نمونه × 2 byte = 1024 byte
     */
    for (i = 0U;
         i < OSCOPE_ADC_BLOCK_SIZE;
         i++)
    {
        latest_block[i] =
            source[i];
    }


    latest_block_ready = true;
}


/* ==========================================================
 * Start ADC + DMA + Timer
 * ========================================================== */

HAL_StatusTypeDef OscopeADC_Start(void)
{
    HAL_StatusTypeDef status;


    if (adc_running)
    {
        return HAL_OK;
    }


    latest_block_ready = false;


    /*
     * ADC + DMA
     *
     * DMA buffer is circular.
     */
    status =
        HAL_ADC_Start_DMA(
            &hadc1,
            (uint32_t *)adc_dma_buffer,
            OSCOPE_ADC_DMA_BUFFER_SIZE
        );


    if (status != HAL_OK)
    {
        return status;
    }


    /*
     * ADC DMA must already be active
     * before starting the trigger timer.
     */
    status =
        HAL_TIM_Base_Start(
            &htim2
        );


    if (status != HAL_OK)
    {
        HAL_ADC_Stop_DMA(&hadc1);

        return status;
    }


    adc_running = true;


    return HAL_OK;
}


/* ==========================================================
 * Stop ADC + DMA + Timer
 * ========================================================== */

HAL_StatusTypeDef OscopeADC_Stop(void)
{
    HAL_StatusTypeDef timer_status;
    HAL_StatusTypeDef adc_status;


    if (!adc_running)
    {
        return HAL_OK;
    }


    /*
     * اول Trigger متوقف شود.
     */
    timer_status =
        HAL_TIM_Base_Stop(
            &htim2
        );


    /*
     * سپس ADC + DMA متوقف شود.
     */
    adc_status =
        HAL_ADC_Stop_DMA(
            &hadc1
        );


    adc_running = false;

    latest_block_ready = false;


    if (timer_status != HAL_OK)
    {
        return timer_status;
    }


    return adc_status;
}


/* ==========================================================
 * Get latest complete block
 * ========================================================== */

bool OscopeADC_GetLatestBlock(
    uint16_t *destination,
    uint16_t destination_size
)
{
    uint16_t i;


    if (destination == NULL)
    {
        return false;
    }


    if (destination_size <
        OSCOPE_ADC_BLOCK_SIZE)
    {
        return false;
    }


    /*
     * Callback ممکن است همزمان در حال تغییر latest_block
     * باشد.
     *
     * برای copy حدود 1KB، موقتاً interrupt را متوقف می‌کنیم.
     * این زمان در 168MHz بسیار کوتاه است.
     */
    __disable_irq();


    if (!latest_block_ready)
    {
        __enable_irq();

        return false;
    }


    for (i = 0U;
         i < OSCOPE_ADC_BLOCK_SIZE;
         i++)
    {
        destination[i] =
            latest_block[i];
    }


    latest_block_ready = false;


    __enable_irq();


    return true;
}


/* ==========================================================
 * Compatibility function
 * ========================================================== */

HAL_StatusTypeDef OscopeADC_ReadSamples(
    uint16_t *destination,
    uint16_t destination_size
)
{
    bool result;


    if (destination == NULL)
    {
        return HAL_ERROR;
    }


    if (destination_size <
        OSCOPE_ADC_BLOCK_SIZE)
    {
        return HAL_ERROR;
    }


    result =
        OscopeADC_GetLatestBlock(
            destination,
            destination_size
        );


    if (result)
    {
        return HAL_OK;
    }


    return HAL_BUSY;
}


/* ==========================================================
 * Read one sample
 *
 * Compatibility function
 * ========================================================== */

HAL_StatusTypeDef OscopeADC_ReadOne(
    uint16_t *value
)
{
    uint16_t temporary[
        OSCOPE_ADC_BLOCK_SIZE
    ];


    if (value == NULL)
    {
        return HAL_ERROR;
    }


    if (!OscopeADC_GetLatestBlock(
            temporary,
            OSCOPE_ADC_BLOCK_SIZE))
    {
        return HAL_BUSY;
    }


    *value =
        temporary[
            OSCOPE_ADC_BLOCK_SIZE - 1U
        ];


    return HAL_OK;
}


/* ==========================================================
 * Block size
 * ========================================================== */

uint16_t OscopeADC_GetBlockSize(void)
{
    return OSCOPE_ADC_BLOCK_SIZE;
}


/* ==========================================================
 * Sample rate
 * ========================================================== */

uint32_t OscopeADC_GetSampleRate(void)
{
    return OSCOPE_ADC_SAMPLE_RATE_HZ;
}


/* ==========================================================
 * Running state
 * ========================================================== */

bool OscopeADC_IsRunning(void)
{
    return adc_running;
}


/* ==========================================================
 * DMA Half Transfer Callback
 * ========================================================== */

void HAL_ADC_ConvHalfCpltCallback(
    ADC_HandleTypeDef *hadc
)
{
    if (hadc == NULL)
    {
        return;
    }


    if (hadc->Instance != ADC1)
    {
        return;
    }


    /*
     * نیمه اول DMA:
     *
     * [0 ... 511]
     */
    copy_dma_block(
        &adc_dma_buffer[0]
    );
}


/* ==========================================================
 * DMA Complete Callback
 * ========================================================== */

void HAL_ADC_ConvCpltCallback(
    ADC_HandleTypeDef *hadc
)
{
    if (hadc == NULL)
    {
        return;
    }


    if (hadc->Instance != ADC1)
    {
        return;
    }


    /*
     * نیمه دوم DMA:
     *
     * [512 ... 1023]
     */
    copy_dma_block(
        &adc_dma_buffer[
            OSCOPE_ADC_BLOCK_SIZE
        ]
    );
}
