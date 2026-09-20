#include "TouchController.h"
#include "main.h"

#include <stdint.h>
#include <stdbool.h>

/* =========================================================
 * TFT_eSPI compatible calibration data
 *
 * calData[0] = X minimum
 * calData[1] = X range
 * calData[2] = Y minimum
 * calData[3] = Y range
 * calData[4] = flags
 *
 * flags:
 *
 * bit 0 = rotate / swap XY
 * bit 1 = invert X
 * bit 2 = invert Y
 *
 * 7 = 0b111
 *
 * therefore:
 *
 * rotate   = 1
 * invert X = 1
 * invert Y = 1
 * ========================================================= */

static uint16_t calData[5] =
{
    351,
    3465,
    306,
    3446,
    7
};

/* =========================================================
 * SPI / Touch state
 * ========================================================= */

static SPI_HandleTypeDef *touch_hspi = NULL;

static GPIO_TypeDef *touch_cs_port = NULL;
static uint16_t touch_cs_pin = 0U;

/*
 * Runtime calibration.
 *
 * These values use the same meaning as TFT_eSPI:
 *
 * x0 = minimum
 * x1 = range
 *
 * y0 = minimum
 * y1 = range
 */
static uint16_t touch_x0 = 351U;
static uint16_t touch_x_range = 3465U;

static uint16_t touch_y0 = 306U;
static uint16_t touch_y_range = 3446U;

static bool touch_rotate = true;
static bool touch_invert_x = true;
static bool touch_invert_y = true;

/*
 * Calibration mode.
 *
 * true:
 *     Normal LVGL input is disabled.
 *
 * false:
 *     Normal LVGL input is active.
 */
static bool touch_calibration_mode = false;

/*
 * Last valid screen coordinate.
 */
static uint16_t last_x = 160U;
static uint16_t last_y = 120U;

/* =========================================================
 * SPI helper
 * ========================================================= */

static bool Touch_SPI_WaitNotBusy(void)
{
    if (touch_hspi == NULL)
    {
        return false;
    }

    for (uint32_t i = 0U; i < 100000U; i++)
    {
        if (
            __HAL_SPI_GET_FLAG(
                touch_hspi,
                SPI_FLAG_BSY
            ) == RESET
        )
        {
            return true;
        }
    }

    return false;
}

/* =========================================================
 * Slow SPI for XPT2046
 *
 * 84 MHz / 64 = 1.3125 MHz
 * ========================================================= */

static bool Touch_SPI_Slow(void)
{
    if (touch_hspi == NULL)
    {
        return false;
    }

    if (!Touch_SPI_WaitNotBusy())
    {
        return false;
    }

    __HAL_SPI_DISABLE(
        touch_hspi
    );

    MODIFY_REG(
        touch_hspi->Instance->CR1,
        SPI_CR1_BR,
        SPI_BAUDRATEPRESCALER_64
    );

    __HAL_SPI_ENABLE(
        touch_hspi
    );

    return true;
}

/* =========================================================
 * Restore LCD SPI
 *
 * 84 MHz / 8 = 10.5 MHz
 * ========================================================= */

static bool Touch_SPI_Fast(void)
{
    if (touch_hspi == NULL)
    {
        return false;
    }

    if (!Touch_SPI_WaitNotBusy())
    {
        return false;
    }

    __HAL_SPI_DISABLE(
        touch_hspi
    );

    MODIFY_REG(
        touch_hspi->Instance->CR1,
        SPI_CR1_BR,
        SPI_BAUDRATEPRESCALER_4
    );

    __HAL_SPI_ENABLE(
        touch_hspi
    );

    return true;
}

/* =========================================================
 * XPT2046 12-bit transfer
 *
 * Transaction:
 *
 * command
 * dummy
 * dummy
 *
 * ADC result is extracted from RX bytes 1 and 2.
 * ========================================================= */

