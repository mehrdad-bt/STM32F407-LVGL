/**
  ******************************************************************************
  * @file    LCDController.c
  * @brief   LVGL display driver for ILI9341
  ******************************************************************************
  */

#include "LCDController.h"
#include "ili9341.h"
#include "main.h"

#include <stdint.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------- */
/* Resolution                                                                 */
/* -------------------------------------------------------------------------- */

#ifndef MY_DISP_HOR_RES
#define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
#define MY_DISP_VER_RES    240
#endif

/* -------------------------------------------------------------------------- */
/* Update enable                                                              */
/* -------------------------------------------------------------------------- */

volatile bool disp_flush_enabled = true;

/* -------------------------------------------------------------------------- */
/* Private functions                                                          */
/* -------------------------------------------------------------------------- */

static void disp_init(void);

static void disp_flush(
        lv_disp_drv_t *disp_drv,
        const lv_area_t *area,
        lv_color_t *color_p
);

/* -------------------------------------------------------------------------- */
/* LVGL display init                                                          */
/* -------------------------------------------------------------------------- */

void lv_port_disp_init(void)
{
    static lv_disp_draw_buf_t draw_buf_dsc;

    static lv_color_t buf_1[
            MY_DISP_HOR_RES * 10
    ];

    static lv_color_t buf_2[
            MY_DISP_HOR_RES * 10
    ];

    static lv_disp_drv_t disp_drv;

    /*
     * Initialize LCD
     */
    disp_init();

    /*
     * Draw buffers
     */
    lv_disp_draw_buf_init(
            &draw_buf_dsc,
            buf_1,
            buf_2,
            MY_DISP_HOR_RES * 10
    );

    /*
     * LVGL driver
     */
    lv_disp_drv_init(
            &disp_drv
    );

    disp_drv.hor_res =
            MY_DISP_HOR_RES;

    disp_drv.ver_res =
            MY_DISP_VER_RES;

    disp_drv.flush_cb =
            disp_flush;

    disp_drv.draw_buf =
            &draw_buf_dsc;

    /*
     * Register driver
     */
    lv_disp_drv_register(
            &disp_drv
    );
}

/* -------------------------------------------------------------------------- */
/* Enable / Disable                                                           */
/* -------------------------------------------------------------------------- */

void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/* -------------------------------------------------------------------------- */
/* LCD init                                                                   */
/* -------------------------------------------------------------------------- */

static void disp_init(void)
{
    extern SPI_HandleTypeDef hspi1;

    ILI9341_Init(
            &hspi1,

            CS_GPIO_Port,
            CS_Pin,

            DC_GPIO_Port,
            DC_Pin,

            RESET_GPIO_Port,
            RESET_Pin
    );
}

/* -------------------------------------------------------------------------- */
/* LVGL flush                                                                 */
/* -------------------------------------------------------------------------- */

static void disp_flush(
        lv_disp_drv_t *disp_drv,
        const lv_area_t *area,
        lv_color_t *color_p
)
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;

    uint16_t width;
    uint16_t height;

    uint32_t pixel_count;

    if (!disp_flush_enabled)
    {
        lv_disp_flush_ready(
                disp_drv
        );

        return;
    }

    x1 = (uint16_t)area->x1;
    y1 = (uint16_t)area->y1;
    x2 = (uint16_t)area->x2;
    y2 = (uint16_t)area->y2;

    width =
            (uint16_t)(x2 - x1 + 1U);

    height =
            (uint16_t)(y2 - y1 + 1U);

    pixel_count =
            (uint32_t)width *
            (uint32_t)height;

    /*
     * Set window
     */
    ILI9341_SetAddressWindow(
            x1,
            y1,
            x2,
            y2
    );

    /*
     * Send RGB565 pixels
     */
    for (uint32_t i = 0U;
         i < pixel_count;
         i++)
    {
        ILI9341_WriteData16(
                color_p[i].full
        );
    }

    /*
     * Flush finished
     */
    lv_disp_flush_ready(
            disp_drv
    );
}
