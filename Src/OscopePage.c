#include "OscopePage.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "lvgl.h"

#include "ui/ui.h"
#include "ui/screens.h"

#include "OscopeADC.h"


/* ==========================================================
 * Configuration
 * ========================================================== */

/*
 * تعداد نمونه‌ای که ADC فعلی برمی‌گرداند.
 */
#define OSCOPE_POINT_COUNT             300U

/*
 * ADC 12-bit
 */
#define OSCOPE_ADC_MAX                4095U

/*
 * ولتاژ مرجع ADC
 */
#define OSCOPE_ADC_VOLTAGE             3.3f

/*
 * تعداد تقسیمات عمودی/افقی
 */
#define OSCOPE_VERTICAL_DIVS           8U
#define OSCOPE_HORIZONTAL_DIVS        10U

/*
 * موقعیت Trigger روی صفحه
 * 25% یعنی Trigger کمی بعد از ابتدای صفحه دیده می‌شود.
 */
#define OSCOPE_TRIGGER_POSITION        25U


/* ==========================================================
 * Time / Division
 * ========================================================== */

/*
 * در این مرحله Time/Div بر اساس تعداد نمونه قابل نمایش
 * روی همان بلوک 300 نمونه‌ای کنترل می‌شود.
 *
 * نرخ نمونه‌برداری فعلی با Polling سخت‌افزاری ثابت نشده،
 * بنابراین این اعداد فعلاً SCALE نمایشی هستند.
 *
 * بعداً که ADC را TIM2 + DMA کنیم، همین جدول را می‌توان
 * به Time/Div واقعی تبدیل کرد.
 */
static const uint32_t oscope_time_div_us[] =
{
    20U,
    50U,
    100U,
    200U,
    500U,
    1000U
};

#define OSCOPE_TIME_DIV_COUNT \
    (sizeof(oscope_time_div_us) / sizeof(oscope_time_div_us[0]))

static uint32_t oscope_time_index = 2U;


/*
 * تعداد نمونه قابل نمایش برای هر Time/Div
 *
 * فعلاً متناسب با بلوک 300 نمونه‌ای انتخاب شده.
 */
static const uint16_t oscope_samples_per_div[] =
{
    15U,
    25U,
    30U,
    50U,
    100U,
    150U
};


/* ==========================================================
 * Volt / Division
 * ========================================================== */

static const float oscope_volt_div[] =
{
    0.1f,
    0.2f,
    0.5f,
    1.0f,
    2.0f
};

#define OSCOPE_VOLT_DIV_COUNT \
    (sizeof(oscope_volt_div) / sizeof(oscope_volt_div[0]))

static uint32_t oscope_volt_index = 2U;


/* ==========================================================
 * Trigger
 * ========================================================== */

/*
 * Trigger به صورت ADC count نگهداری می‌شود.
 *
 * 0V   -> 0
 * 3.3V -> 4095
 */
#define OSCOPE_TRIGGER_MIN             128U
#define OSCOPE_TRIGGER_MAX            3967U
#define OSCOPE_TRIGGER_STEP            128U

static uint16_t oscope_trigger_level = 2048U;


/* ==========================================================
 * Run / Stop
 * ========================================================== */

static bool oscope_running = true;


/* ==========================================================
 * LVGL
 * ========================================================== */

static lv_timer_t *oscope_timer = NULL;

static lv_obj_t *oscope_info_label = NULL;

static lv_chart_series_t *oscope_series = NULL;


/* ==========================================================
 * ADC buffers
 * ========================================================== */

static uint16_t oscope_samples[OSCOPE_POINT_COUNT];

static uint16_t oscope_last_samples[OSCOPE_POINT_COUNT];

static uint16_t oscope_last_count = 0U;


/* ==========================================================
 * Internal functions
 * ========================================================== */

static void OscopePage_ConfigureChart(void);

static void OscopePage_CreateInfoLabel(void);

static void OscopePage_UpdateInfoLabel(void);

static void OscopePage_UpdateRunStopButton(void);

static uint16_t OscopePage_GetVisibleSampleCount(void);

static uint16_t OscopePage_AdcToChart(
    uint16_t adc_value
);

static float OscopePage_AdcToVoltage(
    uint16_t adc_value
);

static int OscopePage_FindTrigger(
    const uint16_t *samples,
    uint16_t count
);

