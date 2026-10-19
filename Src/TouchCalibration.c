#include "touchCalibration.h"

#include "TouchController.h"
#include "lvgl.h"

#include <stdint.h>
#include <stdbool.h>

/* =========================================================
 * Calibration configuration
 * ========================================================= */

/*
 * Same method as TFT_eSPI calibrateTouch():
 *
 * The user touches the four real CORNERS of the screen
 * (the "15" in the TFT_eSPI sketch is only the size of the
 * arrow that is drawn, the touch point is the corner itself):
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
 * Arrow drawn at the corner (same as TFT_eSPI calibrateTouch,
 * "size" = 15 pixels).
 */
#define CAL_ARROW_SIZE              15
#define CAL_ARROW_LINE_WIDTH        1
#define CAL_ARROW_COLOR             0xFF00FFU   /* TFT_MAGENTA */

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
        0U,
        0U
    },

    /* 2 - Top Right */
    {
        TOUCH_SCREEN_WIDTH - 1U,
        0U
    },

    /* 3 - Bottom Right */
    {
        TOUCH_SCREEN_WIDTH - 1U,
        TOUCH_SCREEN_HEIGHT - 1U
    },

    /* 4 - Bottom Left */
    {
        0U,
        TOUCH_SCREEN_HEIGHT - 1U
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

/*
 * calibration_target is a transparent container (full screen)
 * that holds the three lines of the corner arrow.
 */
static lv_obj_t *calibration_target = NULL;

static lv_obj_t *calibration_arrow[3] = { NULL, NULL, NULL };

static lv_point_t arrow_points[3][2];

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
 * Absolute value
 * ========================================================= */

static int32_t Abs32(
    int32_t value
)
{
    return (value < 0) ? -value : value;
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
     * Arrow pointing at the corner, like TFT_eSPI:
     *
     *   - one line along the vertical edge
     *   - one line along the horizontal edge
     *   - one diagonal line
     *
     * all starting at the corner and CAL_ARROW_SIZE long.
     */
    int32_t sx = (x == 0U) ? 1 : -1;
    int32_t sy = (y == 0U) ? 1 : -1;

    int32_t cx = (int32_t)x;
    int32_t cy = (int32_t)y;

    arrow_points[0][0].x = (lv_coord_t)cx;
    arrow_points[0][0].y = (lv_coord_t)cy;
    arrow_points[0][1].x = (lv_coord_t)cx;
    arrow_points[0][1].y = (lv_coord_t)(cy + sy * CAL_ARROW_SIZE);

    arrow_points[1][0].x = (lv_coord_t)cx;
    arrow_points[1][0].y = (lv_coord_t)cy;
    arrow_points[1][1].x = (lv_coord_t)(cx + sx * CAL_ARROW_SIZE);
    arrow_points[1][1].y = (lv_coord_t)cy;

    arrow_points[2][0].x = (lv_coord_t)cx;
    arrow_points[2][0].y = (lv_coord_t)cy;
    arrow_points[2][1].x = (lv_coord_t)(cx + sx * CAL_ARROW_SIZE);
    arrow_points[2][1].y = (lv_coord_t)(cy + sy * CAL_ARROW_SIZE);

    for (uint8_t k = 0U; k < 3U; k++)
    {
        if (calibration_arrow[k] != NULL)
        {
            lv_line_set_points(
                calibration_arrow[k],
                arrow_points[k],
                2U
            );
        }
    }

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
     * Calibration target: corner arrow (3 lines)
     * ===================================================== */

    calibration_target =
        lv_obj_create(
            calibration_screen
        );

    lv_obj_set_size(
        calibration_target,
        TOUCH_SCREEN_WIDTH,
        TOUCH_SCREEN_HEIGHT
    );

    lv_obj_set_pos(
        calibration_target,
        0,
        0
    );

    lv_obj_set_style_bg_opa(
        calibration_target,
        LV_OPA_TRANSP,
        LV_PART_MAIN
    );

    lv_obj_set_style_border_width(
        calibration_target,
        0,
        LV_PART_MAIN
    );

    lv_obj_set_style_pad_all(
        calibration_target,
        0,
        LV_PART_MAIN
    );

    lv_obj_clear_flag(
        calibration_target,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_clear_flag(
        calibration_target,
        LV_OBJ_FLAG_CLICKABLE
    );

    for (uint8_t k = 0U; k < 3U; k++)
    {
        calibration_arrow[k] =
            lv_line_create(
                calibration_target
            );

        lv_obj_set_style_line_color(
            calibration_arrow[k],
            lv_color_hex(CAL_ARROW_COLOR),
            LV_PART_MAIN
        );

        lv_obj_set_style_line_width(
            calibration_arrow[k],
            CAL_ARROW_LINE_WIDTH,
            LV_PART_MAIN
        );
    }

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
 * Apply calibration  (TFT_eSPI calibrateTouch() algorithm)
 *
 * raw_points:
 *     0 = Top Left
 *     1 = Top Right
 *     2 = Bottom Right
 *     3 = Bottom Left
 *
 * 1. From Top Left to Bottom Left the screen Y changes.
 *    If the raw X changes more than the raw Y, the touch and
 *    screen axes are swapped.
 * 2. Raw min/max of the axis that feeds the screen X and the
 *    axis that feeds the screen Y are the averages of the two
 *    corners on each side.
 * 3. If min > max, the axis is inverted: min and max are
 *    exchanged and the invert flag is set.
 * ========================================================= */

static bool ApplyCalibration(void)
{
    int32_t ul_x = (int32_t)raw_points[0].x;
    int32_t ul_y = (int32_t)raw_points[0].y;

    int32_t ur_x = (int32_t)raw_points[1].x;
    int32_t ur_y = (int32_t)raw_points[1].y;

    int32_t br_x = (int32_t)raw_points[2].x;
    int32_t br_y = (int32_t)raw_points[2].y;

    int32_t bl_x = (int32_t)raw_points[3].x;
    int32_t bl_y = (int32_t)raw_points[3].y;

    int32_t x0;
    int32_t x1;
    int32_t y0;
    int32_t y1;

    bool swap_xy;
    bool invert_x;
    bool invert_y;

    swap_xy =
        (
            Abs32(ul_x - bl_x) >
            Abs32(ul_y - bl_y)
        );

    if (swap_xy)
    {
        x0 = (ul_y + bl_y) / 2;   /* min X */
        x1 = (ur_y + br_y) / 2;   /* max X */
        y0 = (ul_x + ur_x) / 2;   /* min Y */
        y1 = (bl_x + br_x) / 2;   /* max Y */
    }
    else
    {
        x0 = (ul_x + bl_x) / 2;   /* min X */
        x1 = (ur_x + br_x) / 2;   /* max X */
        y0 = (ul_y + ur_y) / 2;   /* min Y */
        y1 = (bl_y + br_y) / 2;   /* max Y */
    }

    invert_x = false;
    invert_y = false;

    if (x0 > x1)
    {
        int32_t tmp = x0;

        x0 = x1;
        x1 = tmp;

        invert_x = true;
    }

    if (y0 > y1)
    {
        int32_t tmp = y0;

        y0 = y1;
        y1 = tmp;

        invert_y = true;
    }

    /*
     * Invalid calibration (corners not touched correctly).
     */
    if (
        ((x1 - x0) < (int32_t)CAL_MIN_RAW_RANGE) ||
        ((y1 - y0) < (int32_t)CAL_MIN_RAW_RANGE)
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

    if (x0 < 0)    { x0 = 0;    }
    if (y0 < 0)    { y0 = 0;    }
    if (x1 > 4095) { x1 = 4095; }
    if (y1 > 4095) { y1 = 4095; }

    /*
     * Apply to the XPT2046 driver.
     */
    /*
     * The XPT2046 driver uses TFT_eSPI semantics:
     * the 2nd and 4th parameter are the RANGE (max - min),
     * not the maximum value.
     */
    XPT2046_SetCalibration(
        (uint16_t)x0,
        (uint16_t)(x1 - x0),
        (uint16_t)y0,
        (uint16_t)(y1 - y0),
        swap_xy,
        invert_x,
        invert_y
    );

    if (calibration_status != NULL)
    {
        lv_label_set_text_fmt(
            calibration_status,
            "Calibration complete!\n"
            "X: %u - %u\n"
            "Y: %u - %u",
            (unsigned)x0,
            (unsigned)x1,
            (unsigned)y0,
            (unsigned)y1
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

    calibration_arrow[0] = NULL;
    calibration_arrow[1] = NULL;
    calibration_arrow[2] = NULL;

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
