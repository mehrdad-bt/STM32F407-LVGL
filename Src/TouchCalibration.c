#include "touchCalibration.h"

#include "TouchController.h"
#include "lvgl.h"

#include <stdint.h>
#include <stdbool.h>

/* =========================================================
 * Calibration configuration
 * ========================================================= */

/*
 * Exactly like TFT_eSPI calibration:
 *
 * 1 -> Top Left
 * 2 -> Top Right
 * 3 -> Bottom Right
 * 4 -> Bottom Left
 *
 * No center point.
 */

#define CAL_POINT_COUNT             4U

/*
 * Distance of calibration points from the screen edges.
 * Equivalent to the "15" used in the original TFT_eSPI code.
 */
#define CAL_POINT_OFFSET_X          15U
#define CAL_POINT_OFFSET_Y          15U

/*
 * Number of raw samples used to calculate one point.
 */
#define CAL_SAMPLE_COUNT            10U

/*
 * Polling interval.
 */
#define CAL_TIMER_PERIOD_MS         20U

/*
 * Display time after calibration.
 */
#define CAL_COMPLETE_DELAY_MS       1000U

/*
 * Minimum valid raw range.
 */
#define CAL_MIN_RAW_RANGE           300U

/* =========================================================
 * Calibration point
 * ========================================================= */

typedef struct
{
    uint16_t x;
    uint16_t y;
} CalibrationPoint_t;

/* =========================================================
 * Raw calibration point
 * ========================================================= */

typedef struct
{
    uint16_t x;
    uint16_t y;
} CalibrationRawPoint_t;

/* =========================================================
 * Calibration points
 * ========================================================= */

static const CalibrationPoint_t calibration_points[
    CAL_POINT_COUNT
] =
{
    /* 1 - Top Left */
    {
        CAL_POINT_OFFSET_X,
        CAL_POINT_OFFSET_Y
    },

    /* 2 - Top Right */
    {
        TOUCH_SCREEN_WIDTH - 1U - CAL_POINT_OFFSET_X,
        CAL_POINT_OFFSET_Y
    },

    /* 3 - Bottom Right */
    {
        TOUCH_SCREEN_WIDTH - 1U - CAL_POINT_OFFSET_X,
        TOUCH_SCREEN_HEIGHT - 1U - CAL_POINT_OFFSET_Y
    },

    /* 4 - Bottom Left */
    {
        CAL_POINT_OFFSET_X,
        TOUCH_SCREEN_HEIGHT - 1U - CAL_POINT_OFFSET_Y
    }
};

/* =========================================================
 * Calibration state
 * ========================================================= */

typedef enum
{
    CAL_STATE_WAIT_INITIAL_RELEASE = 0,
    CAL_STATE_WAIT_TOUCH,
    CAL_STATE_CAPTURE,
    CAL_STATE_WAIT_RELEASE,
    CAL_STATE_COMPLETE
} CalibrationState_t;

/* =========================================================
 * Static state
 * ========================================================= */

static bool calibration_running = false;

static CalibrationState_t calibration_state =
    CAL_STATE_WAIT_INITIAL_RELEASE;

static uint8_t current_point = 0U;

static uint32_t raw_sum_x = 0U;
static uint32_t raw_sum_y = 0U;

static uint8_t raw_sample_count = 0U;

static CalibrationRawPoint_t raw_points[
    CAL_POINT_COUNT
];

/* =========================================================
 * LVGL objects
 * ========================================================= */

static lv_obj_t *calibration_screen = NULL;

static lv_obj_t *calibration_title = NULL;

static lv_obj_t *calibration_status = NULL;

static lv_obj_t *calibration_target = NULL;

/*
 * Screen active before calibration started.
 */
static lv_obj_t *return_screen = NULL;

/* =========================================================
 * Timers
 * ========================================================= */

static lv_timer_t *calibration_timer = NULL;

static lv_timer_t *exit_timer = NULL;

/* =========================================================
 * Forward declarations
 * ========================================================= */

static void CalibrationTimerCallback(
    lv_timer_t *timer
);

static void CalibrationExitCallback(
    lv_timer_t *timer
);

