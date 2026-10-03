/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32f4xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */


extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_spi1_tx;


/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */


/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */


/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */


/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */


/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */


/* USER CODE BEGIN 0 */

/* USER CODE END 0 */


/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
    /* USER CODE BEGIN MspInit 0 */

    /* USER CODE END MspInit 0 */

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* System interrupt init*/

    /* USER CODE BEGIN MspInit 1 */

    /* USER CODE END MspInit 1 */
}


/**
  * @brief ADC MSP Initialization
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hadc->Instance == ADC1)
    {
        /* USER CODE BEGIN ADC1_MspInit 0 */

        /* USER CODE END ADC1_MspInit 0 */

        /*
         * ------------------------------------------------------
         * ADC1 clock
         * ------------------------------------------------------
         */
        __HAL_RCC_ADC1_CLK_ENABLE();


        /*
         * ------------------------------------------------------
         * GPIOC clock
         * ------------------------------------------------------
         */
        __HAL_RCC_GPIOC_CLK_ENABLE();


        /*
         * ------------------------------------------------------
         * PC2 = ADC1_IN12
         * ------------------------------------------------------
         */
        GPIO_InitStruct.Pin =
            GPIO_PIN_2;

        GPIO_InitStruct.Mode =
            GPIO_MODE_ANALOG;

        GPIO_InitStruct.Pull =
            GPIO_NOPULL;

        HAL_GPIO_Init(
            GPIOC,
            &GPIO_InitStruct
        );


        /*
         * ------------------------------------------------------
         * ADC1 DMA
         * ------------------------------------------------------
         *
         * ADC1
         *   |
         *   +--> DMA2 Stream0 Channel0
         *
         * Peripheral -> Memory
         * Circular
         * Half Word
         * High priority
         */
        hdma_adc1.Instance =
            DMA2_Stream0;

        hdma_adc1.Init.Channel =
            DMA_CHANNEL_0;

        hdma_adc1.Init.Direction =
            DMA_PERIPH_TO_MEMORY;

        hdma_adc1.Init.PeriphInc =
            DMA_PINC_DISABLE;

        hdma_adc1.Init.MemInc =
            DMA_MINC_ENABLE;

        hdma_adc1.Init.PeriphDataAlignment =
            DMA_PDATAALIGN_HALFWORD;

        hdma_adc1.Init.MemDataAlignment =
            DMA_MDATAALIGN_HALFWORD;

        hdma_adc1.Init.Mode =
            DMA_CIRCULAR;

        hdma_adc1.Init.Priority =
            DMA_PRIORITY_HIGH;

        hdma_adc1.Init.FIFOMode =
            DMA_FIFOMODE_DISABLE;


        if (
            HAL_DMA_Init(
                &hdma_adc1
            ) != HAL_OK
        )
        {
            Error_Handler();
        }


        /*
         * Link DMA to ADC1.
         */
        __HAL_LINKDMA(
            hadc,
            DMA_Handle,
            hdma_adc1
        );


        /*
         * ------------------------------------------------------
         * DMA2 Stream0 interrupt
         * ------------------------------------------------------
         *
         * Half Transfer + Transfer Complete
         *
         * OscopeADC.c handles:
         *
         * HAL_ADC_ConvHalfCpltCallback()
         * HAL_ADC_ConvCpltCallback()
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
         * ADC IRQ intentionally not enabled.
         *
         * We use DMA callbacks for waveform capture.
         */


        /* USER CODE BEGIN ADC1_MspInit 1 */

        /* USER CODE END ADC1_MspInit 1 */
    }
}


/**
  * @brief ADC MSP De-Initialization
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        /* USER CODE BEGIN ADC1_MspDeInit 0 */

        /* USER CODE END ADC1_MspDeInit 0 */


        /*
         * Disable ADC1 clock.
         */
        __HAL_RCC_ADC1_CLK_DISABLE();


        /*
         * PC2 = ADC1_IN12
         */
        HAL_GPIO_DeInit(
            GPIOC,
            GPIO_PIN_2
        );


        /*
         * ADC1 DMA DeInit
         */
        HAL_DMA_DeInit(
            hadc->DMA_Handle
        );


        /*
         * DMA interrupt disable
         */
        HAL_NVIC_DisableIRQ(
            DMA2_Stream0_IRQn
        );


        /* USER CODE BEGIN ADC1_MspDeInit 1 */

        /* USER CODE END ADC1_MspDeInit 1 */
    }
}