static bool XPT2046_Transfer12(
    uint8_t command,
    uint16_t *value
)
{
    uint8_t tx[3] =
    {
        command,
        0x00U,
        0x00U
    };

    uint8_t rx[3] =
    {
        0x00U,
        0x00U,
        0x00U
    };

    if (
        touch_hspi == NULL ||
        value == NULL
    )
    {
        return false;
    }

    if (
        HAL_SPI_TransmitReceive(
            touch_hspi,
            tx,
            rx,
            3U,
            20U
        ) != HAL_OK
    )
    {
        return false;
    }

    uint16_t result =
        (uint16_t)(
            ((uint16_t)rx[1] << 8U) |
            rx[2]
        );

    result =
        (uint16_t)(
            (result >> 3U) &
            0x0FFFU
        );

    *value = result;

    return true;
}

/* =========================================================
 * Average two values
 * ========================================================= */

static uint16_t Average2(
    uint16_t a,
    uint16_t b
)
{
    return (uint16_t)(
        ((uint32_t)a + (uint32_t)b) / 2U
    );
}

/* =========================================================
 * Sort five samples
 * ========================================================= */

static void Sort5(
    uint16_t *values
)
{
    for (uint8_t i = 0U; i < 5U; i++)
    {
        for (
            uint8_t j = (uint8_t)(i + 1U);
            j < 5U;
            j++
        )
        {
            if (values[j] < values[i])
            {
                uint16_t temp = values[i];

                values[i] = values[j];
                values[j] = temp;
            }
        }
    }
}

/* =========================================================
 * Average middle three
 * ========================================================= */

static uint16_t AverageMiddle3(
    uint16_t *values
)
{
    Sort5(values);

    uint32_t sum =
        (uint32_t)values[1] +
        (uint32_t)values[2] +
        (uint32_t)values[3];

    return (uint16_t)(
        sum / 3U
    );
}

/* =========================================================
 * RAW TOUCH
 * ========================================================= */

