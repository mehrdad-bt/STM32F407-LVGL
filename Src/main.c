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

#include "lvgl.h"
#include "LCDController.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define LCD_WIDTH       320U
#define LCD_HEIGHT      240U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi1;

/*
 * DMA2 Stream3
 *
 * SPI1 TX DMA handle.
 */
DMA_HandleTypeDef hdma_spi1_tx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

static void MX_GPIO_Init(void);

static void MX_SPI1_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Nothing here for LVGL tick.
 *
 * LVGL 8.x receives its 1 ms tick from:
 *
 * SysTick_Handler()
 *
 * in stm32f4xx_it.c
 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /*
     * System Clock
     */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /*
     * Initialize GPIO
     */
    MX_GPIO_Init();

    /*
     * Initialize SPI1
     *
     * DMA2 Stream3 is initialized by:
     *
     * HAL_SPI_MspInit()
     */
    MX_SPI1_Init();

    /* USER CODE BEGIN 2 */

    /* ---------------------------------------------------------------------- */
    /* LVGL initialization                                                    */
    /* ---------------------------------------------------------------------- */

    lv_init();

    /* ---------------------------------------------------------------------- */
    /* LCD + LVGL display driver                                              */
    /* ---------------------------------------------------------------------- */

    lv_port_disp_init();

    /*
     * At this point:
     *
     * SPI1
     * DMA
     * ILI9341
     * LVGL display driver
     *
     * are initialized.
     */

    /* ---------------------------------------------------------------------- */
    /* Test screen                                                             */
    /* ---------------------------------------------------------------------- */

    lv_obj_t *screen = lv_scr_act();

    /*
     * ----------------------------------------------------------------------
     * Background
     * ----------------------------------------------------------------------
     */

    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0x000000),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        screen,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    /*
     * ----------------------------------------------------------------------
     * RED BOX
     * ----------------------------------------------------------------------
     */

    lv_obj_t *red_box =
        lv_obj_create(screen);

    lv_obj_set_size(
        red_box,
        220,
        50
    );

    lv_obj_align(
        red_box,
        LV_ALIGN_TOP_MID,
        0,
        15
    );

    lv_obj_set_style_bg_color(
        red_box,
        lv_color_hex(0xFF0000),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        red_box,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    lv_obj_set_style_border_width(
        red_box,
        0,
        LV_PART_MAIN
    );

    /*
     * ----------------------------------------------------------------------
     * GREEN BOX
     * ----------------------------------------------------------------------
     */

    lv_obj_t *green_box =
        lv_obj_create(screen);

    lv_obj_set_size(
        green_box,
        220,
        50
    );

    lv_obj_align(
        green_box,
        LV_ALIGN_CENTER,
        0,
        0
    );

    lv_obj_set_style_bg_color(
        green_box,
        lv_color_hex(0x00FF00),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        green_box,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    lv_obj_set_style_border_width(
        green_box,
        0,
        LV_PART_MAIN
    );

    /*
     * ----------------------------------------------------------------------
     * BLUE BOX
     * ----------------------------------------------------------------------
     */

    lv_obj_t *blue_box =
        lv_obj_create(screen);

    lv_obj_set_size(
        blue_box,
        220,
        50
    );

    lv_obj_align(
        blue_box,
        LV_ALIGN_BOTTOM_MID,
        0,
        -15
    );

    lv_obj_set_style_bg_color(
        blue_box,
        lv_color_hex(0x0000FF),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        blue_box,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    lv_obj_set_style_border_width(
        blue_box,
        0,
        LV_PART_MAIN
    );

    /*
     * ----------------------------------------------------------------------
     * WHITE LABEL
     * ----------------------------------------------------------------------
     */

    lv_obj_t *label =
        lv_label_create(screen);

    lv_label_set_text(
        label,
        "LVGL + DMA"
    );

    lv_obj_set_style_text_color(
        label,
        lv_color_hex(0xFFFFFF),
        LV_PART_MAIN
    );

    lv_obj_align(
        label,
        LV_ALIGN_CENTER,
        0,
        0
    );

    /*
     * Force immediate rendering.
     */
    lv_refr_now(NULL);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    while (1)
    {
        /*
         * LVGL timer handler.
         *
         * When LVGL needs to redraw the screen,
         * LCDController.c starts SPI DMA.
         */
        lv_timer_handler();

        /*
         * Small delay.
         */
        HAL_Delay(5);
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
     * ----------------------------------------------------------------------
     * Power
     * ----------------------------------------------------------------------
     */

    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(
        PWR_REGULATOR_VOLTAGE_SCALE1
    );

    /*
     * ----------------------------------------------------------------------
     * HSI + PLL
     *
     * HSI    = 16 MHz
     *
     * PLLM   = 8
     * PLLN   = 168
     * PLLP   = 2
     * PLLQ   = 4
     *
     * SYSCLK = 168 MHz
     * ----------------------------------------------------------------------
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

    RCC_OscInitStruct.PLL.PLLM =
        8;

    RCC_OscInitStruct.PLL.PLLN =
        168;

    RCC_OscInitStruct.PLL.PLLP =
        RCC_PLLP_DIV2;

    RCC_OscInitStruct.PLL.PLLQ =
        4;

    if (
        HAL_RCC_OscConfig(
            &RCC_OscInitStruct
        ) != HAL_OK
    )
    {
        Error_Handler();
    }

    /*
     * ----------------------------------------------------------------------
     * Clocks
     *
     * SYSCLK = 168 MHz
     * HCLK   = 168 MHz
     * APB1   = 42 MHz
     * APB2   = 84 MHz
     * ----------------------------------------------------------------------
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
  * @brief SPI1 Initialization Function
  * @retval None
  */
static void MX_SPI1_Init(void)
{
    /*
     * SPI1
     */
    hspi1.Instance =
        SPI1;

    /*
     * Master
     */
    hspi1.Init.Mode =
        SPI_MODE_MASTER;

    /*
     * 2-line
     */
    hspi1.Init.Direction =
        SPI_DIRECTION_2LINES;

    /*
     * 8-bit
     */
    hspi1.Init.DataSize =
        SPI_DATASIZE_8BIT;

    /*
     * SPI mode 0
     *
     * CPOL = 0
     * CPHA = 0
     */
    hspi1.Init.CLKPolarity =
        SPI_POLARITY_LOW;

    hspi1.Init.CLKPhase =
        SPI_PHASE_1EDGE;

    /*
     * Software NSS
     */
    hspi1.Init.NSS =
        SPI_NSS_SOFT;

    /*
     * APB2 = 84 MHz
     *
     * 84 / 8 = 10.5 MHz
     */
    hspi1.Init.BaudRatePrescaler =
        SPI_BAUDRATEPRESCALER_8;

    /*
     * MSB first
     */
    hspi1.Init.FirstBit =
        SPI_FIRSTBIT_MSB;

    /*
     * Motorola mode
     */
    hspi1.Init.TIMode =
        SPI_TIMODE_DISABLE;

    /*
     * CRC disabled
     */
    hspi1.Init.CRCCalculation =
        SPI_CRCCALCULATION_DISABLE;

    hspi1.Init.CRCPolynomial =
        10;

    /*
     * HAL_SPI_Init()
     *
     * calls HAL_SPI_MspInit()
     *
     * which configures:
     *
     * PA5 = SCK
     * PA6 = MISO
     * PA7 = MOSI
     *
     * and DMA2 Stream3.
     */
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
  * @brief GPIO Initialization Function
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /*
     * GPIO clocks
     */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();

    /*
     * ----------------------------------------------------------------------
     * Initial LCD states
     *
     * CS    = HIGH
     * RESET = HIGH
     * DC    = HIGH
     * ----------------------------------------------------------------------
     */

    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_0,
        GPIO_PIN_SET
    );

    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_1,
        GPIO_PIN_SET
    );

    HAL_GPIO_WritePin(
        GPIOC,
        GPIO_PIN_5,
        GPIO_PIN_SET
    );

    /*
     * ----------------------------------------------------------------------
     * PB0 = LCD CS
     * PB1 = LCD RESET
     * ----------------------------------------------------------------------
     */

    GPIO_InitStruct.Pin =
        GPIO_PIN_0 |
        GPIO_PIN_1;

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
     * ----------------------------------------------------------------------
     * PC5 = LCD DC
     * ----------------------------------------------------------------------
     */

    GPIO_InitStruct.Pin =
        GPIO_PIN_5;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        GPIOC,
        &GPIO_InitStruct
    );
}

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

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(
    uint8_t *file,
    uint32_t line)
{
    (void)file;
    (void)line;
}

#endif /* USE_FULL_ASSERT */