/**
  * @brief I2C MSP Initialization
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hi2c->Instance == I2C1)
    {
        /* USER CODE BEGIN I2C1_MspInit 0 */

        /* USER CODE END I2C1_MspInit 0 */


        /*
         * GPIOB clock
         */
        __HAL_RCC_GPIOB_CLK_ENABLE();


        /*
         * I2C1
         *
         * PB6 = SCL
         * PB7 = SDA
         */
        GPIO_InitStruct.Pin =
            GPIO_PIN_6 |
            GPIO_PIN_7;

        GPIO_InitStruct.Mode =
            GPIO_MODE_AF_OD;

        GPIO_InitStruct.Pull =
            GPIO_NOPULL;

        GPIO_InitStruct.Speed =
            GPIO_SPEED_FREQ_VERY_HIGH;

        GPIO_InitStruct.Alternate =
            GPIO_AF4_I2C1;

        HAL_GPIO_Init(
            GPIOB,
            &GPIO_InitStruct
        );


        /*
         * I2C1 clock
         */
        __HAL_RCC_I2C1_CLK_ENABLE();


        /* USER CODE BEGIN I2C1_MspInit 1 */

        /* USER CODE END I2C1_MspInit 1 */
    }
}


/**
  * @brief I2C MSP De-Initialization
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        /* USER CODE BEGIN I2C1_MspDeInit 0 */

        /* USER CODE END I2C1_MspDeInit 0 */


        __HAL_RCC_I2C1_CLK_DISABLE();


        HAL_GPIO_DeInit(
            GPIOB,
            GPIO_PIN_6
        );

        HAL_GPIO_DeInit(
            GPIOB,
            GPIO_PIN_7
        );


        /* USER CODE BEGIN I2C1_MspDeInit 1 */

        /* USER CODE END I2C1_MspDeInit 1 */
    }
}


/**
  * @brief SPI MSP Initialization
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hspi->Instance == SPI1)
    {
        /* USER CODE BEGIN SPI1_MspInit 0 */

        /* USER CODE END SPI1_MspInit 0 */


        /*
         * ------------------------------------------------------
         * SPI1 clock
         * ------------------------------------------------------
         */
        __HAL_RCC_SPI1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();


        /*
         * SPI1
         *
         * PA5 = SCK
         * PA6 = MISO
         * PA7 = MOSI
         */
        GPIO_InitStruct.Pin =
            GPIO_PIN_5 |
            GPIO_PIN_6 |
            GPIO_PIN_7;

        GPIO_InitStruct.Mode =
            GPIO_MODE_AF_PP;

        GPIO_InitStruct.Pull =
            GPIO_NOPULL;

        GPIO_InitStruct.Speed =
            GPIO_SPEED_FREQ_VERY_HIGH;

        GPIO_InitStruct.Alternate =
            GPIO_AF5_SPI1;

        HAL_GPIO_Init(
            GPIOA,
            &GPIO_InitStruct
        );


        /*
         * ------------------------------------------------------
         * SPI1 TX DMA
         * ------------------------------------------------------
         */
        hdma_spi1_tx.Instance =
            DMA2_Stream3;

        hdma_spi1_tx.Init.Channel =
            DMA_CHANNEL_3;

        hdma_spi1_tx.Init.Direction =
            DMA_MEMORY_TO_PERIPH;

        hdma_spi1_tx.Init.PeriphInc =
            DMA_PINC_DISABLE;

        hdma_spi1_tx.Init.MemInc =
            DMA_MINC_ENABLE;

        hdma_spi1_tx.Init.PeriphDataAlignment =
            DMA_PDATAALIGN_BYTE;

        hdma_spi1_tx.Init.MemDataAlignment =
            DMA_MDATAALIGN_BYTE;

        hdma_spi1_tx.Init.Mode =
            DMA_NORMAL;

        hdma_spi1_tx.Init.Priority =
            DMA_PRIORITY_LOW;

        hdma_spi1_tx.Init.FIFOMode =
            DMA_FIFOMODE_DISABLE;


        if (
            HAL_DMA_Init(
                &hdma_spi1_tx
            ) != HAL_OK
        )
        {
            Error_Handler();
        }


        /*
         * Link DMA to SPI1 TX
         */
        __HAL_LINKDMA(
            hspi,
            hdmatx,
            hdma_spi1_tx
        );


        /*
         * SPI1 interrupt
         */
        HAL_NVIC_SetPriority(
            SPI1_IRQn,
            0,
            0
        );

        HAL_NVIC_EnableIRQ(
            SPI1_IRQn
        );


        /* USER CODE BEGIN SPI1_MspInit 1 */

        /* USER CODE END SPI1_MspInit 1 */
    }
}


