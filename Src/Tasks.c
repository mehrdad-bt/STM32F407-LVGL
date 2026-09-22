#include "Tasks.h"

#include "main.h"
#include "lvgl.h"

#include "ui/ui.h"

#include "AHT10.h"
#include "UartTask.h"
#include "MainPage.h"
#include "RTC_Task.h"

#define LVGL_TASK_PERIOD_MS 5U


/**
 * @brief Initialize all application tasks.
 */
void Tasks_Init(void)
{
    UartTask_Init();
    RTC_Task_Init();

    /*
     * Future tasks can be added here:
     *
     * AdcTask_Init();
     * BuzzerTask_Init();
     * RelayTask_Init();
     * ModbusTask_Init();
     */
}


/**
 * @brief Execute all application tasks.
 */
void Tasks_Run(void)
{
    /*
     * Communication
     */
    UartTask_Run();

    /*
     * RTC / date / time
     */
    RTC_Task_Run();

    /*
     * Temperature and humidity
     */
    AHT10_Task();

    /*
     * Main page logic
     */
    MainPage_Run();

    /*
     * EEZ / LVGL
     */
    ui_tick();
    lv_timer_handler();

    /*
     * Give LVGL a regular execution period.
     */
    HAL_Delay(LVGL_TASK_PERIOD_MS);
}
