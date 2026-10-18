/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "Application.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * External crystal frequency in MHz (the crystal soldered on the board,
 * next to the OSC_IN / OSC_OUT pins PH0 / PH1).
 *
 * IMPORTANT:
 *   HSE_VALUE in stm32f4xx_hal_conf.h must be the same value
 *   (e.g. 8000000U for an 8 MHz crystal).  The check below stops
 *   the build when they differ, because a wrong value would make
 *   SYSCLK, SPI, timers and the oscilloscope time base wrong.
 *
 * Use 8 for an 8 MHz crystal, 12 for 12 MHz, 16 for 16 MHz,
 * 25 for 25 MHz.
 */
#define OSCOPE_HSE_MHZ      8U

/*
 * Uncomment when the board has an external active oscillator
 * (clock signal on OSC_IN, no crystal).
 */
/* #define OSCOPE_HSE_BYPASS */

_Static_assert(
    HSE_VALUE == (OSCOPE_HSE_MHZ * 1000000U),
    "HSE_VALUE in stm32f4xx_hal_conf.h must match OSCOPE_HSE_MHZ in main.c"
);

_Static_assert(
    (OSCOPE_HSE_MHZ >= 2U) && (OSCOPE_HSE_MHZ <= 63U),
    "OSCOPE_HSE_MHZ must be between 2 and 63 (PLLM range)"
);

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/*
 * 1 = system runs from the external crystal (HSE).
 * 0 = HSE did not start, system fell back to the internal HSI.
 * (Look at it in the debugger.)
 */
volatile uint8_t g_clock_hse_active = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();
    MX_TIM2_Init();

    /* USER CODE BEGIN 2 */

    Application_Init();

    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */

    while (1)
    {
        Application_Run();

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }

    /* USER CODE END 3 */
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(
        PWR_REGULATOR_VOLTAGE_SCALE1
    );

    /*
     * ------------------------------------------------------
     * Primary clock source: external crystal (HSE)
     * ------------------------------------------------------
     *
     * HSE / PLLM = 1 MHz   (VCO input)
     * VCO        = 1 MHz x 336 = 336 MHz
     * SYSCLK     = 336 MHz / 2 = 168 MHz
     * USB / SDIO = 336 MHz / 7 =  48 MHz
     *
     * PLLM equals the crystal frequency in MHz, so the VCO input
     * is always 1 MHz and the same PLLN works for 8, 12, 16, 25 MHz.
     *
     * The crystal accuracy is tens of ppm, the internal HSI only
     * about 1 %.  ADC sample rate, frequency counter and Time/Div
     * all follow this clock.
     */
    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSE;

#ifdef OSCOPE_HSE_BYPASS
    RCC_OscInitStruct.HSEState =
        RCC_HSE_BYPASS;
#else
    RCC_OscInitStruct.HSEState =
        RCC_HSE_ON;
#endif

    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_ON;

    RCC_OscInitStruct.PLL.PLLSource =
        RCC_PLLSOURCE_HSE;

    RCC_OscInitStruct.PLL.PLLM = OSCOPE_HSE_MHZ;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (
        HAL_RCC_OscConfig(
            &RCC_OscInitStruct
        ) == HAL_OK
    )
    {
        g_clock_hse_active = 1U;
    }
    else
    {
        /*
         * ------------------------------------------------------
         * Fallback: crystal did not start (or PLL did not lock)
         * ------------------------------------------------------
         *
         * Run from the internal HSI so the device still works:
         *
         * HSI = 16 MHz
         * HSI / 8 = 2 MHz, x 168 = 336 MHz, / 2 = 168 MHz
         *
         * The HSE is switched off again.
         */
        RCC_OscInitStruct = (RCC_OscInitTypeDef){0};

        RCC_OscInitStruct.OscillatorType =
            RCC_OSCILLATORTYPE_HSE |
            RCC_OSCILLATORTYPE_HSI;

        RCC_OscInitStruct.HSEState =
            RCC_HSE_OFF;

        RCC_OscInitStruct.HSIState =
            RCC_HSI_ON;

        RCC_OscInitStruct.HSICalibrationValue =
            RCC_HSICALIBRATION_DEFAULT;

        RCC_OscInitStruct.PLL.PLLState =
            RCC_PLL_ON;

        RCC_OscInitStruct.PLL.PLLSource =
            RCC_PLLSOURCE_HSI;

        RCC_OscInitStruct.PLL.PLLM = 8;
        RCC_OscInitStruct.PLL.PLLN = 168;
        RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
        RCC_OscInitStruct.PLL.PLLQ = 7;

        if (
            HAL_RCC_OscConfig(
                &RCC_OscInitStruct
            ) != HAL_OK
        )
        {
            Error_Handler();
        }

        g_clock_hse_active = 0U;
    }

    /*
     * CPU  = 168 MHz
     * APB1 = 42 MHz  (timers: 84 MHz)
     * APB2 = 84 MHz
     * TIM2 = 84 MHz
     */
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_PLLCLK;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV4;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV2;

    if (
        HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_5
        ) != HAL_OK
    )
    {
        Error_Handler();
    }
}


