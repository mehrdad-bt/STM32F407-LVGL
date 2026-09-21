#ifndef ANIMATION_H
#define ANIMATION_H

#include "lvgl.h"


/* -------------------------------------------------------------------------- */
/* Waiting Dots                                                               */
/* -------------------------------------------------------------------------- */

void Animation_CreateWaitingDots(
    lv_obj_t *parent,
    int32_t x,
    int32_t y
);


/* -------------------------------------------------------------------------- */
/* Data Transfer Animation                                                   */
/* -------------------------------------------------------------------------- */

void data_transfer_animation_start(
    lv_obj_t *parent
);


void data_transfer_animation_stop(
    void
);

#endif