static void CreateCalibrationScreen(void);

static void UpdateCalibrationTarget(void);

static void StartPointCapture(void);

static void FinishPointCapture(void);

static bool ApplyCalibration(void);

/* =========================================================
 * Average two values
 * ========================================================= */

static uint16_t Average2(
    uint16_t a,
    uint16_t b
)
{
    return (uint16_t)(
        ((uint32_t)a + (uint32_t)b) / 2U
    );
}

/* =========================================================
 * Clamp 12-bit XPT2046 value
 * ========================================================= */

static uint16_t ClampRaw(
    int32_t value
)
{
    if (value < 0)
    {
        return 0U;
    }

    if (value > 4095)
    {
        return 4095U;
    }

    return (uint16_t)value;
}

/* =========================================================
 * Extrapolate raw coordinate to screen edge
 *
 * Calibration points are 15 pixels from the edges.
 * We therefore calculate the raw value corresponding
 * to screen coordinate 0 and screen maximum.
 * ========================================================= */

static uint16_t ExtrapolateRaw(
    uint16_t raw1,
    uint16_t screen1,
    uint16_t raw2,
    uint16_t screen2,
    uint16_t target_screen
)
{
    int32_t r1 = (int32_t)raw1;
    int32_t r2 = (int32_t)raw2;

    int32_t s1 = (int32_t)screen1;
    int32_t s2 = (int32_t)screen2;

    int32_t st = (int32_t)target_screen;

    int32_t screen_delta =
        s2 - s1;

    if (screen_delta == 0)
    {
        return raw1;
    }

    int32_t raw_delta =
        r2 - r1;

    int32_t target_delta =
        st - s1;

    int32_t result =
        r1 +
        (
            (raw_delta * target_delta) /
            screen_delta
        );

    return ClampRaw(result);
}

/* =========================================================
 * Update calibration target
 * ========================================================= */

static void UpdateCalibrationTarget(void)
{
    if (calibration_target == NULL)
    {
        return;
    }

    if (current_point >= CAL_POINT_COUNT)
    {
        return;
    }

    uint16_t x =
        calibration_points[current_point].x;

    uint16_t y =
        calibration_points[current_point].y;

    /*
     * Target object is 26x26.
     * Put its center exactly on the calibration point.
     */
    lv_obj_set_pos(
        calibration_target,
        (lv_coord_t)(x - 13U),
        (lv_coord_t)(y - 13U)
    );

    if (calibration_status != NULL)
    {
        lv_label_set_text_fmt(
            calibration_status,
            "Touch point %u / %u",
            (unsigned)(current_point + 1U),
            (unsigned)CAL_POINT_COUNT
        );
    }
}

/* =========================================================
 * Create calibration screen
 * ========================================================= */

