#ifndef TOUCHCONTROLLER_H
#define TOUCHCONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "lvgl.h"

#include <stdint.h>
#include <stdbool.h>

/* =========================================================
 * XPT2046 Commands
 * =========================================================
 *
 * Standard XPT2046 command bytes:
 *
 * Z1 = 0xB0
 * Z2 = 0xC0
 * X  = 0xD0
 * Y  = 0x90
 *
 * ========================================================= */

#define XPT2046_CMD_Z1       0xB0U
#define XPT2046_CMD_Z2       0xC0U
#define XPT2046_CMD_X        0xD0U
#define XPT2046_CMD_Y        0x90U

/* Y channel + power down */
#define XPT2046_CMD_Y_PD     0xD0U

/* =========================================================
 * Pressure threshold
 * ========================================================= */

#define XPT2046_Z_THRESHOLD  400U

/* =========================================================
 * Default calibration
 * ========================================================= */

#define TOUCH_X_MIN          351U
#define TOUCH_X_MAX          3465U

#define TOUCH_Y_MIN          306U
#define TOUCH_Y_MAX          3446U

/* =========================================================
 * Display resolution
 * ========================================================= */

#define TOUCH_SCREEN_WIDTH   320U
#define TOUCH_SCREEN_HEIGHT  240U

/* =========================================================
 * Default coordinate transformation
 * ========================================================= */

#define TOUCH_SWAP_XY        0U
#define TOUCH_INVERT_X       0U
#define TOUCH_INVERT_Y       0U

/* =========================================================
 * Initialization
 * ========================================================= */

void XPT2046_Init(
    SPI_HandleTypeDef *hspi,
    GPIO_TypeDef *cs_port,
    uint16_t cs_pin
);

void XPT2046_LVGL_Init(void);

/* =========================================================
 * Touch reading
 * ========================================================= */

/*
 * Read mapped screen coordinates.
 *
 * Returns:
 *
 * true  = touched
 * false = released / invalid
 */
bool XPT2046_GetTouch(
    uint16_t *x,
    uint16_t *y,
    uint16_t *pressure
);

/*
 * Read raw XPT2046 coordinates.
 *
 * x / y are 0..4095.
 */
bool XPT2046_GetRawTouch(
    uint16_t *x,
    uint16_t *y,
    uint16_t *pressure
);

/* =========================================================
 * Runtime calibration
 * ========================================================= */

void XPT2046_SetCalibration(
    uint16_t x_min,
    uint16_t x_max,
    uint16_t y_min,
    uint16_t y_max,
    bool swap_xy,
    bool invert_x,
    bool invert_y
);

/*
 * Disable normal LVGL touch reading while calibration
 * module reads raw data directly.
 */
void XPT2046_SetCalibrationMode(
    bool enabled
);

/* =========================================================
 * LVGL callback
 * ========================================================= */

void XPT2046_ReadCallback(
    lv_indev_drv_t *drv,
    lv_indev_data_t *data
);

#ifdef __cplusplus
}
#endif

#endif /* TOUCHCONTROLLER_H */