static void OscopePage_DrawSamples(
    const uint16_t *samples,
    uint16_t count
);

static void OscopePage_DrawLastSamples(void);

static void OscopePage_TimerCallback(
    lv_timer_t *timer
);


/* ==========================================================
 * ADC -> Voltage
 * ========================================================== */

static float OscopePage_AdcToVoltage(
    uint16_t adc_value
)
{
    return
        ((float)adc_value *
         OSCOPE_ADC_VOLTAGE) /
        (float)OSCOPE_ADC_MAX;
}


/* ==========================================================
 * Get visible sample count
 * ========================================================== */

static uint16_t OscopePage_GetVisibleSampleCount(void)
{
    uint32_t samples_per_div;
    uint32_t total_samples;

    samples_per_div =
        oscope_samples_per_div[oscope_time_index];

    total_samples =
        samples_per_div *
        OSCOPE_HORIZONTAL_DIVS;

    /*
     * بیشتر از تعداد نمونه موجود نمی‌خواهیم.
     */
    if (total_samples > OSCOPE_POINT_COUNT)
    {
        total_samples = OSCOPE_POINT_COUNT;
    }

    /*
     * حداقل مقدار منطقی.
     */
    if (total_samples < 20U)
    {
        total_samples = 20U;
    }

    return (uint16_t)total_samples;
}


/* ==========================================================
 * ADC -> Chart
 *
 * Volt/Div روی محدوده عمودی Chart اعمال می‌شود.
 * مرکز صفحه = 1.65V
 * ========================================================== */

static uint16_t OscopePage_AdcToChart(
    uint16_t adc_value
)
{
    float sample_voltage;
    float center_voltage;

    float total_range;
    float half_range;

    float min_voltage;
    float max_voltage;

    float chart_ratio;
    float chart_value;


    sample_voltage =
        OscopePage_AdcToVoltage(
            adc_value
        );

    center_voltage =
        OSCOPE_ADC_VOLTAGE / 2.0f;

    total_range =
        oscope_volt_div[oscope_volt_index] *
        (float)OSCOPE_VERTICAL_DIVS;

    half_range =
        total_range / 2.0f;

    min_voltage =
        center_voltage - half_range;

    max_voltage =
        center_voltage + half_range;


    /*
     * جلوگیری از تقسیم بر صفر.
     */
    if (total_range <= 0.001f)
    {
        total_range = OSCOPE_ADC_VOLTAGE;
        min_voltage = 0.0f;
        max_voltage = OSCOPE_ADC_VOLTAGE;
    }


    /*
     * خارج از محدوده پایین
     */
    if (sample_voltage <= min_voltage)
    {
        return 0U;
    }


    /*
     * خارج از محدوده بالا
     */
    if (sample_voltage >= max_voltage)
    {
        return OSCOPE_ADC_MAX;
    }


    chart_ratio =
        (sample_voltage - min_voltage) /
        total_range;


    chart_value =
        chart_ratio *
        (float)OSCOPE_ADC_MAX;


    if (chart_value < 0.0f)
    {
        chart_value = 0.0f;
    }

    if (chart_value > (float)OSCOPE_ADC_MAX)
    {
        chart_value =
            (float)OSCOPE_ADC_MAX;
    }


    return (uint16_t)chart_value;
}


/* ==========================================================
 * Find rising trigger
 * ========================================================== */

static int OscopePage_FindTrigger(
    const uint16_t *samples,
    uint16_t count
)
{
    uint16_t i;

    if (samples == NULL)
    {
        return -1;
    }

    if (count < 2U)
    {
        return -1;
    }


    /*
     * Rising edge:
     *
     * previous < trigger
     * current  >= trigger
     */
    for (i = 1U; i < count; i++)
    {
        if (samples[i - 1U] <
                oscope_trigger_level &&
            samples[i] >=
                oscope_trigger_level)
        {
            return (int)i;
        }
    }


    return -1;
}


/* ==========================================================
 * Configure chart
 * ========================================================== */

