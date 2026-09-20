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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "lvgl.h"
#include "LCDController.h"
#include "TouchController.h"
#include "ui/ui.h"
#include "PersianText.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define LVGL_TASK_PERIOD_MS    5U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi1;

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
     * Configure system clock.
     */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /*
     * Initialize GPIO.
     */
    MX_GPIO_Init();

    /*
     * Initialize SPI1 + DMA.
     */
    MX_SPI1_Init();

    /* USER CODE BEGIN 2 */

    /* ---------------------------------------------------------------------- */
    /* LVGL                                                                    */
    /* ---------------------------------------------------------------------- */

    lv_init();

    /* ---------------------------------------------------------------------- */
    /* LCD                                                                      */
    /* ---------------------------------------------------------------------- */

    lv_port_disp_init();

    /* ---------------------------------------------------------------------- */
    /* XPT2046                                                                  */
    /* ---------------------------------------------------------------------- */

    XPT2046_Init(
        &hspi1,
        TCS_GPIO_Port,
        TCS_Pin
    );

    XPT2046_LVGL_Init();

    /* ---------------------------------------------------------------------- */
    /* EEZ Studio UI                                                           */
    /* ---------------------------------------------------------------------- */

    ui_init();
    PersianText_Init();

    /*
     * Force first screen rendering.
     */
    lv_refr_now(NULL);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    while (1)
    {
        /*
         * EEZ generated screen tick.
         */
        ui_tick();

        /*
         * LVGL:
         *
         * - input processing
         * - timers
         * - animations
         * - rendering
         * - LCD DMA flush
         */
        lv_timer_handler();

        /*
         * Small delay.
         */
        HAL_Delay(
            LVGL_TASK_PERIOD_MS
        );
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
    RCC_OscInitTypeDef RCC_OscInitStruct =
        {0};

    RCC_ClkInitTypeDef RCC_ClkInitStruct =
        {0};

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
     * PLLQ = 4
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
        ) != HAL_OK
    )
    {
        Error_Handler();
    }

    /*
     * SYSCLK = 168 MHz
     * HCLK   = 168 MHz
     * APB1   = 42 MHz
     * APB2   = 84 MHz
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
    hspi1.Instance =
        SPI1;

    /*
     * Master
     */
    hspi1.Init.Mode =
        SPI_MODE_MASTER;

    /*
     * Full duplex
     */
    hspi1.Init.Direction =
        SPI_DIRECTION_2LINES;

    /*
     * 8-bit
     */
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
     * Used by ILI9341.
     *
     * XPT2046 temporarily changes this
     * to prescaler 64.
     */
    hspi1.Init.BaudRatePrescaler =
        SPI_BAUDRATEPRESCALER_8;

    /*
     * MSB first
     */
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
  * @brief GPIO Initialization Function
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct =
        {0};

    /*
     * ----------------------------------------------------------------------
     * GPIO clocks
     * ----------------------------------------------------------------------
     */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();

    __HAL_RCC_GPIOE_CLK_ENABLE();

    /*
     * ----------------------------------------------------------------------
     * Initial LCD states
     * ----------------------------------------------------------------------
     *
     * CS    = HIGH
     * RESET = HIGH
     * DC    = HIGH
     *
     * Touch CS = HIGH
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

    HAL_GPIO_WritePin(
        GPIOE,
        GPIO_PIN_9,
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

    /*
     * ----------------------------------------------------------------------
     * PE9 = XPT2046 TCS
     * ----------------------------------------------------------------------
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_9;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        GPIOE,
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

void assert_failed(
    uint8_t *file,
    uint32_t line)
{
    (void)file;
    (void)line;
}

#endif /* USE_FULL_ASSERT */
