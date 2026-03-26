// This file implements the Fall Detection Alert screen (Screen 4)
// It shows a countdown that, when reaches zero, triggers an SOS alert.
// Tapping anywhere cancels the alert.

#include "../ui.h"
#include <stdio.h>

lv_obj_t *ui_Screen4 = NULL;
lv_obj_t *ui_LabelCountdown = NULL;
lv_obj_t *ui_LabelFallQ = NULL;
lv_obj_t *ui_LabelTapCancel = NULL;
lv_timer_t *scr4_timer = NULL;
int scr4_counter = 10;

extern void trigger_sos_alarm(void);

static void scr4_countdown_cb(lv_timer_t *timer)
{
    scr4_counter--;
    if (scr4_counter <= 0)
    {
        lv_timer_del(timer);
        scr4_timer = NULL;
        trigger_sos_alarm();
        // Return to main screen after alarm triggered (or wait for user intervention)
        _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, &ui_Screen1_screen_init);
    }
    else
    {
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", scr4_counter);
        lv_label_set_text(ui_LabelCountdown, buf);
    }
}

void scr4_tap_cb(lv_event_t *e)
{
    if (scr4_timer)
    {
        lv_timer_del(scr4_timer);
        scr4_timer = NULL;
    }
    _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, &ui_Screen1_screen_init);
}

void ui_Screen4_screen_init(void)
{
    ui_Screen4 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen4, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen4, lv_color_hex(0x800000), 0); // Dark Red alert background

    ui_LabelFallQ = lv_label_create(ui_Screen4);
    lv_obj_set_align(ui_LabelFallQ, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_LabelFallQ, 100);
    lv_label_set_text(ui_LabelFallQ, "DETEKTOVAN PAD!\nDa li ste dobro?");
    lv_obj_set_style_text_font(ui_LabelFallQ, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_align(ui_LabelFallQ, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui_LabelFallQ, lv_color_hex(0xFFFFFF), 0);

    ui_LabelCountdown = lv_label_create(ui_Screen4);
    lv_obj_set_align(ui_LabelCountdown, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelCountdown, "10");
    lv_obj_set_style_text_font(ui_LabelCountdown, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(ui_LabelCountdown, lv_color_hex(0xFFFFFF), 0);

    ui_LabelTapCancel = lv_label_create(ui_Screen4);
    lv_obj_set_align(ui_LabelTapCancel, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(ui_LabelTapCancel, -100);
    lv_label_set_text(ui_LabelTapCancel, "DODIRNI ZA OTKAZIVANJE");
    lv_obj_set_style_text_font(ui_LabelTapCancel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ui_LabelTapCancel, lv_color_hex(0xAAAAAA), 0);

    scr4_counter = 10;
    scr4_timer = lv_timer_create(scr4_countdown_cb, 1000, NULL);

    lv_obj_add_event_cb(ui_Screen4, scr4_tap_cb, LV_EVENT_CLICKED, NULL);
}

void ui_Screen4_screen_destroy(void)
{
    if (scr4_timer)
    {
        lv_timer_del(scr4_timer);
        scr4_timer = NULL;
    }
    if (ui_Screen4)
        lv_obj_del(ui_Screen4);
    ui_Screen4 = NULL;
    ui_LabelCountdown = NULL;
    ui_LabelFallQ = NULL;
    ui_LabelTapCancel = NULL;
}