/**
  * @brief ADC1 Initialization Function
  * @retval None
  */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance =
        ADC1;

    /*
     * PCLK2 = 84 MHz
     * ADC clock = 84 / 4 = 21 MHz
     */
    hadc1.Init.ClockPrescaler =
        ADC_CLOCK_SYNC_PCLK_DIV4;

    hadc1.Init.Resolution =
        ADC_RESOLUTION_12B;

    hadc1.Init.ScanConvMode =
        DISABLE;

    /*
     * One conversion per TIM2 trigger.
     */
    hadc1.Init.ContinuousConvMode =
        DISABLE;

    hadc1.Init.DiscontinuousConvMode =
        DISABLE;

    /*
     * TIM2 TRGO -> ADC
     */
    hadc1.Init.ExternalTrigConvEdge =
        ADC_EXTERNALTRIGCONVEDGE_RISING;

    hadc1.Init.ExternalTrigConv =
        ADC_EXTERNALTRIGCONV_T2_TRGO;

    hadc1.Init.DataAlign =
        ADC_DATAALIGN_RIGHT;

    hadc1.Init.NbrOfConversion =
        1;

    /*
     * DMA remains active in circular mode.
     */
    hadc1.Init.DMAContinuousRequests =
        ENABLE;

    hadc1.Init.EOCSelection =
        ADC_EOC_SINGLE_CONV;

    if (
        HAL_ADC_Init(
            &hadc1
        ) != HAL_OK
    )
    {
        Error_Handler();
    }

    /*
     * PC2 = ADC1_IN12
     */
    sConfig.Channel =
        ADC_CHANNEL_12;

    sConfig.Rank =
        1;

    /*
     * 15 cycles is enough for this test source.
     */
    sConfig.SamplingTime =
        ADC_SAMPLETIME_15CYCLES;

    if (
        HAL_ADC_ConfigChannel(
            &hadc1,
            &sConfig
        ) != HAL_OK
    )
    {
        Error_Handler();
    }
}


/**
  * @brief I2C1 Initialization Function
  * @retval None
  */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance =
        I2C1;

    hi2c1.Init.ClockSpeed =
        100000;

    hi2c1.Init.DutyCycle =
        I2C_DUTYCYCLE_2;

    hi2c1.Init.OwnAddress1 =
        0;

    hi2c1.Init.AddressingMode =
        I2C_ADDRESSINGMODE_7BIT;

    hi2c1.Init.DualAddressMode =
        I2C_DUALADDRESS_DISABLE;

    hi2c1.Init.OwnAddress2 =
        0;

    hi2c1.Init.GeneralCallMode =
        I2C_GENERALCALL_DISABLE;

    hi2c1.Init.NoStretchMode =
        I2C_NOSTRETCH_DISABLE;

    if (
        HAL_I2C_Init(
            &hi2c1
        ) != HAL_OK
    )
    {
        Error_Handler();
    }
}


/**
  * @brief SPI1 Initialization Function
  * @retval None
  */
static void MX_SPI1_Init(void)
{
    hspi1.Instance =
        SPI1;

    hspi1.Init.Mode =
        SPI_MODE_MASTER;

    hspi1.Init.Direction =
        SPI_DIRECTION_2LINES;

    hspi1.Init.DataSize =
        SPI_DATASIZE_8BIT;

    hspi1.Init.CLKPolarity =
        SPI_POLARITY_LOW;

    hspi1.Init.CLKPhase =
        SPI_PHASE_1EDGE;

    hspi1.Init.NSS =
        SPI_NSS_SOFT;

    hspi1.Init.BaudRatePrescaler =
        SPI_BAUDRATEPRESCALER_2;

    hspi1.Init.FirstBit =
        SPI_FIRSTBIT_MSB;

    hspi1.Init.TIMode =
        SPI_TIMODE_DISABLE;

    hspi1.Init.CRCCalculation =
        SPI_CRCCALCULATION_DISABLE;

    hspi1.Init.CRCPolynomial =
        10;

    if (
        HAL_SPI_Init(
            &hspi1
        ) != HAL_OK
    )
    {
        Error_Handler();
    }
}


