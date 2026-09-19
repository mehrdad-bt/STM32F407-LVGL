#ifndef ILI9341_H
#define ILI9341_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* ILI9341 commands                                                           */
/* -------------------------------------------------------------------------- */

#define ILI9341_NOP             0x00
#define ILI9341_SWRESET         0x01
#define ILI9341_RDDID           0x04
#define ILI9341_RDDST           0x09
#define ILI9341_RDDPM           0x0A
#define ILI9341_RDDMADCTL       0x0B
#define ILI9341_RDDCOLMOD       0x0C
#define ILI9341_RDDIM           0x0D
#define ILI9341_RDDSM           0x0E
#define ILI9341_RDDSDR          0x0F

#define ILI9341_SLPIN           0x10
#define ILI9341_SLPOUT          0x11
#define ILI9341_PTLON           0x12
#define ILI9341_NORON           0x13

#define ILI9341_INVOFF          0x20
#define ILI9341_INVON           0x21

#define ILI9341_GAMMASET        0x26

#define ILI9341_DISPOFF         0x28
#define ILI9341_DISPON          0x29

#define ILI9341_CASET           0x2A
#define ILI9341_PASET           0x2B
#define ILI9341_RAMWR           0x2C
#define ILI9341_RAMRD           0x2E

#define ILI9341_PLTAR           0x30
#define ILI9341_TEOFF           0x34
#define ILI9341_TEON            0x35

#define ILI9341_MADCTL          0x36
#define ILI9341_VSCRSADD        0x37

#define ILI9341_IDMOFF          0x38
#define ILI9341_IDMON           0x39

#define ILI9341_PIXFMT          0x3A

#define ILI9341_WRMEMC          0x3C
#define ILI9341_RDMEMC          0x3E

#define ILI9341_STE             0x44
#define ILI9341_GSCAN           0x45

#define ILI9341_WRDISBV         0x51
#define ILI9341_RDDISBV         0x52
#define ILI9341_WRCTRLD         0x53
#define ILI9341_RDCTRLD         0x54

#define ILI9341_RDID1           0xDA
#define ILI9341_RDID2           0xDB
#define ILI9341_RDID3           0xDC
#define ILI9341_RDID4           0xD3

#define ILI9341_PWCTR1          0xC0
#define ILI9341_PWCTR2          0xC1
#define ILI9341_VMCTR1          0xC5
#define ILI9341_VMCTR2          0xC7

#define ILI9341_PGAMMA          0xE0
#define ILI9341_NGAMMA          0xE1

#define ILI9341_DGAMCTRL1       0xE2
#define ILI9341_DGAMCTRL2       0xE3

#define ILI9341_IFCTL           0xF6

/* -------------------------------------------------------------------------- */
/* Colors                                                                     */
/* -------------------------------------------------------------------------- */

#define COLOR_BLACK             0x0000
#define COLOR_WHITE             0xFFFF
#define COLOR_RED               0xF800
#define COLOR_GREEN             0x07E0
#define COLOR_BLUE              0x001F
#define COLOR_CYAN              0x07FF
#define COLOR_MAGENTA           0xF81F
#define COLOR_YELLOW            0xFFE0

/* -------------------------------------------------------------------------- */
/* Resolution                                                                 */
/* -------------------------------------------------------------------------- */

#define ILI9341_WIDTH           320
#define ILI9341_HEIGHT          240

/* -------------------------------------------------------------------------- */
/* Driver API                                                                 */
/* -------------------------------------------------------------------------- */

void ILI9341_Init(
        SPI_HandleTypeDef *hspi_instance,
        GPIO_TypeDef *CS_port_instance,
        uint16_t CS_pin_instance,
        GPIO_TypeDef *DC_port_instance,
        uint16_t DC_pin_instance,
        GPIO_TypeDef *RESET_port_instance,
        uint16_t RESET_pin_instance
);

void ILI9341_WriteCommand(
        uint8_t cmd
);

void ILI9341_WriteData(
        uint8_t data
);

void ILI9341_WriteData16(
        uint16_t data
);

void ILI9341_SetAddressWindow(
        uint16_t x0,
        uint16_t y0,
        uint16_t x1,
        uint16_t y1
);

void ILI9341_FillScreen(
        uint16_t color
);

/* -------------------------------------------------------------------------- */
/* DMA API                                                                    */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef ILI9341_StartDMATransmit(
        uint8_t *data,
        uint16_t size
);

void ILI9341_DMA_End(void);

/* -------------------------------------------------------------------------- */
/* Graphics API                                                               */
/* -------------------------------------------------------------------------- */

void ILI9341_DrawPixel(
        uint16_t x,
        uint16_t y,
        uint16_t color
);

void ILI9341_FillRectangle(
        uint16_t x,
        uint16_t y,
        uint16_t w,
        uint16_t h,
        uint16_t color
);

void ILI9341_DrawImage(
        uint16_t x,
        uint16_t y,
        uint16_t w,
        uint16_t h,
        const uint16_t *img
);

void ILI9341_DrawLine(
        uint16_t x0,
        uint16_t y0,
        uint16_t x1,
        uint16_t y1,
        uint16_t color
);

void ILI9341_FillCircle(
        uint16_t x0,
        uint16_t y0,
        uint16_t r,
        uint16_t color
);

void ILI9341_DrawRectangle(
        uint16_t x,
        uint16_t y,
        uint16_t w,
        uint16_t h,
        uint16_t color
);

#endif /* ILI9341_H */
