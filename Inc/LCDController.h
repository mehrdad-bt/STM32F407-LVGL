/* ============================================================
 * File: Core/Inc/LCDController.h
 * ============================================================ */

#ifndef __LCDCONTROLLER_H
#define __LCDCONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void lv_port_disp_init(void);

void disp_enable_update(void);
void disp_disable_update(void);

#ifdef __cplusplus
}
#endif

#endif
