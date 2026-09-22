#ifndef RTC_TASK_H
#define RTC_TASK_H

#include <stdint.h>
#include <stdbool.h>

void RTC_Task_Init(void);
void RTC_Task_Run(void);

bool RTC_Task_GetTime(
    uint8_t *hours,
    uint8_t *minutes,
    uint8_t *seconds
);

bool RTC_Task_GetDate(
    uint8_t *day,
    uint8_t *month,
    uint8_t *year
);

#endif
