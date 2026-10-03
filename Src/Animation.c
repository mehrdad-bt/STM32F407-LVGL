#include "Animation.h"

#include "lvgl.h"


/* ========================================================================== */
/* WAITING DOTS                                                               */
/* ========================================================================== */


/* -------------------------------------------------------------------------- */
/* Defines                                                                    */
/* -------------------------------------------------------------------------- */

#define WAITING_DOT_SIZE            8U
#define WAITING_DOT_SPACING         6U
#define WAITING_DOT_ANIM_TIME       700U
#define WAITING_DOT_DELAY           250U


/* -------------------------------------------------------------------------- */
/* Callback                                                                   */
/* -------------------------------------------------------------------------- */

static void Waiting_Dot_Anim_Callback(
    void *var,
    int32_t value
)
{
    lv_obj_t *dot =
        (lv_obj_t *)var;


    if (dot == NULL)
    {
        return;
    }


    lv_obj_set_style_opa(
        dot,
        (lv_opa_t)value,
        LV_PART_MAIN
    );
}


/* -------------------------------------------------------------------------- */
/* Create Waiting Dots                                                        */
/* -------------------------------------------------------------------------- */

void Animation_CreateWaitingDots(
    lv_obj_t *parent,
    int32_t x,
    int32_t y
)
{
    lv_obj_t *container;


    if (parent == NULL)
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Container                                                               */
    /* ---------------------------------------------------------------------- */

    container =
        lv_obj_create(parent);


    if (container == NULL)
    {
        return;
    }


    lv_obj_remove_style_all(
        container
    );


    lv_obj_set_size(
        container,
        (3U * WAITING_DOT_SIZE) +
        (2U * WAITING_DOT_SPACING),
        WAITING_DOT_SIZE
    );


    lv_obj_set_pos(
        container,
        x,
        y
    );


    /* ---------------------------------------------------------------------- */
    /* Create Dots                                                             */
    /* ---------------------------------------------------------------------- */

    for (
        uint8_t i = 0U;
        i < 3U;
        i++
    )
    {
        lv_obj_t *dot =
            lv_obj_create(
                container
            );


        if (dot == NULL)
        {
            continue;
        }


        lv_obj_remove_style_all(
            dot
        );


        lv_obj_set_size(
            dot,
            WAITING_DOT_SIZE,
            WAITING_DOT_SIZE
        );


        lv_obj_set_style_bg_color(
            dot,
            lv_color_hex(0xFFA500),
            LV_PART_MAIN
        );


        lv_obj_set_style_bg_opa(
            dot,
            LV_OPA_COVER,
            LV_PART_MAIN
        );


        lv_obj_set_style_radius(
            dot,
            LV_RADIUS_CIRCLE,
            LV_PART_MAIN
        );


        lv_obj_set_pos(
            dot,
            i * (
                WAITING_DOT_SIZE +
                WAITING_DOT_SPACING
            ),
            0
        );


        /* ------------------------------------------------------------------ */
        /* Animation                                                           */
        /* ------------------------------------------------------------------ */

        lv_anim_t anim;


        lv_anim_init(
            &anim
        );


        lv_anim_set_var(
            &anim,
            dot
        );


        lv_anim_set_values(
            &anim,
            40,
            255
        );


        lv_anim_set_time(
            &anim,
            WAITING_DOT_ANIM_TIME
        );


        lv_anim_set_playback_time(
            &anim,
            WAITING_DOT_ANIM_TIME
        );


        lv_anim_set_exec_cb(
            &anim,
            Waiting_Dot_Anim_Callback
        );


        lv_anim_set_repeat_count(
            &anim,
            LV_ANIM_REPEAT_INFINITE
        );


        lv_anim_set_delay(
            &anim,
            i * WAITING_DOT_DELAY
        );


        lv_anim_set_path_cb(
            &anim,
            lv_anim_path_ease_in_out
        );


        lv_anim_start(
            &anim
        );
    }
}


/* ========================================================================== */
/* DATA TRANSFER ANIMATION                                                    */
/* ========================================================================== */


/* -------------------------------------------------------------------------- */
/* Defines                                                                    */
/* -------------------------------------------------------------------------- */

#define ARROW_COLOR             0x0080FF
#define ARROW_WIDTH             22
#define ARROW_HEIGHT            24

#define TX_X                    4
#define TX_Y                    3

#define RX_X                    20
#define RX_Y                    3

#define MOVE_DISTANCE           7
#define ANIM_TIME               500
#define ARROW_LINE_WIDTH        4


