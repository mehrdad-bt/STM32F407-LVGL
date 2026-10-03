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

  /*
   * Keep the original peripheral initialization order.
   *
   * This is important for the existing LCD/SPI system.
   */

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

  /* Infinite loop */
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

  /*
   * Enable PWR clock.
   */

  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(
      PWR_REGULATOR_VOLTAGE_SCALE1
  );

  /*
   * HSI = 16 MHz
   *
   * PLLM = 8
   * PLLN = 168
   * PLLP = 2
   *
   * SYSCLK = 168 MHz
   */

  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_HSI;

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
  RCC_OscInitStruct.PLL.PLLQ = 4;

  if (
      HAL_RCC_OscConfig(
          &RCC_OscInitStruct
      ) != HAL_OK
  )
  {
    Error_Handler();
  }

  /*
   * CPU = 168 MHz
   * APB1 = 42 MHz
   * APB2 = 84 MHz
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

  /*
   * ADC1.
   */

  hadc1.Instance =
      ADC1;

  /*
   * ADC clock:
   *
   * PCLK2 = 84 MHz
   * ADC clock = 84 / 4 = 21 MHz
   */

  hadc1.Init.ClockPrescaler =
      ADC_CLOCK_SYNC_PCLK_DIV4;

  /*
   * 12-bit resolution.
   */

  hadc1.Init.Resolution =
      ADC_RESOLUTION_12B;

  /*
   * Single channel.
   */

  hadc1.Init.ScanConvMode =
      DISABLE;

  /*
   * IMPORTANT:
   *
   * Continuous conversion.
   *
   * ADC starts once and keeps converting PC2.
   */

  hadc1.Init.ContinuousConvMode =
      ENABLE;

  hadc1.Init.DiscontinuousConvMode =
      DISABLE;

  /*
   * Software trigger.
   *
   * TIM2 is NOT involved in this test.
   */

  hadc1.Init.ExternalTrigConvEdge =
      ADC_EXTERNALTRIGCONVEDGE_NONE;

  hadc1.Init.ExternalTrigConv =
      ADC_SOFTWARE_START;

  /*
   * Right aligned.
   */

  hadc1.Init.DataAlign =
      ADC_DATAALIGN_RIGHT;

  /*
   * One conversion in sequence.
   */

  hadc1.Init.NbrOfConversion =
      1;

  /*
   * DMA is disabled for this direct ADC test.
   */

  hadc1.Init.DMAContinuousRequests =
      DISABLE;

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
   * PC2 = ADC1_IN12.
   */

  sConfig.Channel =
      ADC_CHANNEL_12;

  sConfig.Rank =
      1;

  /*
   * Long sample time for the diagnostic test.
   */

  sConfig.SamplingTime =
      ADC_SAMPLETIME_84CYCLES;

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
   * TIM2 is kept because it exists in the project.
   *
   * It is NOT used by ADC in this test.
   */

  htim2.Instance =
      TIM2;

  htim2.Init.Prescaler =
      0;

  htim2.Init.CounterMode =
      TIM_COUNTERMODE_UP;

  htim2.Init.Period =
      419;

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
   * TRGO remains configured but is not used
   * because ADC trigger is disabled above.
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
  /*
   * DMA2 is needed by SPI1.
   *
   * ADC DMA is not used in this diagnostic test.
   */

  __HAL_RCC_DMA2_CLK_ENABLE();

  /*
   * SPI1 TX DMA.
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

  /*
   * GPIO clocks.
   */

  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /*
   * LCD DC.
   */

  HAL_GPIO_WritePin(
      DC_GPIO_Port,
      DC_Pin,
      GPIO_PIN_RESET
  );

  /*
   * LCD CS + RESET.
   */

  HAL_GPIO_WritePin(
      GPIOB,
      CS_Pin | RESET_Pin,
      GPIO_PIN_RESET
  );

  /*
   * Touch CS.
   */

  HAL_GPIO_WritePin(
      TCS_GPIO_Port,
      TCS_Pin,
      GPIO_PIN_RESET
  );

  /*
   * DC.
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
   * LCD CS + RESET.
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
   * Touch CS.
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