/**
  * @brief SPI MSP De-Initialization
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        /* USER CODE BEGIN SPI1_MspDeInit 0 */

        /* USER CODE END SPI1_MspDeInit 0 */


        __HAL_RCC_SPI1_CLK_DISABLE();


        HAL_GPIO_DeInit(
            GPIOA,
            GPIO_PIN_5 |
            GPIO_PIN_6 |
            GPIO_PIN_7
        );


        HAL_DMA_DeInit(
            hspi->hdmatx
        );


        HAL_NVIC_DisableIRQ(
            SPI1_IRQn
        );


        /* USER CODE BEGIN SPI1_MspDeInit 1 */

        /* USER CODE END SPI1_MspDeInit 1 */
    }
}


/**
  * @brief TIM Base MSP Initialization
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim_base)
{
    if (htim_base->Instance == TIM2)
    {
        /* USER CODE BEGIN TIM2_MspInit 0 */

        /* USER CODE END TIM2_MspInit 0 */


        /*
         * TIM2 clock
         */
        __HAL_RCC_TIM2_CLK_ENABLE();


        /* USER CODE BEGIN TIM2_MspInit 1 */

        /* USER CODE END TIM2_MspInit 1 */
    }
}


/**
  * @brief TIM Base MSP De-Initialization
  */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim_base)
{
    if (htim_base->Instance == TIM2)
    {
        /* USER CODE BEGIN TIM2_MspDeInit 0 */

        /* USER CODE END TIM2_MspDeInit 0 */


        __HAL_RCC_TIM2_CLK_DISABLE();


        /* USER CODE BEGIN TIM2_MspDeInit 1 */

        /* USER CODE END TIM2_MspDeInit 1 */
    }
}


/**
  * @brief UART MSP Initialization
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1)
    {
        /* USER CODE BEGIN USART1_MspInit 0 */

        /* USER CODE END USART1_MspInit 0 */


        /*
         * USART1 clock
         */
        __HAL_RCC_USART1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();


        /*
         * PA9  = TX
         * PA10 = RX
         */
        GPIO_InitStruct.Pin =
            GPIO_PIN_9 |
            GPIO_PIN_10;

        GPIO_InitStruct.Mode =
            GPIO_MODE_AF_PP;

        GPIO_InitStruct.Pull =
            GPIO_NOPULL;

        GPIO_InitStruct.Speed =
            GPIO_SPEED_FREQ_VERY_HIGH;

        GPIO_InitStruct.Alternate =
            GPIO_AF7_USART1;

        HAL_GPIO_Init(
            GPIOA,
            &GPIO_InitStruct
        );


        /*
         * USART1 interrupt
         */
        HAL_NVIC_SetPriority(
            USART1_IRQn,
            0,
            0
        );

        HAL_NVIC_EnableIRQ(
            USART1_IRQn
        );


        /* USER CODE BEGIN USART1_MspInit 1 */

        /* USER CODE END USART1_MspInit 1 */
    }
}


/**
  * @brief UART MSP De-Initialization
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        /* USER CODE BEGIN USART1_MspDeInit 0 */

        /* USER CODE END USART1_MspDeInit 0 */


        __HAL_RCC_USART1_CLK_DISABLE();


        HAL_GPIO_DeInit(
            GPIOA,
            GPIO_PIN_9 |
            GPIO_PIN_10
        );


        HAL_NVIC_DisableIRQ(
            USART1_IRQn
        );


        /* USER CODE BEGIN USART1_MspDeInit 1 */

        /* USER CODE END USART1_MspDeInit 1 */
    }
}


/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
