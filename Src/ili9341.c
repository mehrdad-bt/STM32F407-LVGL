/*
 * ili9341.c
 *
 * Based on the previously working ILI9341 driver.
 */

#include "ili9341.h"

#include <stdlib.h>

/* -------------------------------------------------------------------------- */
/* Local handles                                                              */
/* -------------------------------------------------------------------------- */

static SPI_HandleTypeDef *hspi = NULL;

static GPIO_TypeDef *CS_port = NULL;
static uint16_t CS_pin = 0;

static GPIO_TypeDef *DC_port = NULL;
static uint16_t DC_pin = 0;

static GPIO_TypeDef *RESET_port = NULL;
static uint16_t RESET_pin = 0;

/* -------------------------------------------------------------------------- */
/* Write command                                                              */
/* -------------------------------------------------------------------------- */

void ILI9341_WriteCommand(uint8_t cmd)
{
    /*
     * DC LOW = command
     */
    HAL_GPIO_WritePin(
            DC_port,
            DC_pin,
            GPIO_PIN_RESET
    );

    /*
     * CS LOW = select LCD
     */
    HAL_GPIO_WritePin(
            CS_port,
            CS_pin,
            GPIO_PIN_RESET
    );

    HAL_SPI_Transmit(
            hspi,
            &cmd,
            1,
            HAL_MAX_DELAY
    );

    /*
     * CS HIGH = deselect LCD
     */
    HAL_GPIO_WritePin(
            CS_port,
            CS_pin,
            GPIO_PIN_SET
    );
}

/* -------------------------------------------------------------------------- */
/* Write 8-bit data                                                           */
/* -------------------------------------------------------------------------- */

void ILI9341_WriteData(uint8_t data)
{
    /*
     * DC HIGH = data
     */
    HAL_GPIO_WritePin(
            DC_port,
            DC_pin,
            GPIO_PIN_SET
    );

    /*
     * Select LCD
     */
    HAL_GPIO_WritePin(
            CS_port,
            CS_pin,
            GPIO_PIN_RESET
    );

    HAL_SPI_Transmit(
            hspi,
            &data,
            1,
            HAL_MAX_DELAY
    );

    /*
     * Deselect LCD
     */
    HAL_GPIO_WritePin(
            CS_port,
            CS_pin,
            GPIO_PIN_SET
    );
}

/* -------------------------------------------------------------------------- */
/* Write 16-bit RGB565                                                        */
/* -------------------------------------------------------------------------- */

void ILI9341_WriteData16(uint16_t data)
{
    uint8_t buffer[2];

    /*
     * ILI9341 wants MSB first.
     */
    buffer[0] = (uint8_t)(data >> 8);
    buffer[1] = (uint8_t)(data & 0xFF);

    HAL_GPIO_WritePin(
            DC_port,
            DC_pin,
            GPIO_PIN_SET
    );

    HAL_GPIO_WritePin(
            CS_port,
            CS_pin,
            GPIO_PIN_RESET
    );

    HAL_SPI_Transmit(
            hspi,
            buffer,
            2,
            HAL_MAX_DELAY
    );

    HAL_GPIO_WritePin(
            CS_port,
            CS_pin,
            GPIO_PIN_SET
    );
}

/* -------------------------------------------------------------------------- */
/* Set address window                                                         */
/* -------------------------------------------------------------------------- */

void ILI9341_SetAddressWindow(
        uint16_t x0,
        uint16_t y0,
        uint16_t x1,
        uint16_t y1)
{
    /*
     * Column address
     */
    ILI9341_WriteCommand(
            ILI9341_CASET
    );

    ILI9341_WriteData(
            (uint8_t)(x0 >> 8)
    );

    ILI9341_WriteData(
            (uint8_t)(x0 & 0xFF)
    );

    ILI9341_WriteData(
            (uint8_t)(x1 >> 8)
    );

    ILI9341_WriteData(
            (uint8_t)(x1 & 0xFF)
    );

    /*
     * Page address
     */
    ILI9341_WriteCommand(
            ILI9341_PASET
    );

    ILI9341_WriteData(
            (uint8_t)(y0 >> 8)
    );

    ILI9341_WriteData(
            (uint8_t)(y0 & 0xFF)
    );

    ILI9341_WriteData(
            (uint8_t)(y1 >> 8)
    );

    ILI9341_WriteData(
            (uint8_t)(y1 & 0xFF)
    );

    /*
     * Start memory write
     */
    ILI9341_WriteCommand(
            ILI9341_RAMWR
    );
}

/* -------------------------------------------------------------------------- */
/* Fill screen                                                                */
/* -------------------------------------------------------------------------- */

void ILI9341_FillScreen(uint16_t color)
{
    ILI9341_SetAddressWindow(
            0,
            0,
            ILI9341_WIDTH - 1,
            ILI9341_HEIGHT - 1
    );

    for (
            uint32_t i = 0;
            i < ((uint32_t)ILI9341_WIDTH *
                 (uint32_t)ILI9341_HEIGHT);
            i++)
    {
        ILI9341_WriteData16(
                color
        );
    }
}

/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

