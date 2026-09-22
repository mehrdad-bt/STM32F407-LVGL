#include "RTC_Task.h"
#include "main.h"
#include "MainPage.h"

#include <stdint.h>
#include <stdbool.h>

#define RTC_BACKUP_MAGIC       0x20260922UL
#define RTC_UPDATE_PERIOD_MS   1000U

/*
 * Default date/time used when RTC has not been initialized before.
 */
#define RTC_DEFAULT_YEAR       26U
#define RTC_DEFAULT_MONTH      RTC_MONTH_SEPTEMBER
#define RTC_DEFAULT_DATE       22U

#define RTC_DEFAULT_HOUR       9U
#define RTC_DEFAULT_MINUTE     0U
#define RTC_DEFAULT_SECOND     0U

extern RTC_HandleTypeDef hrtc;

static uint32_t rtc_last_update_tick = 0U;


/**
 * @brief Initialize RTC application logic.
 *        Sets date/time only on first initialization.
 */
void RTC_Task_Init(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    HAL_PWR_EnableBkUpAccess();

    /*
     * If the backup register does not contain our magic value,
     * RTC has not been initialized yet.
     */
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != RTC_BACKUP_MAGIC)
    {
        /*
         * Set default time.
         */
        sTime.Hours = RTC_DEFAULT_HOUR;
        sTime.Minutes = RTC_DEFAULT_MINUTE;
        sTime.Seconds = RTC_DEFAULT_SECOND;

        sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        sTime.StoreOperation = RTC_STOREOPERATION_RESET;

        if (HAL_RTC_SetTime(
                &hrtc,
                &sTime,
                RTC_FORMAT_BIN) != HAL_OK)
        {
            Error_Handler();
        }

        /*
         * Set default date.
         */
        sDate.WeekDay = RTC_WEEKDAY_TUESDAY;
        sDate.Month = RTC_DEFAULT_MONTH;
        sDate.Date = RTC_DEFAULT_DATE;
        sDate.Year = RTC_DEFAULT_YEAR;

        if (HAL_RTC_SetDate(
                &hrtc,
                &sDate,
                RTC_FORMAT_BIN) != HAL_OK)
        {
            Error_Handler();
        }

        /*
         * Mark RTC as initialized.
         */
        HAL_RTCEx_BKUPWrite(
            &hrtc,
            RTC_BKP_DR0,
            RTC_BACKUP_MAGIC
        );
    }

    rtc_last_update_tick = HAL_GetTick();
}


/**
 * @brief Periodic RTC task.
 */
void RTC_Task_Run(void)
{
    uint32_t now;

    now = HAL_GetTick();

    if ((now - rtc_last_update_tick) < RTC_UPDATE_PERIOD_MS)
    {
        return;
    }

    rtc_last_update_tick = now;

    /*
     * Update the clock shown on the main page.
     */
    MainPage_UpdateDateTime();
}


/**
 * @brief Read current RTC time.
 */
bool RTC_Task_GetTime(
    uint8_t *hours,
    uint8_t *minutes,
    uint8_t *seconds
)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    if ((hours == NULL) ||
        (minutes == NULL) ||
        (seconds == NULL))
    {
        return false;
    }

    /*
     * IMPORTANT:
     * Always read TIME before DATE.
     */
    if (HAL_RTC_GetTime(
            &hrtc,
            &time,
            RTC_FORMAT_BIN) != HAL_OK)
    {
        return false;
    }

    /*
     * Reading DATE completes the RTC shadow-register sequence.
     */
    if (HAL_RTC_GetDate(
            &hrtc,
            &date,
            RTC_FORMAT_BIN) != HAL_OK)
    {
        return false;
    }

    *hours = time.Hours;
    *minutes = time.Minutes;
    *seconds = time.Seconds;

    return true;
}


/**
 * @brief Read current RTC date.
 */
bool RTC_Task_GetDate(
    uint8_t *day,
    uint8_t *month,
    uint8_t *year
)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    if ((day == NULL) ||
        (month == NULL) ||
        (year == NULL))
    {
        return false;
    }

    /*
     * IMPORTANT:
     * Read TIME before DATE.
     */
    if (HAL_RTC_GetTime(
            &hrtc,
            &time,
            RTC_FORMAT_BIN) != HAL_OK)
    {
        return false;
    }

    if (HAL_RTC_GetDate(
            &hrtc,
            &date,
            RTC_FORMAT_BIN) != HAL_OK)
    {
        return false;
    }

    *day = date.Date;
    *month = date.Month;
    *year = date.Year;

    return true;
}
