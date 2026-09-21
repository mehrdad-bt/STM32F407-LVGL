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
/* Private Variables                                                         */
/* -------------------------------------------------------------------------- */

static lv_obj_t *waiting_container = NULL;

static lv_obj_t *waiting_dots[3] =
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
/* Create Waiting Animation                                                  */
/* -------------------------------------------------------------------------- */

void Animation_Init(void)
{
    if (objects.main == NULL)
    {
        return;
    }


    /* ---------------------------------------------------------------------- */
    /* Container                                                               */
    /* ---------------------------------------------------------------------- */

    waiting_container =
        lv_obj_create(objects.main);

    if (waiting_container == NULL)
    {
        return;
    }


    lv_obj_remove_style_all(
        waiting_container
    );


    /* Container size */
    lv_obj_set_size(
        waiting_container,
        (3U * WAITING_DOT_SIZE) +
        (2U * WAITING_DOT_SPACING),
        WAITING_DOT_SIZE
    );


    /* Top-right corner */
    lv_obj_align(
        waiting_container,
        LV_ALIGN_TOP_RIGHT,
        -8,
        8
    );


    /* ---------------------------------------------------------------------- */
    /* Create Dots                                                             */
    /* ---------------------------------------------------------------------- */

    for (int i = 0; i < 3; i++)
    {
        waiting_dots[i] =
            lv_obj_create(waiting_container);

        if (waiting_dots[i] == NULL)
        {
            continue;
        }


        /* Remove default style */
        lv_obj_remove_style_all(
            waiting_dots[i]
        );


        /* Size */
        lv_obj_set_size(
            waiting_dots[i],
            WAITING_DOT_SIZE,
            WAITING_DOT_SIZE
        );


        /* Orange */
        lv_obj_set_style_bg_color(
            waiting_dots[i],
            lv_color_hex(0xFFA500),
            LV_PART_MAIN
        );

        lv_obj_set_style_bg_opa(
            waiting_dots[i],
            LV_OPA_COVER,
            LV_PART_MAIN
        );


        /* Circle */
        lv_obj_set_style_radius(
            waiting_dots[i],
            LV_RADIUS_CIRCLE,
            LV_PART_MAIN
        );


        /* Position */
        lv_obj_set_pos(
            waiting_dots[i],
            i * (WAITING_DOT_SIZE +
                 WAITING_DOT_SPACING),
            0
        );


        /* ------------------------------------------------------------------ */
        /* Animation                                                           */
        /* ------------------------------------------------------------------ */

        lv_anim_t anim;

        lv_anim_init(
            &anim
        );


        /* Object */
        lv_anim_set_var(
            &anim,
            waiting_dots[i]
        );


        /* Opacity range */
        lv_anim_set_values(
            &anim,
            40,
            255
        );


        /* Speed */
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


        /* Infinite */
        lv_anim_set_repeat_count(
            &anim,
            LV_ANIM_REPEAT_INFINITE
        );


        /* Delay */
        lv_anim_set_delay(
            &anim,
            i * WAITING_DOT_DELAY
        );


        /* Smooth */
        lv_anim_set_path_cb(
            &anim,
            lv_anim_path_ease_in_out
        );


        /* Start */
        lv_anim_start(
            &anim
        );
    }
}
