/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "ili9341.h"

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi1;

/*
 * DMA handle must exist because CubeMX DMA/IRQ is enabled.
 */
DMA_HandleTypeDef hdma_spi1_tx;

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

static void MX_GPIO_Init(void);

static void MX_SPI1_Init(void);

/* -------------------------------------------------------------------------- */
/* Main                                                                       */
/* -------------------------------------------------------------------------- */

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();

    MX_SPI1_Init();

    /*
     * ILI9341 pin mapping:
     *
     * CS    = PB0
     * DC    = PC5
     * RESET = PB1
     */
    ILI9341_Init(
            &hspi1,

            GPIOB,
            GPIO_PIN_0,

            GPIOC,
            GPIO_PIN_5,

            GPIOB,
            GPIO_PIN_1
    );

    /*
     * Test colors
     */
    while (1)
    {
        ILI9341_FillScreen(
                COLOR_RED
        );

        HAL_Delay(1500);

        ILI9341_FillScreen(
                COLOR_GREEN
        );

        HAL_Delay(1500);

        ILI9341_FillScreen(
                COLOR_BLUE
        );

        HAL_Delay(1500);

        ILI9341_FillScreen(
                COLOR_WHITE
        );

        HAL_Delay(1500);

        ILI9341_FillScreen(
                COLOR_BLACK
        );

        HAL_Delay(1500);
    }
}

/* -------------------------------------------------------------------------- */
/* System Clock                                                              */
/* -------------------------------------------------------------------------- */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /*
     * Power
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
            ) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * HCLK    = 168 MHz
     * APB1    = 42 MHz
     * APB2    = 84 MHz
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
            ) != HAL_OK)
    {
        Error_Handler();
    }
}

/* -------------------------------------------------------------------------- */
/* SPI1 Initialization                                                        */
/* -------------------------------------------------------------------------- */

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

    /*
     * SPI Mode 0
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
     *
     * Same as the previously working project.
     */
    hspi1.Init.BaudRatePrescaler =
            SPI_BAUDRATEPRESCALER_8;

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
            ) != HAL_OK)
    {
        Error_Handler();
    }
}

/* -------------------------------------------------------------------------- */
/* GPIO Initialization                                                        */
/* -------------------------------------------------------------------------- */

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
     * Initial LCD levels
     *
     * CS = HIGH
     * DC = HIGH
     * RESET = HIGH
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
     * PB0 = CS
     * PB1 = RESET
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
     * PC5 = DC
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

/* -------------------------------------------------------------------------- */
/* Error Handler                                                              */
/* -------------------------------------------------------------------------- */

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
        uint32_t line)
{
    (void)file;
    (void)line;
}

#endif /* USE_FULL_ASSERT */
