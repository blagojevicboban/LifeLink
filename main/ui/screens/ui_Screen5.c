#include "ui_Screen5.h"
#include <stdio.h>
#include <string.h>
#include "../ui.h"

lv_obj_t *ui_Screen5 = NULL;
static lv_obj_t *ui_TextAreaNumpad = NULL;
static lv_obj_t *ui_LabelTitleNumpad = NULL;
static edit_mode_t current_edit_mode = EDIT_MODE_SMS;

// References to lifelink.cpp global arrays
extern char g_w_sms_numbers[128];
extern char g_w_call_numbers[128];
extern char g_w_sos_number[20];
extern void save_settings();

static void numpad_btn_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    const char *txt = lv_label_get_text(label);

    if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        lv_textarea_del_char(ui_TextAreaNumpad);
    } else if (strcmp(txt, "Done") == 0) {
        const char *entered_text = lv_textarea_get_text(ui_TextAreaNumpad);
        if (current_edit_mode == EDIT_MODE_SMS) {
            strncpy(g_w_sms_numbers, entered_text, sizeof(g_w_sms_numbers) - 1);
            g_w_sms_numbers[sizeof(g_w_sms_numbers)-1] = '\0';
        } else if (current_edit_mode == EDIT_MODE_CALL) {
            strncpy(g_w_call_numbers, entered_text, sizeof(g_w_call_numbers) - 1);
            g_w_call_numbers[sizeof(g_w_call_numbers)-1] = '\0';
        } else if (current_edit_mode == EDIT_MODE_SOS) {
            strncpy(g_w_sos_number, entered_text, sizeof(g_w_sos_number) - 1);
            g_w_sos_number[sizeof(g_w_sos_number)-1] = '\0';
        }
        save_settings();
        _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, &ui_Screen3_screen_init);
    } else {
        lv_textarea_add_text(ui_TextAreaNumpad, txt);
    }
}

static lv_obj_t *create_btn(lv_obj_t *parent, const char *text, int x, int y, int w, int h) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A2A3B), LV_PART_MAIN);
    
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    
    lv_obj_add_event_cb(btn, numpad_btn_cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

void ui_Screen5_screen_init(void) {
    ui_Screen5 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen5, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen5, lv_color_hex(0x00050A), LV_PART_MAIN);

    ui_LabelTitleNumpad = lv_label_create(ui_Screen5);
    lv_obj_set_align(ui_LabelTitleNumpad, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_LabelTitleNumpad, 30);
    lv_obj_set_style_text_font(ui_LabelTitleNumpad, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_LabelTitleNumpad, lv_color_hex(0xFFFFFF), 0);

    if (current_edit_mode == EDIT_MODE_SMS) lv_label_set_text(ui_LabelTitleNumpad, "Podesi SMS Broj");
    else if (current_edit_mode == EDIT_MODE_CALL) lv_label_set_text(ui_LabelTitleNumpad, "Podesi Poziv Broj");
    else lv_label_set_text(ui_LabelTitleNumpad, "Podesi SOS Broj");

    ui_TextAreaNumpad = lv_textarea_create(ui_Screen5);
    lv_obj_set_size(ui_TextAreaNumpad, 300, 45);
    lv_obj_set_align(ui_TextAreaNumpad, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_TextAreaNumpad, 60);
    lv_textarea_set_one_line(ui_TextAreaNumpad, true);
    lv_obj_set_style_text_font(ui_TextAreaNumpad, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_TextAreaNumpad, lv_color_hex(0xFFFFFF), 0);

    if (current_edit_mode == EDIT_MODE_SMS) lv_textarea_set_text(ui_TextAreaNumpad, g_w_sms_numbers);
    else if (current_edit_mode == EDIT_MODE_CALL) lv_textarea_set_text(ui_TextAreaNumpad, g_w_call_numbers);
    else lv_textarea_set_text(ui_TextAreaNumpad, g_w_sos_number);

    // Numpad Grid
    int bw = 85, bh = 50, gap = 10;
    int sx = 80, sy = 120;

    char *keys[] = {"1","2","3","4","5","6","7","8","9","+","0",LV_SYMBOL_BACKSPACE};
    for(int i=0; i<12; i++) {
        create_btn(ui_Screen5, keys[i], sx + (i%3)*(bw+gap), sy + (i/3)*(bh+gap), bw, bh);
    }
    
    lv_obj_t *done = create_btn(ui_Screen5, "Done", 183, sy + 4*(bh+gap), 100, 50);
    lv_obj_set_style_bg_color(done, lv_color_hex(0x008800), LV_PART_MAIN);
}

void ui_Screen5_screen_destroy(void) {
    if (ui_Screen5) lv_obj_del(ui_Screen5);
    ui_Screen5 = NULL;
}

void ui_open_numpad(edit_mode_t mode) {
    current_edit_mode = mode;
    if (ui_Screen5 == NULL) ui_Screen5_screen_init();
    lv_scr_load_anim(ui_Screen5, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}