void ILI9341_Init(
        SPI_HandleTypeDef *hspi_instance,
        GPIO_TypeDef *CS_port_instance,
        uint16_t CS_pin_instance,
        GPIO_TypeDef *DC_port_instance,
        uint16_t DC_pin_instance,
        GPIO_TypeDef *RESET_port_instance,
        uint16_t RESET_pin_instance)
{
    /*
     * Save handles and pins.
     */
    hspi = hspi_instance;

    CS_port = CS_port_instance;
    CS_pin = CS_pin_instance;

    DC_port = DC_port_instance;
    DC_pin = DC_pin_instance;

    RESET_port = RESET_port_instance;
    RESET_pin = RESET_pin_instance;

    /*
     * Initial states
     */
    HAL_GPIO_WritePin(
            CS_port,
            CS_pin,
            GPIO_PIN_SET
    );

    HAL_GPIO_WritePin(
            DC_port,
            DC_pin,
            GPIO_PIN_SET
    );

    /*
     * --------------------------------------------------------------
     * Hardware reset
     * --------------------------------------------------------------
     */
    HAL_GPIO_WritePin(
            RESET_port,
            RESET_pin,
            GPIO_PIN_RESET
    );

    HAL_Delay(10);

    HAL_GPIO_WritePin(
            RESET_port,
            RESET_pin,
            GPIO_PIN_SET
    );

    HAL_Delay(120);

    /*
     * --------------------------------------------------------------
     * Software reset
     * --------------------------------------------------------------
     */
    ILI9341_WriteCommand(
            ILI9341_SWRESET
    );

    HAL_Delay(150);

    /*
     * --------------------------------------------------------------
     * Power control 1
     * --------------------------------------------------------------
     */
    ILI9341_WriteCommand(
            ILI9341_PWCTR1
    );

    ILI9341_WriteData(
            0x23
    );

    /*
     * --------------------------------------------------------------
     * Power control 2
     * --------------------------------------------------------------
     */
    ILI9341_WriteCommand(
            ILI9341_PWCTR2
    );

    ILI9341_WriteData(
            0x10
    );

    /*
     * --------------------------------------------------------------
     * VCOM control 1
     * --------------------------------------------------------------
     */
    ILI9341_WriteCommand(
            ILI9341_VMCTR1
    );

    ILI9341_WriteData(
            0x3E
    );

    ILI9341_WriteData(
            0x28
    );

    /*
     * --------------------------------------------------------------
     * VCOM control 2
     * --------------------------------------------------------------
     */
    ILI9341_WriteCommand(
            ILI9341_VMCTR2
    );

    ILI9341_WriteData(
            0x86
    );

    /*
     * --------------------------------------------------------------
     * Memory access control
     *
     * 0x28 = Landscape + BGR
     * --------------------------------------------------------------
     */
    ILI9341_WriteCommand(
            ILI9341_MADCTL
    );

    ILI9341_WriteData(
            0x28
    );

    /*
     * --------------------------------------------------------------
     * Pixel format
     *
     * 0x55 = 16 bit RGB565
     * --------------------------------------------------------------
     */
    ILI9341_WriteCommand(
            ILI9341_PIXFMT
    );

    ILI9341_WriteData(
            0x55
    );

    /*
     * --------------------------------------------------------------
     * Sleep out
     * --------------------------------------------------------------
     */
    ILI9341_WriteCommand(
            ILI9341_SLPOUT
    );

    HAL_Delay(120);

    /*
     * --------------------------------------------------------------
     * Normal display mode
     * --------------------------------------------------------------
     */
    ILI9341_WriteCommand(
            ILI9341_NORON
    );

    HAL_Delay(10);

    /*
     * --------------------------------------------------------------
     * Display ON
     * --------------------------------------------------------------
     */
    ILI9341_WriteCommand(
            ILI9341_DISPON
    );

    HAL_Delay(120);
}

/* -------------------------------------------------------------------------- */
/* Draw pixel                                                                 */
/* -------------------------------------------------------------------------- */

void ILI9341_DrawPixel(
        uint16_t x,
        uint16_t y,
        uint16_t color)
{
    if (
            x >= ILI9341_WIDTH ||
            y >= ILI9341_HEIGHT)
    {
        return;
    }

    ILI9341_SetAddressWindow(
            x,
            y,
            x,
            y
    );

    ILI9341_WriteData16(
            color
    );
}

/* -------------------------------------------------------------------------- */
/* Fill rectangle                                                             */
/* -------------------------------------------------------------------------- */