bool XPT2046_GetRawTouch(
    uint16_t *x,
    uint16_t *y,
    uint16_t *pressure
)
{
    uint16_t z1 = 0U;
    uint16_t z2 = 0U;

    uint16_t x_samples[5] =
    {
        0U, 0U, 0U, 0U, 0U
    };

    uint16_t y_samples[5] =
    {
        0U, 0U, 0U, 0U, 0U
    };

    if (
        x == NULL ||
        y == NULL ||
        pressure == NULL
    )
    {
        return false;
    }

    *x = last_x;
    *y = last_y;
    *pressure = 0U;

    if (
        touch_hspi == NULL ||
        touch_cs_port == NULL
    )
    {
        return false;
    }

    /*
     * Never access SPI while LCD DMA is active.
     */
    if (
        HAL_SPI_GetState(touch_hspi)
        != HAL_SPI_STATE_READY
    )
    {
        return false;
    }

    /*
     * LCD CS inactive.
     */
    HAL_GPIO_WritePin(
        CS_GPIO_Port,
        CS_Pin,
        GPIO_PIN_SET
    );

    /*
     * XPT2046 uses lower SPI clock.
     */
    if (!Touch_SPI_Slow())
    {
        return false;
    }

    /*
     * Select touch controller.
     */
    HAL_GPIO_WritePin(
        touch_cs_port,
        touch_cs_pin,
        GPIO_PIN_RESET
    );

    /* =====================================================
     * Z1
     * ===================================================== */

    if (
        !XPT2046_Transfer12(
            XPT2046_CMD_Z1,
            &z1
        )
    )
    {
        HAL_GPIO_WritePin(
            touch_cs_port,
            touch_cs_pin,
            GPIO_PIN_SET
        );

        Touch_SPI_Fast();

        return false;
    }

    /* =====================================================
     * Z2
     * ===================================================== */

    if (
        !XPT2046_Transfer12(
            XPT2046_CMD_Z2,
            &z2
        )
    )
    {
        HAL_GPIO_WritePin(
            touch_cs_port,
            touch_cs_pin,
            GPIO_PIN_SET
        );

        Touch_SPI_Fast();

        return false;
    }

    /* =====================================================
     * Touch detection
     * ===================================================== */

    if (
        z1 == 0U ||
        z2 == 0U
    )
    {
        uint16_t dummy = 0U;

        XPT2046_Transfer12(
            XPT2046_CMD_Y_PD,
            &dummy
        );

        HAL_GPIO_WritePin(
            touch_cs_port,
            touch_cs_pin,
            GPIO_PIN_SET
        );

        Touch_SPI_Fast();

        return false;
    }

    uint32_t z =
        (uint32_t)z1 +
        4095U;

    if (
        z >= (uint32_t)z2
    )
    {
        z -= (uint32_t)z2;
    }
    else
    {
        z = 0U;
    }

    if (
        z < XPT2046_Z_THRESHOLD
    )
    {
        uint16_t dummy = 0U;

        XPT2046_Transfer12(
            XPT2046_CMD_Y_PD,
            &dummy
        );

        HAL_GPIO_WritePin(
            touch_cs_port,
            touch_cs_pin,
            GPIO_PIN_SET
        );

        Touch_SPI_Fast();

        return false;
    }

    if (z > 4095U)
    {
        z = 4095U;
    }

    *pressure = (uint16_t)z;

    /* =====================================================
     * Read X/Y
     * ===================================================== */

    for (
        uint8_t i = 0U;
        i < 5U;
        i++
    )
    {
        if (
            !XPT2046_Transfer12(
                XPT2046_CMD_X,
                &x_samples[i]
            )
        )
        {
            HAL_GPIO_WritePin(
                touch_cs_port,
                touch_cs_pin,
                GPIO_PIN_SET
            );

            Touch_SPI_Fast();

            return false;
        }

        if (
            !XPT2046_Transfer12(
                XPT2046_CMD_Y,
                &y_samples[i]
            )
        )
        {
            HAL_GPIO_WritePin(
                touch_cs_port,
                touch_cs_pin,
                GPIO_PIN_SET
            );

            Touch_SPI_Fast();

            return false;
        }
    }

    /* =====================================================
     * Power down
     * ===================================================== */

    {
        uint16_t dummy = 0U;

        XPT2046_Transfer12(
            XPT2046_CMD_Y_PD,
            &dummy
        );
    }

    /*
     * Release touch controller.
     */
    HAL_GPIO_WritePin(
        touch_cs_port,
        touch_cs_pin,
        GPIO_PIN_SET
    );

    /*
     * Restore LCD SPI speed.
     */
    if (!Touch_SPI_Fast())
    {
        return false;
    }

    /* =====================================================
     * Filter samples
     * ===================================================== */

    uint16_t raw_x =
        AverageMiddle3(
            x_samples
        );

    uint16_t raw_y =
        AverageMiddle3(
            y_samples
        );

    if (
        raw_x > 4095U ||
        raw_y > 4095U
    )
    {
        return false;
    }

    *x = raw_x;
    *y = raw_y;

    return true;
}

/* =========================================================
 * TFT_eSPI-compatible coordinate mapping
 * ========================================================= */