/* -------------------------------------------------------------------------- */
/* Arrow Objects                                                              */
/* -------------------------------------------------------------------------- */

static lv_obj_t *tx_arrow = NULL;
static lv_obj_t *rx_arrow = NULL;


/* TX */

static lv_obj_t *tx_line_vertical = NULL;
static lv_obj_t *tx_line_left     = NULL;
static lv_obj_t *tx_line_right    = NULL;


/* RX */

static lv_obj_t *rx_line_vertical = NULL;
static lv_obj_t *rx_line_left     = NULL;
static lv_obj_t *rx_line_right    = NULL;


/* -------------------------------------------------------------------------- */
/* Create Line                                                                */
/* -------------------------------------------------------------------------- */

static lv_obj_t *create_line(
    lv_obj_t *parent,
    lv_point_t *points,
    uint16_t point_count
)
{
    lv_obj_t *line =
        lv_line_create(parent);


    if (line == NULL)
    {
        return NULL;
    }


    lv_line_set_points(
        line,
        points,
        point_count
    );


    lv_obj_set_style_line_color(
        line,
        lv_color_hex(ARROW_COLOR),
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    lv_obj_set_style_line_width(
        line,
        ARROW_LINE_WIDTH,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    lv_obj_set_style_line_rounded(
        line,
        true,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    return line;
}


/* -------------------------------------------------------------------------- */
/* Start Data Transfer Animation                                              */
/* -------------------------------------------------------------------------- */

void data_transfer_animation_start(
    lv_obj_t *parent
)
{
    /* ---------------------------------------------------------------------- */
    /* Already created                                                         */
    /* ---------------------------------------------------------------------- */

    if (
        tx_arrow != NULL ||
        rx_arrow != NULL
    )
    {
        return;
    }


    if (parent == NULL)
    {
        return;
    }


    /* ====================================================================== */
    /* TX ARROW                                                               */
    /* ↑                                                                        */
    /* ====================================================================== */

    tx_arrow =
        lv_obj_create(parent);


    if (tx_arrow == NULL)
    {
        return;
    }


    lv_obj_set_size(
        tx_arrow,
        ARROW_WIDTH,
        ARROW_HEIGHT
    );


    lv_obj_set_pos(
        tx_arrow,
        TX_X,
        TX_Y
    );


    /* بدون Background */

    lv_obj_set_style_bg_opa(
        tx_arrow,
        LV_OPA_TRANSP,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    /* بدون Border */

    lv_obj_set_style_border_width(
        tx_arrow,
        0,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    /* بدون Padding */

    lv_obj_set_style_pad_all(
        tx_arrow,
        0,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    /* بدون Shadow */

    lv_obj_set_style_shadow_width(
        tx_arrow,
        0,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    /* ---------------------------------------------------------------------- */
    /* TX Vertical                                                            */
    /* ---------------------------------------------------------------------- */

    static lv_point_t tx_vertical_points[] =
    {
        {11, 20},
        {11, 5}
    };


    tx_line_vertical =
        create_line(
            tx_arrow,
            tx_vertical_points,
            2
        );


    /* ---------------------------------------------------------------------- */
    /* TX Left Head                                                           */
    /* ---------------------------------------------------------------------- */

    static lv_point_t tx_left_points[] =
    {
        {11, 4},
        {5, 10}
    };


    tx_line_left =
        create_line(
            tx_arrow,
            tx_left_points,
            2
        );


    /* ---------------------------------------------------------------------- */
    /* TX Right Head                                                          */
    /* ---------------------------------------------------------------------- */

    static lv_point_t tx_right_points[] =
    {
        {11, 4},
        {17, 10}
    };


    tx_line_right =
        create_line(
            tx_arrow,
            tx_right_points,
            2
        );


    /* ====================================================================== */
    /* RX ARROW                                                               */
    /* ↓                                                                        */
    /* ====================================================================== */

    rx_arrow =
        lv_obj_create(parent);


    if (rx_arrow == NULL)
    {
        data_transfer_animation_stop();

        return;
    }


    lv_obj_set_size(
        rx_arrow,
        ARROW_WIDTH,
        ARROW_HEIGHT
    );


    lv_obj_set_pos(
        rx_arrow,
        RX_X,
        RX_Y
    );


    /* بدون Background */

    lv_obj_set_style_bg_opa(
        rx_arrow,
        LV_OPA_TRANSP,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    /* بدون Border */

    lv_obj_set_style_border_width(
        rx_arrow,
        0,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    /* بدون Padding */

    lv_obj_set_style_pad_all(
        rx_arrow,
        0,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    /* بدون Shadow */

    lv_obj_set_style_shadow_width(
        rx_arrow,
        0,
        LV_PART_MAIN | LV_STATE_DEFAULT
    );


    /* ---------------------------------------------------------------------- */
    /* RX Vertical                                                            */
    /* ---------------------------------------------------------------------- */

    static lv_point_t rx_vertical_points[] =
    {
        {11, 4},
        {11, 19}
    };


    rx_line_vertical =
        create_line(
            rx_arrow,
            rx_vertical_points,
            2
        );


    /* ---------------------------------------------------------------------- */
    /* RX Left Head                                                           */
    /* ---------------------------------------------------------------------- */

    static lv_point_t rx_left_points[] =
    {
        {11, 20},
        {5, 14}
    };


    rx_line_left =
        create_line(
            rx_arrow,
            rx_left_points,
            2
        );


    /* ---------------------------------------------------------------------- */
    /* RX Right Head                                                          */
    /* ---------------------------------------------------------------------- */

    static lv_point_t rx_right_points[] =
    {
        {11, 20},
        {17, 14}
    };


    rx_line_right =
        create_line(
            rx_arrow,
            rx_right_points,
            2
        );


    /* ====================================================================== */
    /* TX Animation                                                           */
    /*                                                                           */
    /* فلش بالا حرکت می‌کند                                                   */
    /* ====================================================================== */

    lv_anim_t tx_anim;


    lv_anim_init(
        &tx_anim
    );


    lv_anim_set_var(
        &tx_anim,
        tx_arrow
    );


    lv_anim_set_exec_cb(
        &tx_anim,
        (lv_anim_exec_xcb_t)lv_obj_set_y
    );


    lv_anim_set_values(
        &tx_anim,
        TX_Y + MOVE_DISTANCE,
        TX_Y
    );


    lv_anim_set_time(
        &tx_anim,
        ANIM_TIME
    );


    lv_anim_set_playback_time(
        &tx_anim,
        ANIM_TIME
    );


    lv_anim_set_repeat_count(
        &tx_anim,
        LV_ANIM_REPEAT_INFINITE
    );


    lv_anim_set_path_cb(
        &tx_anim,
        lv_anim_path_ease_in_out
    );


    lv_anim_start(
        &tx_anim
    );


    /* ====================================================================== */
    /* RX Animation                                                           */
    /*                                                                           */
    /* فلش پایین حرکت می‌کند                                                  */
    /* ====================================================================== */

    lv_anim_t rx_anim;


    lv_anim_init(
        &rx_anim
    );


    lv_anim_set_var(
        &rx_anim,
        rx_arrow
    );


    lv_anim_set_exec_cb(
        &rx_anim,
        (lv_anim_exec_xcb_t)lv_obj_set_y
    );


    lv_anim_set_values(
        &rx_anim,
        RX_Y,
        RX_Y + MOVE_DISTANCE
    );


    lv_anim_set_time(
        &rx_anim,
        ANIM_TIME
    );


    lv_anim_set_playback_time(
        &rx_anim,
        ANIM_TIME
    );


    lv_anim_set_repeat_count(
        &rx_anim,
        LV_ANIM_REPEAT_INFINITE
    );


    lv_anim_set_path_cb(
        &rx_anim,
        lv_anim_path_ease_in_out
    );


    /* کمی تأخیر برای حرکت مخالف */

    lv_anim_set_delay(
        &rx_anim,
        250
    );


    lv_anim_start(
        &rx_anim
    );
}


/* -------------------------------------------------------------------------- */
/* Stop Data Transfer Animation                                               */
/* -------------------------------------------------------------------------- */

void data_transfer_animation_stop(void)
{
    /* ---------------------------------------------------------------------- */
    /* TX                                                                       */
    /* ---------------------------------------------------------------------- */

    if (tx_arrow != NULL)
    {
        lv_anim_del(
            tx_arrow,
            NULL
        );


        lv_obj_del(
            tx_arrow
        );


        tx_arrow = NULL;


        tx_line_vertical = NULL;
        tx_line_left     = NULL;
        tx_line_right    = NULL;
    }


    /* ---------------------------------------------------------------------- */
    /* RX                                                                       */
    /* ---------------------------------------------------------------------- */

    if (rx_arrow != NULL)
    {
        lv_anim_del(
            rx_arrow,
            NULL
        );


        lv_obj_del(
            rx_arrow
        );


        rx_arrow = NULL;


        rx_line_vertical = NULL;
        rx_line_left     = NULL;
        rx_line_right    = NULL;
    }
}