static void CreateCalibrationScreen(void)
{
    calibration_screen =
        lv_obj_create(NULL);

    /*
     * No scrolling.
     */
    lv_obj_clear_flag(
        calibration_screen,
        LV_OBJ_FLAG_SCROLLABLE
    );

    /*
     * Black background.
     */
    lv_obj_set_style_bg_color(
        calibration_screen,
        lv_color_hex(0x000000),
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        calibration_screen,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    /*
     * Remove border and padding.
     */
    lv_obj_set_style_border_width(
        calibration_screen,
        0,
        LV_PART_MAIN
    );

    lv_obj_set_style_pad_all(
        calibration_screen,
        0,
        LV_PART_MAIN
    );

    /* =====================================================
     * Title
     * ===================================================== */

    calibration_title =
        lv_label_create(
            calibration_screen
        );

    lv_label_set_text(
        calibration_title,
        "TOUCH CALIBRATION"
    );

    lv_obj_set_style_text_color(
        calibration_title,
        lv_color_hex(0xFFFFFF),
        LV_PART_MAIN
    );

    lv_obj_align(
        calibration_title,
        LV_ALIGN_TOP_MID,
        0,
        2
    );

    /* =====================================================
     * Status
     * ===================================================== */

    calibration_status =
        lv_label_create(
            calibration_screen
        );

    lv_label_set_text(
        calibration_status,
        "Release the screen..."
    );

    lv_obj_set_style_text_color(
        calibration_status,
        lv_color_hex(0xFFFFFF),
        LV_PART_MAIN
    );

    lv_obj_set_style_text_align(
        calibration_status,
        LV_TEXT_ALIGN_CENTER,
        LV_PART_MAIN
    );

    lv_obj_align(
        calibration_status,
        LV_ALIGN_BOTTOM_MID,
        0,
        -2
    );

    /* =====================================================
     * Calibration target
     * ===================================================== */

    calibration_target =
        lv_obj_create(
            calibration_screen
        );

    lv_obj_set_size(
        calibration_target,
        26,
        26
    );

    /*
     * Transparent inside.
     */
    lv_obj_set_style_bg_opa(
        calibration_target,
        LV_OPA_TRANSP,
        LV_PART_MAIN
    );

    /*
     * Magenta border.
     */
    lv_obj_set_style_border_width(
        calibration_target,
        3,
        LV_PART_MAIN
    );

    lv_obj_set_style_border_color(
        calibration_target,
        lv_color_hex(0xFF00FF),
        LV_PART_MAIN
    );

    lv_obj_set_style_border_opa(
        calibration_target,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    lv_obj_set_style_radius(
        calibration_target,
        LV_RADIUS_CIRCLE,
        LV_PART_MAIN
    );

    lv_obj_set_style_pad_all(
        calibration_target,
        0,
        LV_PART_MAIN
    );

    /*
     * Initially hidden.
     *
     * We first wait until the finger is released from
     * the calibration button.
     */
    lv_obj_add_flag(
        calibration_target,
        LV_OBJ_FLAG_HIDDEN
    );
}

/* =========================================================
 * Start capturing current point
 * ========================================================= */

static void StartPointCapture(void)
{
    raw_sum_x = 0U;
    raw_sum_y = 0U;

    raw_sample_count = 0U;

    calibration_state =
        CAL_STATE_CAPTURE;

    if (calibration_status != NULL)
    {
        lv_label_set_text(
            calibration_status,
            "Keep touching..."
        );
    }
}

/* =========================================================
 * Apply calibration
 *
 * Four manually selected corner points are enough to
 * determine:
 *
 *     X minimum
 *     X maximum
 *     Y minimum
 *     Y maximum
 *     Axis swap
 *     X inversion
 *     Y inversion
 *
 * ========================================================= */

static bool ApplyCalibration(void)
{
    /*
     * Raw points:
     *
     * 0 = Top Left
     * 1 = Top Right
     * 2 = Bottom Right
     * 3 = Bottom Left
     */

    /*
     * Determine whether raw X follows horizontal or
     * vertical movement.
     */

    uint32_t raw_x_horizontal =
        (uint32_t)(
            (raw_points[1].x >= raw_points[0].x)
                ?
                (raw_points[1].x - raw_points[0].x)
                :
                (raw_points[0].x - raw_points[1].x)
        )
        +
        (uint32_t)(
            (raw_points[2].x >= raw_points[3].x)
                ?
                (raw_points[2].x - raw_points[3].x)
                :
                (raw_points[3].x - raw_points[2].x)
        );

    uint32_t raw_x_vertical =
        (uint32_t)(
            (raw_points[3].x >= raw_points[0].x)
                ?
                (raw_points[3].x - raw_points[0].x)
                :
                (raw_points[0].x - raw_points[3].x)
        )
        +
        (uint32_t)(
            (raw_points[2].x >= raw_points[1].x)
                ?
                (raw_points[2].x - raw_points[1].x)
                :
                (raw_points[1].x - raw_points[2].x)
        );

    bool swap_xy =
        (raw_x_vertical > raw_x_horizontal);

    uint16_t raw_left;
    uint16_t raw_right;
    uint16_t raw_top;
    uint16_t raw_bottom;

    /* =====================================================
     * Normal orientation
     *
     * raw X -> screen X
     * raw Y -> screen Y
     * ===================================================== */

    if (!swap_xy)
    {
        raw_left =
            Average2(
                raw_points[0].x,
                raw_points[3].x
            );

        raw_right =
            Average2(
                raw_points[1].x,
                raw_points[2].x
            );

        raw_top =
            Average2(
                raw_points[0].y,
                raw_points[1].y
            );

        raw_bottom =
            Average2(
                raw_points[3].y,
                raw_points[2].y
            );
    }

    /* =====================================================
     * Swapped orientation
     *
     * raw Y -> screen X
     * raw X -> screen Y
     * ===================================================== */

    else
    {
        raw_left =
            Average2(
                raw_points[0].y,
                raw_points[3].y
            );

        raw_right =
            Average2(
                raw_points[1].y,
                raw_points[2].y
            );

        raw_top =
            Average2(
                raw_points[0].x,
                raw_points[1].x
            );

        raw_bottom =
            Average2(
                raw_points[3].x,
                raw_points[2].x
            );
    }

    /* =====================================================
     * Validate X range
     * ===================================================== */

    uint16_t x_range;

    if (raw_right >= raw_left)
    {
        x_range =
            (uint16_t)(
                raw_right - raw_left
            );
    }
    else
    {
        x_range =
            (uint16_t)(
                raw_left - raw_right
            );
    }

    /* =====================================================
     * Validate Y range
     * ===================================================== */

    uint16_t y_range;

    if (raw_bottom >= raw_top)
    {
        y_range =
            (uint16_t)(
                raw_bottom - raw_top
            );
    }
    else
    {
        y_range =
            (uint16_t)(
                raw_top - raw_bottom
            );
    }

    /*
     * Invalid calibration.
     */
    if (
        (x_range < CAL_MIN_RAW_RANGE) ||
        (y_range < CAL_MIN_RAW_RANGE)
    )
    {
        if (calibration_status != NULL)
        {
            lv_label_set_text(
                calibration_status,
                "Calibration failed!"
            );
        }

        return false;
    }

    /* =====================================================
     * Determine inversion
     * ===================================================== */

    bool invert_x =
        (raw_left > raw_right);

    bool invert_y =
        (raw_top > raw_bottom);

    /* =====================================================
     * Extrapolate raw values to actual screen edges
     *
     * We touched:
     *
     * X = 15 .. 304
     * Y = 15 .. 224
     *
     * The actual screen is:
     *
     * X = 0 .. 319
     * Y = 0 .. 239
     * ===================================================== */

    uint16_t x_edge_0 =
        ExtrapolateRaw(
            raw_left,
            calibration_points[0].x,
            raw_right,
            calibration_points[1].x,
            0U
        );

    uint16_t x_edge_max =
        ExtrapolateRaw(
            raw_left,
            calibration_points[0].x,
            raw_right,
            calibration_points[1].x,
            TOUCH_SCREEN_WIDTH - 1U
        );

    uint16_t y_edge_0 =
        ExtrapolateRaw(
            raw_top,
            calibration_points[0].y,
            raw_bottom,
            calibration_points[3].y,
            0U
        );

    uint16_t y_edge_max =
        ExtrapolateRaw(
            raw_top,
            calibration_points[0].y,
            raw_bottom,
            calibration_points[3].y,
            TOUCH_SCREEN_HEIGHT - 1U
        );

    uint16_t x_min;
    uint16_t x_max;

    uint16_t y_min;
    uint16_t y_max;

    if (x_edge_0 < x_edge_max)
    {
        x_min = x_edge_0;
        x_max = x_edge_max;
    }
    else
    {
        x_min = x_edge_max;
        x_max = x_edge_0;
    }

    if (y_edge_0 < y_edge_max)
    {
        y_min = y_edge_0;
        y_max = y_edge_max;
    }
    else
    {
        y_min = y_edge_max;
        y_max = y_edge_0;
    }

    /*
     * Final validation.
     */
    if (
        ((uint16_t)(x_max - x_min) < CAL_MIN_RAW_RANGE) ||
        ((uint16_t)(y_max - y_min) < CAL_MIN_RAW_RANGE)
    )
    {
        if (calibration_status != NULL)
        {
            lv_label_set_text(
                calibration_status,
                "Calibration failed!"
            );
        }

        return false;
    }

    /* =====================================================
     * Apply calibration to XPT2046 driver
     * ===================================================== */

    XPT2046_SetCalibration(
        x_min,
        x_max,
        y_min,
        y_max,
        swap_xy,
        invert_x,
        invert_y
    );

    /*
     * Show calculated values.
     */
    if (calibration_status != NULL)
    {
        lv_label_set_text_fmt(
            calibration_status,
            "Calibration complete!\n"
            "X: %u - %u\n"
            "Y: %u - %u",
            (unsigned)x_min,
            (unsigned)x_max,
            (unsigned)y_min,
            (unsigned)y_max
        );
    }

    return true;
}

/* =========================================================
 * Finish current point
 * ========================================================= */

static void FinishPointCapture(void)
{
    if (current_point >= CAL_POINT_COUNT)
    {
        return;
    }

    if (raw_sample_count == 0U)
    {
        return;
    }

    /*
     * Save average raw coordinate.
     */
    raw_points[current_point].x =
        (uint16_t)(
            raw_sum_x /
            (uint32_t)raw_sample_count
        );

    raw_points[current_point].y =
        (uint16_t)(
            raw_sum_y /
            (uint32_t)raw_sample_count
        );

    current_point++;

    /*
     * =====================================================
     * More points remain
     * ===================================================== */

    if (current_point < CAL_POINT_COUNT)
    {
        calibration_state =
            CAL_STATE_WAIT_TOUCH;

        UpdateCalibrationTarget();

        return;
    }

    /*
     * =====================================================
     * Four points complete
     * ===================================================== */

    calibration_state =
        CAL_STATE_COMPLETE;

    /*
     * Hide calibration target.
     */
    if (calibration_target != NULL)
    {
        lv_obj_add_flag(
            calibration_target,
            LV_OBJ_FLAG_HIDDEN
        );
    }

    /*
     * Calculate and apply calibration.
     */
    if (ApplyCalibration())
    {
        /*
         * Show final message.
         */
        if (calibration_status != NULL)
        {
            /*
             * ApplyCalibration already set detailed values.
             */
        }
    }

    /*
     * Start exit timer.
     */
    if (exit_timer != NULL)
    {
        lv_timer_del(exit_timer);
        exit_timer = NULL;
    }

    exit_timer =
        lv_timer_create(
            CalibrationExitCallback,
            CAL_COMPLETE_DELAY_MS,
            NULL
        );
}

/* =========================================================
 * Calibration timer
 * ========================================================= */

static void CalibrationTimerCallback(
    lv_timer_t *timer
)
{
    (void)timer;

    if (!calibration_running)
    {
        return;
    }

    if (
        calibration_state ==
        CAL_STATE_COMPLETE
    )
    {
        return;
    }

    uint16_t raw_x = 0U;
    uint16_t raw_y = 0U;
    uint16_t pressure = 0U;

    bool touched =
        XPT2046_GetRawTouch(
            &raw_x,
            &raw_y,
            &pressure
        );

    /* =====================================================
     * Wait until touch of calibration button is released
     * ===================================================== */

    if (
        calibration_state ==
        CAL_STATE_WAIT_INITIAL_RELEASE
    )
    {
        if (!touched)
        {
            /*
             * Now show first calibration target.
             */
            if (calibration_target != NULL)
            {
                lv_obj_clear_flag(
                    calibration_target,
                    LV_OBJ_FLAG_HIDDEN
                );
            }

            current_point = 0U;

            calibration_state =
                CAL_STATE_WAIT_TOUCH;

            UpdateCalibrationTarget();
        }

        return;
    }

    /* =====================================================
     * Wait for user to touch current target
     * ===================================================== */

    if (
        calibration_state ==
        CAL_STATE_WAIT_TOUCH
    )
    {
        if (touched)
        {
            StartPointCapture();

            /*
             * First sample.
             */
            raw_sum_x += raw_x;
            raw_sum_y += raw_y;

            raw_sample_count++;
        }

        return;
    }

    /* =====================================================
     * Capture samples while finger remains on point
     * ===================================================== */

    if (
        calibration_state ==
        CAL_STATE_CAPTURE
    )
    {
        if (touched)
        {
            raw_sum_x += raw_x;
            raw_sum_y += raw_y;

            raw_sample_count++;

            if (
                raw_sample_count >=
                CAL_SAMPLE_COUNT
            )
            {
                calibration_state =
                    CAL_STATE_WAIT_RELEASE;

                if (calibration_status != NULL)
                {
                    lv_label_set_text(
                        calibration_status,
                        "Release"
                    );
                }
            }
        }
        else
        {
            /*
             * Finger was released too early.
             * Restart the same calibration point.
             */
            raw_sum_x = 0U;
            raw_sum_y = 0U;
            raw_sample_count = 0U;

            calibration_state =
                CAL_STATE_WAIT_TOUCH;

            if (calibration_status != NULL)
            {
                lv_label_set_text(
                    calibration_status,
                    "Touch point again"
                );
            }
        }

        return;
    }

    /* =====================================================
     * Wait until finger is released
     * ===================================================== */

    if (
        calibration_state ==
        CAL_STATE_WAIT_RELEASE
    )
    {
        if (!touched)
        {
            FinishPointCapture();
        }

        return;
    }
}

/* =========================================================
 * Exit calibration
 * ========================================================= */

static void CalibrationExitCallback(
    lv_timer_t *timer
)
{
    if (timer != NULL)
    {
        lv_timer_del(timer);
    }

    exit_timer = NULL;

    /*
     * Stop calibration timer.
     */
    if (calibration_timer != NULL)
    {
        lv_timer_del(calibration_timer);
        calibration_timer = NULL;
    }

    /*
     * Re-enable normal LVGL touch handling.
     */
    XPT2046_SetCalibrationMode(false);

    /*
     * Return to previous screen.
     */
    if (return_screen != NULL)
    {
        lv_scr_load(
            return_screen
        );
    }

    /*
     * Delete temporary calibration screen.
     */
    if (calibration_screen != NULL)
    {
        lv_obj_del(
            calibration_screen
        );
    }

    /*
     * Clear objects.
     */
    calibration_screen = NULL;
    calibration_title = NULL;
    calibration_status = NULL;
    calibration_target = NULL;

    return_screen = NULL;

    /*
     * Reset state.
     */
    calibration_running = false;

    calibration_state =
        CAL_STATE_WAIT_INITIAL_RELEASE;

    current_point = 0U;

    raw_sum_x = 0U;
    raw_sum_y = 0U;

    raw_sample_count = 0U;
}

/* =========================================================
 * Public function
 * ========================================================= */

void touch_calibration_start(void)
{
    /*
     * Prevent starting twice.
     */
    if (calibration_running)
    {
        return;
    }

    /*
     * Remember current screen.
     */
    return_screen = lv_scr_act();

    /*
     * Start calibration.
     */
    calibration_running = true;

    calibration_state =
        CAL_STATE_WAIT_INITIAL_RELEASE;

    current_point = 0U;

    raw_sum_x = 0U;
    raw_sum_y = 0U;

    raw_sample_count = 0U;

    /*
     * Clear previous raw calibration data.
     */
    for (
        uint8_t i = 0U;
        i < CAL_POINT_COUNT;
        i++
    )
    {
        raw_points[i].x = 0U;
        raw_points[i].y = 0U;
    }

    /*
     * Normal LVGL touch callback must not consume
     * raw touch while calibration is running.
     */
    XPT2046_SetCalibrationMode(true);

    /*
     * Create calibration screen.
     */
    CreateCalibrationScreen();

    /*
     * Load calibration screen.
     */
    lv_scr_load(
        calibration_screen
    );

    /*
     * Create polling timer.
     */
    if (calibration_timer != NULL)
    {
        lv_timer_del(
            calibration_timer
        );

        calibration_timer = NULL;
    }

    calibration_timer =
        lv_timer_create(
            CalibrationTimerCallback,
            CAL_TIMER_PERIOD_MS,
            NULL
        );
}