static uint16_t MapRawToScreen(
    uint16_t raw_x,
    uint16_t raw_y,
    uint16_t *screen_x,
    uint16_t *screen_y
)
{
    uint32_t xx;
    uint32_t yy;

    if (
        screen_x == NULL ||
        screen_y == NULL
    )
    {
        return false;
    }

    if (
        touch_x_range == 0U ||
        touch_y_range == 0U
    )
    {
        return false;
    }

    /*
     * TFT_eSPI style transformation.
     *
     * Without rotate:
     *
     *   X = raw X
     *   Y = raw Y
     *
     * With rotate:
     *
     *   X = raw Y
     *   Y = raw X
     */
    if (!touch_rotate)
    {
        /*
         * Prevent unsigned underflow.
         */
        uint16_t x =
            (raw_x > touch_x0)
                ? (uint16_t)(raw_x - touch_x0)
                : 0U;

        uint16_t y =
            (raw_y > touch_y0)
                ? (uint16_t)(raw_y - touch_y0)
                : 0U;

        xx =
            ((uint32_t)x *
             (TOUCH_SCREEN_WIDTH - 1U))
            /
            touch_x_range;

        yy =
            ((uint32_t)y *
             (TOUCH_SCREEN_HEIGHT - 1U))
            /
            touch_y_range;
    }
    else
    {
        uint16_t x =
            (raw_y > touch_x0)
                ? (uint16_t)(raw_y - touch_x0)
                : 0U;

        uint16_t y =
            (raw_x > touch_y0)
                ? (uint16_t)(raw_x - touch_y0)
                : 0U;

        xx =
            ((uint32_t)x *
             (TOUCH_SCREEN_WIDTH - 1U))
            /
            touch_x_range;

        yy =
            ((uint32_t)y *
             (TOUCH_SCREEN_HEIGHT - 1U))
            /
            touch_y_range;
    }

    /*
     * Invert X.
     */
    if (touch_invert_x)
    {
        if (xx > TOUCH_SCREEN_WIDTH - 1U)
        {
            xx = TOUCH_SCREEN_WIDTH - 1U;
        }

        xx =
            (TOUCH_SCREEN_WIDTH - 1U) -
            xx;
    }

    /*
     * Invert Y.
     */
    if (touch_invert_y)
    {
        if (yy > TOUCH_SCREEN_HEIGHT - 1U)
        {
            yy = TOUCH_SCREEN_HEIGHT - 1U;
        }

        yy =
            (TOUCH_SCREEN_HEIGHT - 1U) -
            yy;
    }

    /*
     * Clamp.
     */
    if (xx >= TOUCH_SCREEN_WIDTH)
    {
        xx = TOUCH_SCREEN_WIDTH - 1U;
    }

    if (yy >= TOUCH_SCREEN_HEIGHT)
    {
        yy = TOUCH_SCREEN_HEIGHT - 1U;
    }

    *screen_x = (uint16_t)xx;
    *screen_y = (uint16_t)yy;

    return true;
}

/* =========================================================
 * Normal touch
 * ========================================================= */

bool XPT2046_GetTouch(
    uint16_t *x,
    uint16_t *y,
    uint16_t *pressure
)
{
    uint16_t raw_x = 0U;
    uint16_t raw_y = 0U;
    uint16_t raw_pressure = 0U;

    if (
        !XPT2046_GetRawTouch(
            &raw_x,
            &raw_y,
            &raw_pressure
        )
    )
    {
        if (pressure != NULL)
        {
            *pressure = 0U;
        }

        return false;
    }

    uint16_t screen_x = last_x;
    uint16_t screen_y = last_y;

    if (
        !MapRawToScreen(
            raw_x,
            raw_y,
            &screen_x,
            &screen_y
        )
    )
    {
        return false;
    }

    last_x = screen_x;
    last_y = screen_y;

    if (x != NULL)
    {
        *x = screen_x;
    }

    if (y != NULL)
    {
        *y = screen_y;
    }

    if (pressure != NULL)
    {
        *pressure = raw_pressure;
    }

    return true;
}

/* =========================================================
 * Runtime calibration
 *
 * Parameters follow TFT_eSPI semantics:
 *
 * x_min  = calibration_x0
 * x_max  = calibration_x1 RANGE
 * y_min  = calibration_y0
 * y_max  = calibration_y1 RANGE
 *
 * ========================================================= */

