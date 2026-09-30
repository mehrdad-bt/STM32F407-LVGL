#ifndef OSCOPE_PAGE_H
#define OSCOPE_PAGE_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

void OscopePage_OnEnter(void);
void OscopePage_OnExit(void);

void OscopePage_IncreaseSpeed(void);
void OscopePage_DecreaseSpeed(void);

#ifdef __cplusplus
}
#endif

#endif /* OSCOPE_PAGE_H */
