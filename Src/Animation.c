#include "Animation.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/screens.h"

/* -------------------------------------------------------------------------- */
/* Defines                                                                    */
/* -------------------------------------------------------------------------- */

#define WAITING_DOT_SIZE            8U
#define WAITING_DOT_SPACING         6U
#define WAITING_DOT_ANIM_TIME       700U
#define WAITING_DOT_DELAY           250U

/* -------------------------------------------------------------------------- */
/* Private Variables                                                          */
/* -------------------------------------------------------------------------- */

static lv_obj_t *waiting_container_main = NULL;
static lv_obj_t *waiting_dots_main[3] =
{
    NULL,
    NULL,
    NULL
};

static lv_obj_t *waiting_container_temp = NULL;
static lv_obj_t *waiting_dots_temp[3] =
{
    NULL,
    NULL,
    NULL
};

/* -------------------------------------------------------------------------- */
/* Animation Callback                                                         */
/* -------------------------------------------------------------------------- */

static void Waiting_Dot_Anim_Callback(
    void *var,
    int32_t value)
{
    lv_obj_t *dot = (lv_obj_t *)var;

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

static void Animation_CreateWaitingDots(
    lv_obj_t *parent,
    lv_obj_t **container,
    lv_obj_t *dots[3])
{
    if (parent == NULL)
    {
        return;
    }

    /* ---------------------------------------------------------------------- */
    /* Container                                                               */
    /* ---------------------------------------------------------------------- */

    *container = lv_obj_create(parent);

    if (*container == NULL)
    {
        return;
    }

    lv_obj_remove_style_all(*container);

    /* Container size */
    lv_obj_set_size(
        *container,
        (3U * WAITING_DOT_SIZE) +
        (2U * WAITING_DOT_SPACING),
        WAITING_DOT_SIZE
    );

    /* ---------------------------------------------------------------------- */
    /* Create Dots                                                             */
    /* ---------------------------------------------------------------------- */

    for (int i = 0; i < 3; i++)
    {
        dots[i] = lv_obj_create(*container);

        if (dots[i] == NULL)
        {
            continue;
        }

        /* Remove default style */
        lv_obj_remove_style_all(dots[i]);

        /* Size */
        lv_obj_set_size(
            dots[i],
            WAITING_DOT_SIZE,
            WAITING_DOT_SIZE
        );

        /* Orange */
        lv_obj_set_style_bg_color(
            dots[i],
            lv_color_hex(0xFFA500),
            LV_PART_MAIN
        );

        lv_obj_set_style_bg_opa(
            dots[i],
            LV_OPA_COVER,
            LV_PART_MAIN
        );

        /* Circle */
        lv_obj_set_style_radius(
            dots[i],
            LV_RADIUS_CIRCLE,
            LV_PART_MAIN
        );

        /* Position of each dot inside container */
        lv_obj_set_pos(
            dots[i],
            i * (WAITING_DOT_SIZE + WAITING_DOT_SPACING),
            0
        );

        /* ------------------------------------------------------------------ */
        /* Animation                                                           */
        /* ------------------------------------------------------------------ */

        lv_anim_t anim;

        lv_anim_init(&anim);

        /* Object */
        lv_anim_set_var(
            &anim,
            dots[i]
        );

        /* Opacity range */
        lv_anim_set_values(
            &anim,
            40,
            255
        );

        /* Animation time */
        lv_anim_set_time(
            &anim,
            WAITING_DOT_ANIM_TIME
        );

        lv_anim_set_playback_time(
            &anim,
            WAITING_DOT_ANIM_TIME
        );

        /* Callback */
        lv_anim_set_exec_cb(
            &anim,
            Waiting_Dot_Anim_Callback
        );

        /* Infinite repeat */
        lv_anim_set_repeat_count(
            &anim,
            LV_ANIM_REPEAT_INFINITE
        );

        /* Delay for each dot */
        lv_anim_set_delay(
            &anim,
            i * WAITING_DOT_DELAY
        );

        /* Smooth animation */
        lv_anim_set_path_cb(
            &anim,
            lv_anim_path_ease_in_out
        );

        /* Start */
        lv_anim_start(&anim);
    }
}

/* -------------------------------------------------------------------------- */
/* Init                                                                       */
/* -------------------------------------------------------------------------- */

void Animation_Init(void)
{
    /* ---------------------------------------------------------------------- */
    /* MAIN PAGE                                                              */
    /* ---------------------------------------------------------------------- */

//    if (objects.main != NULL)
//    {
//        Animation_CreateWaitingDots(
//            objects.main,
//            &waiting_container_main,
//            waiting_dots_main
//        );
//
//        /*
//         * MAIN position
//         *
//         * Container width:
//         *
//         * 3 * 8 + 2 * 6 = 36 pixels
//         *
//         * Screen width = 320
//         *
//         * Right side with 8 pixel margin:
//         * 320 - 36 - 8 = 276
//         */
//
//        lv_obj_set_pos(
//            waiting_container_main,
//            276,
//            8
//        );
//    }

    /* ---------------------------------------------------------------------- */
    /* TEMP PAGE                                                              */
    /* ---------------------------------------------------------------------- */

    if (objects.temp != NULL)
    {
        Animation_CreateWaitingDots(
            objects.temp,
            &waiting_container_temp,
            waiting_dots_temp
        );

        /*
         * TEMP position
         *
         * X = 142
         * Y = 10
         */

        lv_obj_set_pos(
            waiting_container_temp,
            90,
            12
        );
    }
}