void XPT2046_SetCalibration(
    uint16_t x_min,
    uint16_t x_max,
    uint16_t y_min,
    uint16_t y_max,
    bool swap_xy,
    bool invert_x,
    bool invert_y
)
{
    if (x_max != 0U)
    {
        touch_x0 = x_min;
        touch_x_range = x_max;
    }

    if (y_max != 0U)
    {
        touch_y0 = y_min;
        touch_y_range = y_max;
    }

    touch_rotate = swap_xy;

    touch_invert_x = invert_x;

    touch_invert_y = invert_y;

    /*
     * Keep calData synchronized with runtime values.
     */
    calData[0] = touch_x0;
    calData[1] = touch_x_range;

    calData[2] = touch_y0;
    calData[3] = touch_y_range;

    calData[4] =
        (touch_rotate ? 1U : 0U) |
        (touch_invert_x ? 2U : 0U) |
        (touch_invert_y ? 4U : 0U);
}

/* =========================================================
 * Calibration mode
 * ========================================================= */

void XPT2046_SetCalibrationMode(
    bool enabled
)
{
    touch_calibration_mode = enabled;
}

/* =========================================================
 * LVGL read callback
 * ========================================================= */

void XPT2046_ReadCallback(
    lv_indev_drv_t *drv,
    lv_indev_data_t *data
)
{
    (void)drv;

    if (data == NULL)
    {
        return;
    }

    /*
     * Calibration module reads the raw data itself.
     */
    if (touch_calibration_mode)
    {
        data->state =
            LV_INDEV_STATE_REL;

        data->point.x =
            (lv_coord_t)last_x;

        data->point.y =
            (lv_coord_t)last_y;

        return;
    }

    uint16_t x = last_x;
    uint16_t y = last_y;
    uint16_t pressure = 0U;

    bool touched =
        XPT2046_GetTouch(
            &x,
            &y,
            &pressure
        );

    data->point.x =
        (lv_coord_t)x;

    data->point.y =
        (lv_coord_t)y;

    if (touched)
    {
        data->state =
            LV_INDEV_STATE_PR;
    }
    else
    {
        data->state =
            LV_INDEV_STATE_REL;
    }
}

/* =========================================================
 * Initialization
 * ========================================================= */

void XPT2046_Init(
    SPI_HandleTypeDef *hspi,
    GPIO_TypeDef *cs_port,
    uint16_t cs_pin
)
{
    touch_hspi = hspi;

    touch_cs_port = cs_port;

    touch_cs_pin = cs_pin;

    /*
     * Load the initial calData exactly.
     */
    touch_x0 =
        calData[0];

    touch_x_range =
        calData[1];

    touch_y0 =
        calData[2];

    touch_y_range =
        calData[3];

    /*
     * Decode TFT_eSPI flags.
     */
    touch_rotate =
        ((calData[4] & 0x01U) != 0U);

    touch_invert_x =
        ((calData[4] & 0x02U) != 0U);

    touch_invert_y =
        ((calData[4] & 0x04U) != 0U);

    /*
     * Calibration mode OFF.
     */
    touch_calibration_mode = false;

    /*
     * Default last coordinates.
     */
    last_x = 160U;
    last_y = 120U;

    /*
     * Touch CS inactive.
     */
    HAL_GPIO_WritePin(
        touch_cs_port,
        touch_cs_pin,
        GPIO_PIN_SET
    );

    /*
     * LCD CS inactive.
     */
    HAL_GPIO_WritePin(
        CS_GPIO_Port,
        CS_Pin,
        GPIO_PIN_SET
    );
}

/* =========================================================
 * LVGL initialization
 * ========================================================= */

void XPT2046_LVGL_Init(void)
{
    static lv_indev_drv_t indev_drv;

    lv_indev_drv_init(
        &indev_drv
    );

    indev_drv.type =
        LV_INDEV_TYPE_POINTER;

    indev_drv.read_cb =
        XPT2046_ReadCallback;

    lv_indev_drv_register(
        &indev_drv
    );
}
