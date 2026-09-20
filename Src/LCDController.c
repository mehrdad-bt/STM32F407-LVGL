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
/* Display resolution                                                         */
/* -------------------------------------------------------------------------- */

#ifndef MY_DISP_HOR_RES
#define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
#define MY_DISP_VER_RES    240
#endif

/*
 * LVGL draw buffer height
 *
 * Must match the buffers below.
 */
#define LVGL_BUFFER_LINES  20U

/*
 * Maximum DMA buffer size:
 *
 * 320 pixels × 10 lines × 2 bytes
 *
 * = 6400 bytes
 */

/* -------------------------------------------------------------------------- */
/* External SPI handle                                                        */
/* -------------------------------------------------------------------------- */

extern SPI_HandleTypeDef hspi1;

/* -------------------------------------------------------------------------- */
/* LVGL driver state                                                          */
/* -------------------------------------------------------------------------- */

static lv_disp_drv_t *active_disp_drv = NULL;

static volatile bool dma_flush_active = false;

/*
 * Separate DMA buffer.
 *
 * LVGL buffer is NOT modified directly.
 *
 * This is important because LVGL owns its draw buffer.
 */


/* -------------------------------------------------------------------------- */
/* Flush update enable                                                        */
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
/* LVGL display initialization                                                */
/* -------------------------------------------------------------------------- */

void lv_port_disp_init(void)
{
    static lv_disp_draw_buf_t draw_buf_dsc;

    static lv_color_t buf_1[
        MY_DISP_HOR_RES * LVGL_BUFFER_LINES
    ];

    static lv_color_t buf_2[
        MY_DISP_HOR_RES * LVGL_BUFFER_LINES
    ];

    static lv_disp_drv_t disp_drv;

    /*
     * Initialize LCD
     */
    disp_init();

    /*
     * Initialize LVGL draw buffers
     */
    lv_disp_draw_buf_init(
            &draw_buf_dsc,
            buf_1,
            buf_2,
            MY_DISP_HOR_RES * LVGL_BUFFER_LINES
    );

    /*
     * Initialize LVGL driver
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
     * Register display driver
     */
    lv_disp_drv_register(
            &disp_drv
    );
}

/* -------------------------------------------------------------------------- */
/* Enable / disable display updates                                           */
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
/* LCD initialization                                                         */
/* -------------------------------------------------------------------------- */

static void disp_init(void)
{
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
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;

    uint32_t width;
    uint32_t height;
    uint32_t pixel_count;
    uint32_t dma_bytes;

    /*
     * Display update disabled
     */
    if (!disp_flush_enabled)
    {
        lv_disp_flush_ready(
                disp_drv
        );

        return;
    }

    /*
     * Previous DMA should already be finished.
     */
    if (dma_flush_active)
    {
        return;
    }

    /*
     * Area coordinates
     */
    x1 = area->x1;
    y1 = area->y1;
    x2 = area->x2;
    y2 = area->y2;

    /*
     * Safety clipping
     */
    if (x1 < 0)
    {
        x1 = 0;
    }

    if (y1 < 0)
    {
        y1 = 0;
    }

    if (x2 >= MY_DISP_HOR_RES)
    {
        x2 = MY_DISP_HOR_RES - 1;
    }

    if (y2 >= MY_DISP_VER_RES)
    {
        y2 = MY_DISP_VER_RES - 1;
    }

    /*
     * Invalid area
     */
    if (
            x1 > x2 ||
            y1 > y2
    )
    {
        lv_disp_flush_ready(
                disp_drv
        );

        return;
    }

    width =
            (uint32_t)(x2 - x1 + 1);

    height =
            (uint32_t)(y2 - y1 + 1);

    pixel_count =
            width * height;

    dma_bytes =
            pixel_count * 2U;

    /*
     * Set LCD address window.
     */
    ILI9341_SetAddressWindow(
            (uint16_t)x1,
            (uint16_t)y1,
            (uint16_t)x2,
            (uint16_t)y2
    );

    /*
     * Save the LVGL driver.
     *
     * DMA callback will call lv_disp_flush_ready().
     */
    active_disp_drv =
            disp_drv;

    dma_flush_active =
            true;

    /*
     * Start DMA directly from LVGL draw buffer.
     */
    if (
            ILI9341_StartDMATransmit(
                    (uint8_t *)color_p,
                    (uint16_t)dma_bytes
            ) != HAL_OK)
    {
        dma_flush_active =
                false;

        active_disp_drv =
                NULL;

        ILI9341_DMA_End();

        lv_disp_flush_ready(
                disp_drv
        );
    }
}

/* -------------------------------------------------------------------------- */
/* SPI DMA complete callback                                                  */
/* -------------------------------------------------------------------------- */

void HAL_SPI_TxCpltCallback(
        SPI_HandleTypeDef *hspi)
{
    if (
            hspi == NULL ||
            hspi->Instance != SPI1
    )
    {
        return;
    }

    /*
     * DMA transfer completed.
     *
     * Release CS.
     */
    ILI9341_DMA_End();

    /*
     * Clear DMA state.
     */
    dma_flush_active =
            false;

    /*
     * Tell LVGL that its buffer can be reused.
     */
    if (active_disp_drv != NULL)
    {
        lv_disp_drv_t *drv =
                active_disp_drv;

        active_disp_drv =
                NULL;

        lv_disp_flush_ready(
                drv
        );
    }
}

/* -------------------------------------------------------------------------- */
/* SPI DMA error callback                                                     */
/* -------------------------------------------------------------------------- */

void HAL_SPI_ErrorCallback(
        SPI_HandleTypeDef *hspi)
{
    if (
            hspi == NULL ||
            hspi->Instance != SPI1
    )
    {
        return;
    }

    /*
     * Release CS.
     */
    ILI9341_DMA_End();

    dma_flush_active =
            false;

    /*
     * Release LVGL flush even on error,
     * otherwise LVGL will remain blocked forever.
     */
    if (active_disp_drv != NULL)
    {
        lv_disp_drv_t *drv =
                active_disp_drv;

        active_disp_drv =
                NULL;

        lv_disp_flush_ready(
                drv
        );
    }
}
