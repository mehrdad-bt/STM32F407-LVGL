#include "OscopeADC.h"

#include "main.h"

#include <string.h>


/* ============================================================
 * EXTERNAL CUBE/HAL HANDLES
 * ============================================================ */

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;


/* ============================================================
 * SIGNAL CHANNEL
 *
 * Must be the same as the channel configured in MX_ADC1_Init().
 * It is restored after each VREFINT measurement.
 * ============================================================ */

#define OSCOPE_SIGNAL_CHANNEL          ADC_CHANNEL_12

#define OSCOPE_SIGNAL_SAMPLETIME       ADC_SAMPLETIME_15CYCLES


/* ============================================================
 * VREFINT (internal reference, about 1.21 V)
 * ============================================================ */

/*
 * Factory calibration: raw ADC value of VREFINT measured with
 * VDDA = 3.3 V at 30 degrees C (STM32F405/407/415/417 and
 * the other F4 devices that provide it).
 */
#ifndef OSCOPE_VREFINT_CAL_PTR
#define OSCOPE_VREFINT_CAL_PTR \
    ((const volatile uint16_t *)0x1FFF7A2AU)
#endif

#define OSCOPE_VREFINT_CAL_VDDA_MV     3300U

/*
 * Plausible range of the calibration word.
 * 1.21 V / 3.3 V x 4095 = about 1500.
 * An erased (0xFFFF) or zero word is rejected.
 */
#define OSCOPE_VREFINT_CAL_MIN         1200U
#define OSCOPE_VREFINT_CAL_MAX         1800U

/*
 * VREFINT needs at least 10 us sampling time:
 * 480 cycles / 21 MHz = 22.9 us.
 */
#define OSCOPE_VREFINT_SAMPLES         32U

/*
 * Plausible supply range, anything else is rejected.
 */
#define OSCOPE_VDDA_MIN_MV             2400U
#define OSCOPE_VDDA_MAX_MV             3600U


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

/*
 * Number of completed blocks (only increases).
 */
static volatile uint32_t block_sequence = 0U;

/*
 * Current (actual) sample rate.
 */
static uint32_t sample_rate_hz =
    OSCOPE_ADC_DEFAULT_SAMPLE_RATE_HZ;

/*
 * Reference voltage.
 */
static uint32_t vdda_mv =
    OSCOPE_ADC_VDDA_NOMINAL_MV;

static bool vdda_measured = false;


/* ============================================================
 * TIMER CLOCK
 * ============================================================ */

/*
 * TIM2 is on APB1.
 * When the APB1 prescaler is not 1, the timer clock is 2 x PCLK1.
 * (168 MHz system: PCLK1 = 42 MHz, TIM2 = 84 MHz)
 */
static uint32_t get_timer_clock_hz(void)
{
    uint32_t clk;


    clk =
        HAL_RCC_GetPCLK1Freq();


    if (
        (RCC->CFGR & RCC_CFGR_PPRE1) !=
        RCC_CFGR_PPRE1_DIV1
    )
    {
        clk *= 2U;
    }


    return clk;
}


/* ============================================================
 * MEASURE VDDA WITH VREFINT
 *
 * VREFINT is a fixed 1.21 V inside the chip.
 * The factory stored the ADC reading it gave at VDDA = 3.3 V.
 * The same voltage read at the present VDDA gives
 *
 *   VDDA = 3.3 V x VREFINT_CAL / VREFINT_DATA
 *
 * Must only be called while the ADC/DMA/timer are stopped.
 * The signal channel is restored at the end.
 * ============================================================ */