/**
  * @brief TIM2 Initialization Function
  * @retval None
  */
static void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /*
     * TIM2 clock = 84 MHz
     *
     * New sample rate:
     *
     * 84 MHz / (167 + 1)
     * = 500 kHz
     *
     * One sample every 2 us.
     */
    htim2.Instance =
        TIM2;

    htim2.Init.Prescaler =
        0;

    htim2.Init.CounterMode =
        TIM_COUNTERMODE_UP;

    htim2.Init.Period =
        167;

    htim2.Init.ClockDivision =
        TIM_CLOCKDIVISION_DIV1;

    htim2.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (
        HAL_TIM_Base_Init(
            &htim2
        ) != HAL_OK
    )
    {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource =
        TIM_CLOCKSOURCE_INTERNAL;

    if (
        HAL_TIM_ConfigClockSource(
            &htim2,
            &sClockSourceConfig
        ) != HAL_OK
    )
    {
        Error_Handler();
    }

    /*
     * TIM2 UPDATE -> TRGO
     */
    sMasterConfig.MasterOutputTrigger =
        TIM_TRGO_UPDATE;

    sMasterConfig.MasterSlaveMode =
        TIM_MASTERSLAVEMODE_DISABLE;

    if (
        HAL_TIMEx_MasterConfigSynchronization(
            &htim2,
            &sMasterConfig
        ) != HAL_OK
    )
    {
        Error_Handler();
    }
}


/**
  * @brief USART1 Initialization Function
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance =
        USART1;

    huart1.Init.BaudRate =
        115200;

    huart1.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart1.Init.StopBits =
        UART_STOPBITS_1;

    huart1.Init.Parity =
        UART_PARITY_NONE;

    huart1.Init.Mode =
        UART_MODE_TX_RX;

    huart1.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart1.Init.OverSampling =
        UART_OVERSAMPLING_16;

    if (
        HAL_UART_Init(
            &huart1
        ) != HAL_OK
    )
    {
        Error_Handler();
    }
}


/**
  * @brief Enable DMA controller clock
  * @retval None
  */
static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    /*
     * ADC1 DMA
     */
    HAL_NVIC_SetPriority(
        DMA2_Stream0_IRQn,
        0,
        0
    );

    HAL_NVIC_EnableIRQ(
        DMA2_Stream0_IRQn
    );

    /*
     * SPI1 TX DMA
     */
    HAL_NVIC_SetPriority(
        DMA2_Stream3_IRQn,
        0,
        0
    );

    HAL_NVIC_EnableIRQ(
        DMA2_Stream3_IRQn
    );
}


/**
  * @brief GPIO Initialization Function
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /*
     * LCD DC
     */
    HAL_GPIO_WritePin(
        DC_GPIO_Port,
        DC_Pin,
        GPIO_PIN_RESET
    );

    /*
     * LCD CS + RESET
     */
    HAL_GPIO_WritePin(
        GPIOB,
        CS_Pin | RESET_Pin,
        GPIO_PIN_RESET
    );

    /*
     * Touch CS
     */
    HAL_GPIO_WritePin(
        TCS_GPIO_Port,
        TCS_Pin,
        GPIO_PIN_RESET
    );

    /*
     * DC
     */
    GPIO_InitStruct.Pin =
        DC_Pin;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        DC_GPIO_Port,
        &GPIO_InitStruct
    );

    /*
     * LCD CS + RESET
     */
    GPIO_InitStruct.Pin =
        CS_Pin | RESET_Pin;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        GPIOB,
        &GPIO_InitStruct
    );

    /*
     * Touch CS
     */
    GPIO_InitStruct.Pin =
        TCS_Pin;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        TCS_GPIO_Port,
        &GPIO_InitStruct
    );
}


/* USER CODE BEGIN 4 */

/* USER CODE END 4 */


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}


#ifdef USE_FULL_ASSERT

void assert_failed(
    uint8_t *file,
    uint32_t line
)
{
    (void)file;
    (void)line;
}

#endif /* USE_FULL_ASSERT */
