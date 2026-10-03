#include "OscopeADC.h"

#include <string.h>

#include "main.h"


/* ==========================================================
 * External handles
 * ========================================================== */

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim2;


/* ==========================================================
 * DMA buffer
 * ========================================================== */

static uint16_t adc_dma_buffer[
    OSCOPE_ADC_DMA_BUFFER_SIZE
];


/* ==========================================================
 * State
 * ========================================================== */

static volatile bool adc_running = false;

/*
 * آخرین نیمه‌ای که DMA کامل کرده:
 *
 * 0 = first half
 * 1 = second half
 */
static volatile uint8_t latest_completed_half = 0U;

/*
 * هر بار که یک نیمه کامل شود، این counter زیاد می‌شود.
 */
static volatile uint32_t completed_block_counter = 0U;

/*
 * آخرین block تحویل داده‌شده به OscopePage
 */
static uint32_t delivered_block_counter = 0U;


/* ==========================================================
 * Start
 * ========================================================== */

HAL_StatusTypeDef OscopeADC_Start(void)
{
    HAL_StatusTypeDef status;


    if (adc_running)
    {
        return HAL_OK;
    }


    latest_completed_half = 0U;

    completed_block_counter = 0U;

    delivered_block_counter = 0U;


    /*
     * ADC DMA شروع می‌شود.
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
     * سپس Timer شروع می‌شود.
     *
     * TIM2 TRGO
     * -> ADC
     */
    status =
        HAL_TIM_Base_Start(
            &htim2
        );


    if (status != HAL_OK)
    {
        HAL_ADC_Stop_DMA(
            &hadc1
        );

        return status;
    }


    adc_running = true;


    return HAL_OK;
}


/* ==========================================================
 * Stop
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
     * اول Timer متوقف شود.
     */
    timer_status =
        HAL_TIM_Base_Stop(
            &htim2
        );


    /*
     * سپس ADC + DMA.
     */
    adc_status =
        HAL_ADC_Stop_DMA(
            &hadc1
        );


    adc_running = false;


    if (timer_status != HAL_OK)
    {
        return timer_status;
    }


    return adc_status;
}


/* ==========================================================
 * Get latest stable block
 * ========================================================== */

bool OscopeADC_GetLatestBlock(
    uint16_t *destination,
    uint16_t destination_size
)
{
    uint32_t sequence_snapshot;
    uint32_t dma_remaining;

    uint8_t safe_half;

    uint16_t *source;

    uint16_t i;


    if (destination == NULL)
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


    if (!adc_running)
    {
        return false;
    }


    /*
     * Snapshot interrupt state.
     */
    __disable_irq();

    sequence_snapshot =
        completed_block_counter;

    __enable_irq();


    /*
     * هنوز block جدیدی نرسیده.
     */
    if (
        sequence_snapshot ==
        delivered_block_counter
    )
    {
        return false;
    }


    /*
     * مقدار باقی‌مانده DMA را می‌خوانیم
     * تا نیمه‌ای را انتخاب کنیم که
     * در حال حاضر توسط DMA نوشته نمی‌شود.
     *
     * اگر NDTR > 1024 باشد:
     *
     * DMA در نیمه اول است،
     * پس نیمه دوم امن است.
     *
     * اگر NDTR <= 1024 باشد:
     *
     * DMA در نیمه دوم است،
     * پس نیمه اول امن است.
     */
    dma_remaining =
        __HAL_DMA_GET_COUNTER(
            &hdma_adc1
        );


    if (
        dma_remaining >
        OSCOPE_ADC_BLOCK_SIZE
    )
    {
        safe_half = 1U;
    }
    else
    {
        safe_half = 0U;
    }


    if (safe_half == 0U)
    {
        source =
            &adc_dma_buffer[0U];
    }
    else
    {
        source =
            &adc_dma_buffer[
                OSCOPE_ADC_BLOCK_SIZE
            ];
    }


    /*
     * این نیمه توسط DMA در این لحظه
     * در حال نوشته‌شدن نیست.
     *
     * بنابراین CPU می‌تواند آن را کپی کند.
     */
    for (
        i = 0U;
        i < OSCOPE_ADC_BLOCK_SIZE;
        i++
    )
    {
        destination[i] =
            source[i];
    }


    /*
     * Sequence تحویل داده شد.
     */
    delivered_block_counter =
        sequence_snapshot;


    return true;
}


/* ==========================================================
 * Compatibility API
 * ========================================================== */

HAL_StatusTypeDef OscopeADC_ReadSamples(
    uint16_t *destination,
    uint16_t destination_size
)
{
    if (
        OscopeADC_GetLatestBlock(
            destination,
            destination_size
        )
    )
    {
        return HAL_OK;
    }


    return HAL_BUSY;
}


/* ==========================================================
 * Read one
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


    if (
        !OscopeADC_GetLatestBlock(
            temporary,
            OSCOPE_ADC_BLOCK_SIZE
        )
    )
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
 * DMA Half Complete
 *
 * IMPORTANT:
 * Do NOT copy samples here.
 * Keep ISR extremely short.
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


    latest_completed_half =
        0U;


    completed_block_counter++;
}


/* ==========================================================
 * DMA Complete
 *
 * IMPORTANT:
 * Do NOT copy samples here.
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


    latest_completed_half =
        1U;


    completed_block_counter++;
}