static void measure_vdda(void)
{
    ADC_ChannelConfTypeDef channel_config = {0};

    uint32_t cal;
    uint32_t sum;
    uint32_t count;
    uint32_t i;
    uint32_t raw;
    uint32_t result;


    cal =
        (uint32_t)(*OSCOPE_VREFINT_CAL_PTR);


    if (
        cal < OSCOPE_VREFINT_CAL_MIN ||
        cal > OSCOPE_VREFINT_CAL_MAX
    )
    {
        /*
         * No usable calibration value in this chip.
         * Keep the nominal 3.3 V.
         */
        return;
    }


    sum =
        0U;

    count =
        0U;


    /*
     * Select VREFINT with a long sampling time.
     * HAL also switches on the VREFINT path (ADC_CCR_TSVREFE).
     */
    channel_config.Channel =
        ADC_CHANNEL_VREFINT;

    channel_config.Rank =
        1U;

    channel_config.SamplingTime =
        ADC_SAMPLETIME_480CYCLES;


    if (
        HAL_ADC_ConfigChannel(
            &hadc1,
            &channel_config
        ) == HAL_OK
    )
    {
        /*
         * Software start: remove the TIM2 trigger for now.
         */
        hadc1.Instance->CR2 &=
            ~ADC_CR2_EXTEN;


        for (
            i = 0U;
            i < OSCOPE_VREFINT_SAMPLES;
            i++
        )
        {
            if (
                HAL_ADC_Start(
                    &hadc1
                ) != HAL_OK
            )
            {
                break;
            }


            if (
                HAL_ADC_PollForConversion(
                    &hadc1,
                    5U
                ) == HAL_OK
            )
            {
                sum +=
                    HAL_ADC_GetValue(
                        &hadc1
                    );

                count++;
            }


            HAL_ADC_Stop(
                &hadc1
            );
        }
    }


    /*
     * Switch the VREFINT path off and restore the signal channel
     * and the TIM2 trigger edge.
     */
    ADC->CCR &=
        ~ADC_CCR_TSVREFE;


    channel_config.Channel =
        OSCOPE_SIGNAL_CHANNEL;

    channel_config.Rank =
        1U;

    channel_config.SamplingTime =
        OSCOPE_SIGNAL_SAMPLETIME;


    HAL_ADC_ConfigChannel(
        &hadc1,
        &channel_config
    );


    hadc1.Instance->CR2 =
        (
            hadc1.Instance->CR2 &
            ~ADC_CR2_EXTEN
        )
        |
        hadc1.Init.ExternalTrigConvEdge;


    /*
     * Result.
     * At least half of the conversions must have worked.
     */
    if (
        count <
        (OSCOPE_VREFINT_SAMPLES / 2U)
    )
    {
        return;
    }


    raw =
        (sum + (count / 2U)) /
        count;


    if (
        raw == 0U
    )
    {
        return;
    }


    result =
        (uint32_t)(
            (
                (uint64_t)OSCOPE_VREFINT_CAL_VDDA_MV *
                (uint64_t)cal
                +
                (uint64_t)(raw / 2U)
            )
            /
            (uint64_t)raw
        );


    if (
        result < OSCOPE_VDDA_MIN_MV ||
        result > OSCOPE_VDDA_MAX_MV
    )
    {
        return;
    }


    vdda_mv =
        result;


    vdda_measured =
        true;
}


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
     * Measure the supply voltage while everything is stopped.
     * Takes about 1 ms.
     */
    measure_vdda();


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
     * Start from counter = 0, otherwise a counter value
     * above the new ARR would run through a full 32-bit
     * wrap-around before the first trigger.
     */
    __HAL_TIM_SET_COUNTER(
        &htim2,
        0U
    );


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
 * SET SAMPLE RATE
 * ============================================================ */

HAL_StatusTypeDef OscopeADC_SetSampleRate(
    uint32_t rate_hz
)
{
    uint32_t timer_clk;
    uint32_t period_ticks;
    bool was_running;


    if (
        rate_hz >
        OSCOPE_ADC_MAX_SAMPLE_RATE_HZ
    )
    {
        rate_hz =
            OSCOPE_ADC_MAX_SAMPLE_RATE_HZ;
    }


    if (
        rate_hz <
        OSCOPE_ADC_MIN_SAMPLE_RATE_HZ
    )
    {
        rate_hz =
            OSCOPE_ADC_MIN_SAMPLE_RATE_HZ;
    }


    timer_clk =
        get_timer_clock_hz();


    /*
     * Rounded timer period in timer ticks.
     * TIM2 is a 32-bit timer, so no prescaler is needed.
     */
    period_ticks =
        (
            timer_clk +
            (rate_hz / 2U)
        )
        /
        rate_hz;


    if (
        period_ticks <
        2U
    )
    {
        period_ticks =
            2U;
    }


    was_running =
        adc_running;


    /*
     * The timer and the ADC/DMA must be stopped while
     * the period is changed.
     */
    if (
        was_running
    )
    {
        OscopeADC_Stop();
    }


    __HAL_TIM_SET_AUTORELOAD(
        &htim2,
        period_ticks - 1U
    );


    __HAL_TIM_SET_COUNTER(
        &htim2,
        0U
    );


    htim2.Init.Prescaler =
        0U;


    htim2.Init.Period =
        period_ticks - 1U;


    /*
     * Actual rate after rounding.
     */
    sample_rate_hz =
        (
            timer_clk +
            (period_ticks / 2U)
        )
        /
        period_ticks;


    /*
     * Any stored block belongs to the old rate.
     */
    latest_block_valid =
        false;


    if (
        was_running
    )
    {
        return OscopeADC_Start();
    }


    return HAL_OK;
}


/* ============================================================
 * GET LATEST BLOCK
 * ============================================================ */

bool OscopeADC_GetLatestBlockSeq(
    uint16_t *destination,
    uint32_t destination_size,
    uint32_t *sequence
)
{
    bool valid;
    uint32_t half_index;
    uint32_t seq;
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


    seq =
        block_sequence;


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


    if (
        valid &&
        sequence != NULL
    )
    {
        *sequence =
            seq;
    }


    return valid;
}


bool OscopeADC_GetLatestBlock(
    uint16_t *destination,
    uint32_t destination_size
)
{
    return
        OscopeADC_GetLatestBlockSeq(
            destination,
            destination_size,
            NULL
        );
}


/* ============================================================
 * SAMPLE RATE
 * ============================================================ */

uint32_t OscopeADC_GetSampleRate(void)
{
    return sample_rate_hz;
}


/* ============================================================
 * VDDA
 * ============================================================ */

uint32_t OscopeADC_GetVdda_mV(void)
{
    return vdda_mv;
}


bool OscopeADC_IsVddaMeasured(void)
{
    return vdda_measured;
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


    block_sequence++;


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


    block_sequence++;


    latest_block_valid =
        true;
}