static void OscopePage_ConfigureChart(void)
{
    if (objects.chart_oscope == NULL)
    {
        return;
    }


    lv_chart_set_type(
        objects.chart_oscope,
        LV_CHART_TYPE_LINE
    );


    lv_chart_set_point_count(
        objects.chart_oscope,
        OSCOPE_POINT_COUNT
    );


    lv_chart_set_range(
        objects.chart_oscope,
        LV_CHART_AXIS_PRIMARY_Y,
        0,
        OSCOPE_ADC_MAX
    );


    /*
     * هر بار sample جدید وارد شود،
     * نمودار به صورت Shift حرکت می‌کند.
     */
    lv_chart_set_update_mode(
        objects.chart_oscope,
        LV_CHART_UPDATE_MODE_SHIFT
    );


    /*
     * نقاط کوچک روی waveform حذف شوند.
     */
    lv_obj_set_style_size(
        objects.chart_oscope,
        0,
        LV_PART_INDICATOR
    );


    /*
     * Series فقط یک بار ساخته شود.
     */
    if (oscope_series == NULL)
    {
        oscope_series =
            lv_chart_add_series(
                objects.chart_oscope,
                lv_palette_main(LV_PALETTE_RED),
                LV_CHART_AXIS_PRIMARY_Y
            );
    }
}


/* ==========================================================
 * Create information label
 * ========================================================== */