void ILI9341_FillRectangle(
        uint16_t x,
        uint16_t y,
        uint16_t w,
        uint16_t h,
        uint16_t color)
{
    if (
            x >= ILI9341_WIDTH ||
            y >= ILI9341_HEIGHT ||
            w == 0 ||
            h == 0)
    {
        return;
    }

    if ((x + w) > ILI9341_WIDTH)
    {
        w = ILI9341_WIDTH - x;
    }

    if ((y + h) > ILI9341_HEIGHT)
    {
        h = ILI9341_HEIGHT - y;
    }

    ILI9341_SetAddressWindow(
            x,
            y,
            x + w - 1,
            y + h - 1
    );

    uint32_t pixels =
            (uint32_t)w *
            (uint32_t)h;

    uint8_t color_data[2];

    color_data[0] =
            (uint8_t)(color >> 8);

    color_data[1] =
            (uint8_t)(color & 0xFF);

    /*
     * Keep CS LOW for the whole rectangle.
     */
    HAL_GPIO_WritePin(
            DC_port,
            DC_pin,
            GPIO_PIN_SET
    );

    HAL_GPIO_WritePin(
            CS_port,
            CS_pin,
            GPIO_PIN_RESET
    );

    while (pixels--)
    {
        HAL_SPI_Transmit(
                hspi,
                color_data,
                2,
                HAL_MAX_DELAY
        );
    }

    HAL_GPIO_WritePin(
            CS_port,
            CS_pin,
            GPIO_PIN_SET
    );
}

/* -------------------------------------------------------------------------- */
/* Draw image                                                                 */
/* -------------------------------------------------------------------------- */

void ILI9341_DrawImage(
        uint16_t x,
        uint16_t y,
        uint16_t w,
        uint16_t h,
        const uint16_t *img)
{
    if (img == NULL)
    {
        return;
    }

    if (
            x >= ILI9341_WIDTH ||
            y >= ILI9341_HEIGHT)
    {
        return;
    }

    if ((x + w) > ILI9341_WIDTH)
    {
        w = ILI9341_WIDTH - x;
    }

    if ((y + h) > ILI9341_HEIGHT)
    {
        h = ILI9341_HEIGHT - y;
    }

    ILI9341_SetAddressWindow(
            x,
            y,
            x + w - 1,
            y + h - 1
    );

    for (
            uint32_t i = 0;
            i < ((uint32_t)w * h);
            i++)
    {
        ILI9341_WriteData16(
                img[i]
        );
    }
}

/* -------------------------------------------------------------------------- */
/* Line                                                                       */
/* -------------------------------------------------------------------------- */

void ILI9341_DrawLine(
        uint16_t x0,
        uint16_t y0,
        uint16_t x1,
        uint16_t y1,
        uint16_t color)
{
    int16_t dx =
            abs((int16_t)x1 - (int16_t)x0);

    int16_t sx =
            x0 < x1 ? 1 : -1;

    int16_t dy =
            -abs((int16_t)y1 - (int16_t)y0);

    int16_t sy =
            y0 < y1 ? 1 : -1;

    int16_t err =
            dx + dy;

    while (1)
    {
        ILI9341_DrawPixel(
                x0,
                y0,
                color
        );

        if (
                x0 == x1 &&
                y0 == y1)
        {
            break;
        }

        int16_t e2 =
                2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Fill circle                                                                */
/* -------------------------------------------------------------------------- */

void ILI9341_FillCircle(
        uint16_t x0,
        uint16_t y0,
        uint16_t r,
        uint16_t color)
{
    int16_t f =
            1 - (int16_t)r;

    int16_t ddF_x =
            1;

    int16_t ddF_y =
            -2 * (int16_t)r;

    int16_t x =
            0;

    int16_t y =
            (int16_t)r;

    ILI9341_DrawLine(
            x0 - r,
            y0,
            x0 + r,
            y0,
            color
    );

    while (x < y)
    {
        if (f >= 0)
        {
            y--;

            ddF_y += 2;

            f += ddF_y;
        }

        x++;

        ddF_x += 2;

        f += ddF_x;

        ILI9341_DrawLine(
                x0 - x,
                y0 + y,
                x0 + x,
                y0 + y,
                color
        );

        ILI9341_DrawLine(
                x0 - x,
                y0 - y,
                x0 + x,
                y0 - y,
                color
        );

        ILI9341_DrawLine(
                x0 - y,
                y0 + x,
                x0 + y,
                y0 + x,
                color
        );

        ILI9341_DrawLine(
                x0 - y,
                y0 - x,
                x0 + y,
                y0 - x,
                color
        );
    }
}

/* -------------------------------------------------------------------------- */
/* Rectangle outline                                                          */
/* -------------------------------------------------------------------------- */

void ILI9341_DrawRectangle(
        uint16_t x,
        uint16_t y,
        uint16_t w,
        uint16_t h,
        uint16_t color)
{
    if (
            w == 0 ||
            h == 0 ||
            x >= ILI9341_WIDTH ||
            y >= ILI9341_HEIGHT)
    {
        return;
    }

    if ((x + w) > ILI9341_WIDTH)
    {
        w = ILI9341_WIDTH - x;
    }

    if ((y + h) > ILI9341_HEIGHT)
    {
        h = ILI9341_HEIGHT - y;
    }

    ILI9341_FillRectangle(
            x,
            y,
            w,
            1,
            color
    );

    ILI9341_FillRectangle(
            x,
            y + h - 1,
            w,
            1,
            color
    );

    ILI9341_FillRectangle(
            x,
            y,
            1,
            h,
            color
    );

    ILI9341_FillRectangle(
            x + w - 1,
            y,
            1,
            h,
            color
    );
}
