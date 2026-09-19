/* USER CODE BEGIN Header */

/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "stm32f4xx_hal.h"

/* Exported functions prototypes ---------------------------------------------*/

void Error_Handler(void);

/* Private defines -----------------------------------------------------------*/

/*
 * ILI9341 pins
 *
 * SPI1:
 * PA5 = SCK
 * PA6 = MISO
 * PA7 = MOSI
 *
 * LCD:
 * PB0 = CS
 * PC5 = DC
 * PB1 = RESET
 */

#define CS_Pin          GPIO_PIN_0
#define CS_GPIO_Port    GPIOB

#define DC_Pin          GPIO_PIN_5
#define DC_GPIO_Port    GPIOC

#define RESET_Pin       GPIO_PIN_1
#define RESET_GPIO_Port GPIOB

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