static void OscopePage_CreateInfoLabel(void)
{
    if (oscope_info_label != NULL)
    {
        return;
    }


    if (objects.osilloscop == NULL)
    {
        return;
    }


    oscope_info_label =
        lv_label_create(
            objects.osilloscop
        );


    if (oscope_info_label == NULL)
    {
        return;
    }


    /*
     * داخل محدوده بالای Chart
     */
    lv_obj_set_pos(
        oscope_info_label,
        16,
        12
    );


    lv_obj_set_size(
        oscope_info_label,
        285,
        LV_SIZE_CONTENT
    );


    lv_obj_set_style_text_color(
        oscope_info_label,
        lv_color_white(),
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    lv_obj_set_style_text_font(
        oscope_info_label,
        &lv_font_montserrat_12,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    lv_label_set_text(
        oscope_info_label,
        "ADC:---- MIN:---- MAX:----"
    );
}


/* ==========================================================
 * Update information label
 * ========================================================== */

static void OscopePage_UpdateInfoLabel(void)
{
    uint16_t latest;
    uint16_t min_value;
    uint16_t max_value;

    uint16_t i;

    float trigger_voltage;

    char text[128];

    const char *run_text;


    if (oscope_info_label == NULL)
    {
        return;
    }


    latest = 0U;

    min_value =
        OSCOPE_ADC_MAX;

    max_value = 0U;


    if (oscope_last_count > 0U)
    {
        latest =
            oscope_last_samples[
                oscope_last_count - 1U
            ];


        for (i = 0U;
             i < oscope_last_count;
             i++)
        {
            if (oscope_last_samples[i] <
                    min_value)
            {
                min_value =
                    oscope_last_samples[i];
            }


            if (oscope_last_samples[i] >
                    max_value)
            {
                max_value =
                    oscope_last_samples[i];
            }
        }
    }
    else
    {
        min_value = 0U;
        max_value = 0U;
    }


    trigger_voltage =
        OscopePage_AdcToVoltage(
            oscope_trigger_level
        );


    if (oscope_running)
    {
        run_text = "RUN";
    }
    else
    {
        run_text = "STOP";
    }


    snprintf(
        text,
        sizeof(text),

        "ADC:%4u MIN:%4u MAX:%4u\n"
        "T:%luus V:%.1fV TR:%.2fV %s",

        latest,
        min_value,
        max_value,

        (unsigned long)
            oscope_time_div_us[
                oscope_time_index
            ],

        (double)
            oscope_volt_div[
                oscope_volt_index
            ],

        (double)trigger_voltage,

        run_text
    );


    lv_label_set_text(
        oscope_info_label,
        text
    );
}


/* ==========================================================
 * Update STOP/RUN button
 * ========================================================== */

static void OscopePage_UpdateRunStopButton(void)
{
    lv_obj_t *label;


    if (objects.stop_run_btn == NULL)
    {
        return;
    }


    /*
     * در screen.c تنها child این button همان label است.
     */
    label =
        lv_obj_get_child(
            objects.stop_run_btn,
            0
        );


    if (label == NULL)
    {
        return;
    }


    if (oscope_running)
    {
        lv_label_set_text(
            label,
            "STOP"
        );
    }
    else
    {
        lv_label_set_text(
            label,
            "RUN"
        );
    }
}


/* ==========================================================
 * Draw samples
 * ========================================================== */

static void OscopePage_DrawSamples(
    const uint16_t *samples,
    uint16_t count
)
{
    uint16_t visible_count;

    uint16_t i;

    int trigger_index;

    int start_index;


    if (samples == NULL)
    {
        return;
    }


    if (count == 0U)
    {
        return;
    }


    if (objects.chart_oscope == NULL)
    {
        return;
    }


    if (oscope_series == NULL)
    {
        OscopePage_ConfigureChart();
    }


    if (oscope_series == NULL)
    {
        return;
    }


    visible_count =
        OscopePage_GetVisibleSampleCount();


    if (visible_count > count)
    {
        visible_count = count;
    }


    /*
     * پیدا کردن Trigger
     */
    trigger_index =
        OscopePage_FindTrigger(
            samples,
            count
        );


    if (trigger_index >= 0)
    {
        /*
         * Trigger را روی حدود 25% صفحه قرار می‌دهیم.
         */
        start_index =
            trigger_index -
            (int)(
                ((uint32_t)visible_count *
                 OSCOPE_TRIGGER_POSITION) /
                100U
            );


        /*
         * Wrap
         */
        while (start_index < 0)
        {
            start_index += count;
        }


        while (start_index >= (int)count)
        {
            start_index -= count;
        }
    }
    else
    {
        /*
         * Trigger پیدا نشد.
         * از ابتدای بلوک نمایش می‌دهیم.
         */
        start_index = 0;
    }


    /*
     * پاک کردن waveform قبلی
     */
    lv_chart_set_all_value(
        objects.chart_oscope,
        oscope_series,
        0
    );


    /*
     * وارد کردن waveform جدید
     *
     * بسیار مهم:
     * از set_next_value استفاده می‌کنیم.
     * همین روش در نسخه قبلی waveform را درست
     * نشان می‌داد.
     */
    for (i = 0U;
         i < visible_count;
         i++)
    {
        uint16_t source_index;
        uint16_t chart_value;


        source_index =
            (uint16_t)(
                (
                    start_index +
                    (int)i
                ) % (int)count
            );


        chart_value =
            OscopePage_AdcToChart(
                samples[source_index]
            );


        lv_chart_set_next_value(
            objects.chart_oscope,
            oscope_series,
            (lv_coord_t)chart_value
        );
    }


    lv_chart_refresh(
        objects.chart_oscope
    );


    lv_obj_invalidate(
        objects.chart_oscope
    );
}


/* ==========================================================
 * Draw last captured samples
 * ========================================================== */

static void OscopePage_DrawLastSamples(void)
{
    if (oscope_last_count == 0U)
    {
        OscopePage_UpdateInfoLabel();
        return;
    }


    OscopePage_DrawSamples(
        oscope_last_samples,
        oscope_last_count
    );


    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * Timer
 * ========================================================== */

static void OscopePage_TimerCallback(
    lv_timer_t *timer
)
{
    HAL_StatusTypeDef status;

    uint16_t block_size;

    uint16_t i;


    (void)timer;


    /*
     * در STOP دیگر ADC جدید نمی‌گیریم.
     * آخرین waveform روی صفحه باقی می‌ماند.
     */
    if (!oscope_running)
    {
        return;
    }


    block_size =
        OscopeADC_GetBlockSize();


    if (block_size == 0U)
    {
        return;
    }


    if (block_size > OSCOPE_POINT_COUNT)
    {
        block_size =
            OSCOPE_POINT_COUNT;
    }


    status =
        OscopeADC_ReadSamples(
            oscope_samples,
            block_size
        );


    if (status != HAL_OK)
    {
        return;
    }


    /*
     * ذخیره آخرین بلوک
     */
    for (i = 0U;
         i < block_size;
         i++)
    {
        oscope_last_samples[i] =
            oscope_samples[i];
    }


    oscope_last_count =
        block_size;


    /*
     * رسم waveform
     */
    OscopePage_DrawSamples(
        oscope_last_samples,
        oscope_last_count
    );


    /*
     * بروزرسانی اطلاعات
     */
    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * Enter
 * ========================================================== */

void OscopePage_OnEnter(void)
{
    /*
     * تنظیمات اولیه هر بار ورود به صفحه
     */
    oscope_time_index = 2U;   /* 100 us/div */

    oscope_volt_index = 2U;   /* 0.5 V/div */

    oscope_trigger_level = 2048U;

    oscope_running = true;

    oscope_last_count = 0U;


    /*
     * Chart
     */
    OscopePage_ConfigureChart();


    /*
     * پاک کردن نمودار
     */
    if (oscope_series != NULL)
    {
        lv_chart_set_all_value(
            objects.chart_oscope,
            oscope_series,
            0
        );


        lv_chart_refresh(
            objects.chart_oscope
        );
    }


    /*
     * Label
     */
    OscopePage_CreateInfoLabel();

    OscopePage_UpdateRunStopButton();

    OscopePage_UpdateInfoLabel();


    /*
     * Timer قبلی را حذف کنیم
     */
    if (oscope_timer != NULL)
    {
        lv_timer_del(
            oscope_timer
        );

        oscope_timer = NULL;
    }


    /*
     * هر 30ms یک بلوک جدید.
     */
    oscope_timer =
        lv_timer_create(
            OscopePage_TimerCallback,
            30,
            NULL
        );
}


/* ==========================================================
 * Exit
 * ========================================================== */

void OscopePage_OnExit(void)
{
    if (oscope_timer != NULL)
    {
        lv_timer_del(
            oscope_timer
        );

        oscope_timer = NULL;
    }


    if (oscope_info_label != NULL)
    {
        lv_obj_del(
            oscope_info_label
        );

        oscope_info_label = NULL;
    }


    oscope_running = false;
}


/* ==========================================================
 * TIME +
 * ========================================================== */

void OscopePage_TimeIncrease(void)
{
    if (oscope_time_index <
        (OSCOPE_TIME_DIV_COUNT - 1U))
    {
        oscope_time_index++;
    }


    /*
     * همان waveform آخر را با scale جدید
     * دوباره رسم کن.
     */
    OscopePage_DrawLastSamples();
}


/* ==========================================================
 * TIME -
 * ========================================================== */

void OscopePage_TimeDecrease(void)
{
    if (oscope_time_index > 0U)
    {
        oscope_time_index--;
    }


    OscopePage_DrawLastSamples();
}


/* ==========================================================
 * VOLT +
 * ========================================================== */

void OscopePage_VoltIncrease(void)
{
    if (oscope_volt_index <
        (OSCOPE_VOLT_DIV_COUNT - 1U))
    {
        oscope_volt_index++;
    }


    OscopePage_DrawLastSamples();
}


/* ==========================================================
 * VOLT -
 * ========================================================== */

void OscopePage_VoltDecrease(void)
{
    if (oscope_volt_index > 0U)
    {
        oscope_volt_index--;
    }


    OscopePage_DrawLastSamples();
}


/* ==========================================================
 * TRIGGER +
 * ========================================================== */

void OscopePage_TriggerIncrease(void)
{
    uint32_t new_level;


    new_level =
        (uint32_t)oscope_trigger_level +
        OSCOPE_TRIGGER_STEP;


    if (new_level > OSCOPE_TRIGGER_MAX)
    {
        new_level =
            OSCOPE_TRIGGER_MAX;
    }


    oscope_trigger_level =
        (uint16_t)new_level;


    /*
     * waveform فعلی را با Trigger جدید
     * دوباره مرتب کن.
     */
    OscopePage_DrawLastSamples();
}


/* ==========================================================
 * TRIGGER -
 * ========================================================== */

void OscopePage_TriggerDecrease(void)
{
    int32_t new_level;


    new_level =
        (int32_t)oscope_trigger_level -
        (int32_t)OSCOPE_TRIGGER_STEP;


    if (new_level < OSCOPE_TRIGGER_MIN)
    {
        new_level =
            OSCOPE_TRIGGER_MIN;
    }


    oscope_trigger_level =
        (uint16_t)new_level;


    OscopePage_DrawLastSamples();
}


/* ==========================================================
 * RUN / STOP
 * ========================================================== */

void OscopePage_ToggleRunStop(void)
{
    oscope_running =
        !oscope_running;


    OscopePage_UpdateRunStopButton();


    OscopePage_UpdateInfoLabel();
}


/* ==========================================================
 * Compatibility functions
 *
 * اگر جایی از پروژه هنوز این دو تابع قدیمی را صدا بزند،
 * باعث خطای Link نمی‌شوند.
 * ========================================================== */

void OscopePage_IncreaseSpeed(void)
{
    OscopePage_TimeIncrease();
}


void OscopePage_DecreaseSpeed(void)
{
    OscopePage_TimeDecrease();
}
