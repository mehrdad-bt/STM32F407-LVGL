#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

static void event_handler_checked_cb_main_msg_hide_switch(lv_event_t *e) {
    if (lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED)) {
        action_error_hide(e);
    }
}

//
// Screens
//

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // calibration_button
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.calibration_button = obj;
            lv_obj_set_pos(obj, 8, 180);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, action_calibration, LV_EVENT_RELEASED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x00ffc9), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // calibration_text
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.calibration_text = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Calibration");
                }
            }
        }
        {
            // temp_button
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.temp_button = obj;
            lv_obj_set_pos(obj, 9, 116);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, action_go_to_temp_page, LV_EVENT_RELEASED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x00ffc9), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // temp_button_text
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.temp_button_text = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Button");
                }
            }
        }
        {
            // messageBox
            lv_obj_t *obj = lv_msgbox_create(parent_obj, "", "", 0, true);
            objects.message_box = obj;
            lv_obj_set_pos(obj, 41, 53);
            lv_obj_set_size(obj, 238, 144);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // message_box_text
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.message_box_text = obj;
                    lv_obj_set_pos(obj, 180, 18);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text_static(obj, "Text");
                }
            }
        }
        {
            // information_indicator
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.information_indicator = obj;
            lv_obj_set_pos(obj, 231, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text_static(obj, "Text");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xC3C3C3), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // msg_hide_switch
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.msg_hide_switch = obj;
            lv_obj_set_pos(obj, 260, 201);
            lv_obj_set_size(obj, 50, 25);
            lv_obj_add_event_cb(obj, event_handler_checked_cb_main_msg_hide_switch, LV_EVENT_VALUE_CHANGED, (void *)0);
            lv_obj_add_event_cb(obj, event_handler_cb_main_msg_hide_switch, LV_EVENT_ALL, 0);
        }
        {
            // Error_hide_text
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.error_hide_text = obj;
            lv_obj_set_pos(obj, 192, 210);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text_static(obj, "Text");
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
}

void create_screen_temp() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.temp = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // temp_exit_button
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.temp_exit_button = obj;
            lv_obj_set_pos(obj, 10, 179);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, action_back_to_main, LV_EVENT_RELEASED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // temp_exit_button_text
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.temp_exit_button_text = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Button");
                }
            }
        }
        {
            // Temp_text_static
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.temp_text_static = obj;
            lv_obj_set_pos(obj, 233, 61);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xF47B7B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Text");
        }
        {
            // Humid_text_static
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.humid_text_static = obj;
            lv_obj_set_pos(obj, 233, 114);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x74D7F9), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Text");
        }
        {
            // Temp_text_dynamic
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.temp_text_dynamic = obj;
            lv_obj_set_pos(obj, 110, 61);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Text");
        }
        {
            // Humid_text_dynamic
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.humid_text_dynamic = obj;
            lv_obj_set_pos(obj, 110, 114);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Text");
        }
        {
            // Temp_Humid_Header_Text
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.temp_humid_header_text = obj;
            lv_obj_set_pos(obj, 139, 3);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xC3C3C3), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Text");
        }
    }
    
    tick_screen_temp();
}

void tick_screen_temp() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_temp,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 2) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
    create_screen_temp();
}
