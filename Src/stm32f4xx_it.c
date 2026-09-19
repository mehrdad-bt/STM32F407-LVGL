/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"

/* Private variables ---------------------------------------------------------*/

extern DMA_HandleTypeDef hdma_spi1_tx;

/******************************************************************************/
/* Cortex-M4 Processor Interruption and Exception Handlers                   */
/******************************************************************************/

void NMI_Handler(void)
{
    while (1)
    {
    }
}

void HardFault_Handler(void)
{
    while (1)
    {
    }
}

void MemManage_Handler(void)
{
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

/******************************************************************************/
/* Peripheral Interrupt Handlers                                             */
/******************************************************************************/

/*
 * SPI1 TX DMA
 *
 * DMA2 Stream3 Channel3
 */
void DMA2_Stream3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(
            &hdma_spi1_tx
    );
}
