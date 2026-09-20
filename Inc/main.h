/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro -------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/

void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

#define DC_Pin             GPIO_PIN_5
#define DC_GPIO_Port       GPIOC

#define CS_Pin             GPIO_PIN_0
#define CS_GPIO_Port       GPIOB

#define RESET_Pin          GPIO_PIN_1
#define RESET_GPIO_Port    GPIOB

/* USER CODE BEGIN Private defines */

/*
 * XPT2046 Touch Controller
 *
 * Shared SPI1:
 *
 * TDO   -> PA6 MISO
 * TDIN  -> PA7 MOSI
 * TCLK  -> PA5 SCK
 *
 * TCS   -> PE9
 *
 * TIRQ is not used.
 */
#define TCS_Pin            GPIO_PIN_9
#define TCS_GPIO_Port      GPIOE

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
