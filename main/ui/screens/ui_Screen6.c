#include "../ui.h"
#include <stdio.h>
#include <string.h>

lv_obj_t *ui_Screen6 = NULL;
static lv_obj_t *ui_TextAreaSSID = NULL;
static lv_obj_t *ui_TextAreaPass = NULL;
static lv_obj_t *ui_KeyboardWiFi = NULL;

extern char g_wifi_ssid[32];
extern char g_wifi_pass[32];
extern void save_settings(void);
extern void wifi_reconnect_now(void);

static void ui_event_SaveWiFi(lv_event_t *e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        strncpy(g_wifi_ssid, lv_textarea_get_text(ui_TextAreaSSID), sizeof(g_wifi_ssid) - 1);
        strncpy(g_wifi_pass, lv_textarea_get_text(ui_TextAreaPass), sizeof(g_wifi_pass) - 1);
        save_settings();
        wifi_reconnect_now();
        _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_Screen3_screen_init);
    }
}

static void ui_event_CancelWiFi(lv_event_t *e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_Screen3_screen_init);
    }
}

static void ta_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(ui_KeyboardWiFi, ta);
        lv_obj_clear_flag(ui_KeyboardWiFi, LV_OBJ_FLAG_HIDDEN);
    }
    if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(ui_KeyboardWiFi, NULL);
        lv_obj_add_flag(ui_KeyboardWiFi, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_Screen6_screen_init(void) {
    ui_Screen6 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen6, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen6, lv_color_hex(0x000a14), 0);

    lv_obj_t *title = lv_label_create(ui_Screen6);
    lv_label_set_text(title, "WiFi Podesavanja");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title, 45);

    // SSID label & text area
    lv_obj_t *lbl_ssid = lv_label_create(ui_Screen6);
    lv_label_set_text(lbl_ssid, "SSID:");
    lv_obj_set_style_text_color(lbl_ssid, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(lbl_ssid, 40, 95);

    ui_TextAreaSSID = lv_textarea_create(ui_Screen6);
    lv_obj_set_size(ui_TextAreaSSID, 250, 45);
    lv_obj_set_pos(ui_TextAreaSSID, 40, 120);
    lv_textarea_set_text(ui_TextAreaSSID, g_wifi_ssid);
    lv_textarea_set_one_line(ui_TextAreaSSID, true);
    lv_obj_add_event_cb(ui_TextAreaSSID, ta_event_cb, LV_EVENT_ALL, NULL);

    // Password label & text area
    lv_obj_t *lbl_pass = lv_label_create(ui_Screen6);
    lv_label_set_text(lbl_pass, "Lozinka:");
    lv_obj_set_style_text_color(lbl_pass, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(lbl_pass, 40, 175);

    ui_TextAreaPass = lv_textarea_create(ui_Screen6);
    lv_obj_set_size(ui_TextAreaPass, 250, 45);
    lv_obj_set_pos(ui_TextAreaPass, 40, 200);
    lv_textarea_set_text(ui_TextAreaPass, g_wifi_pass);
    lv_textarea_set_one_line(ui_TextAreaPass, true);
    lv_textarea_set_password_mode(ui_TextAreaPass, true);
    lv_obj_add_event_cb(ui_TextAreaPass, ta_event_cb, LV_EVENT_ALL, NULL);

    // Keyboard
    ui_KeyboardWiFi = lv_keyboard_create(ui_Screen6);
    lv_obj_set_size(ui_KeyboardWiFi, 466, 200);
    lv_obj_set_align(ui_KeyboardWiFi, LV_ALIGN_BOTTOM_MID);
    lv_obj_add_flag(ui_KeyboardWiFi, LV_OBJ_FLAG_HIDDEN);

    // Save Button - to the right of SSID
    lv_obj_t *btn_save = lv_btn_create(ui_Screen6);
    lv_obj_set_size(btn_save, 90, 45);
    lv_obj_set_pos(btn_save, 305, 120);
    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, "Snimi");
    lv_obj_center(lbl_save);
    lv_obj_add_event_cb(btn_save, ui_event_SaveWiFi, LV_EVENT_CLICKED, NULL);

    // Cancel Button - to the right of Pass
    lv_obj_t *btn_cancel = lv_btn_create(ui_Screen6);
    lv_obj_set_size(btn_cancel, 90, 45);
    lv_obj_set_pos(btn_cancel, 305, 200);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x555555), 0);
    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, "Nazad");
    lv_obj_center(lbl_cancel);
    lv_obj_add_event_cb(btn_cancel, ui_event_CancelWiFi, LV_EVENT_CLICKED, NULL);
}

void ui_Screen6_screen_destroy(void) {
    if (ui_Screen6) lv_obj_del(ui_Screen6);
    ui_Screen6 = NULL;
}
