/**
  ******************************************************************************
  * @file    stm32f4xx_hal_msp.c
  ******************************************************************************
  */

#include "main.h"

/*
 * Defined in main.c
 */
extern DMA_HandleTypeDef hdma_spi1_tx;

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hspi->Instance == SPI1)
    {
        /*
         * SPI1 clock
         */
        __HAL_RCC_SPI1_CLK_ENABLE();

        /*
         * GPIOA clock
         */
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /*
         * SPI1:
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
         * DMA2 clock
         */
        __HAL_RCC_DMA2_CLK_ENABLE();

        /*
         * SPI1 TX:
         *
         * DMA2 Stream3 Channel3
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
                DMA_PRIORITY_HIGH;

        hdma_spi1_tx.Init.FIFOMode =
                DMA_FIFOMODE_DISABLE;

        if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
        {
            Error_Handler();
        }

        /*
         * Link DMA to SPI
         */
        __HAL_LINKDMA(
                hspi,
                hdmatx,
                hdma_spi1_tx
        );
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
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
    }
}
