// LifeLink Settings Screen (Screen 3)
// Custom numpad for entering SMS alert phone number on 466x466 round AMOLED

#include "../ui.h"
#include "ui_Screen5.h"
#include <string.h>

lv_obj_t *ui_Screen3 = NULL;
lv_obj_t *ui_TextAreaPhone = NULL;
lv_obj_t *ui_SwitchBLE = NULL;
lv_obj_t *ui_SwitchWiFi = NULL;
lv_obj_t *ui_SwitchSMS = NULL;
lv_obj_t *ui_SwitchCall = NULL;
lv_obj_t *ui_SwitchSOS = NULL;
char g_phone_number[20] = "";

extern bool g_w_enable_sms;
extern bool g_w_enable_call;
extern bool g_w_enable_sos;
extern bool g_wifi_enabled;
extern void save_settings();
void toggle_wifi(bool enable);

static void toggle_sms_cb(lv_event_t *e) {
    g_w_enable_sms = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    save_settings();
}
static void toggle_call_cb(lv_event_t *e) {
    g_w_enable_call = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    save_settings();
}
static void toggle_sos_cb(lv_event_t *e) {
    g_w_enable_sos = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    save_settings();
}

static void edit_sms_cb(lv_event_t *e) { ui_open_numpad(EDIT_MODE_SMS); }
static void edit_call_cb(lv_event_t *e) { ui_open_numpad(EDIT_MODE_CALL); }
static void edit_sos_cb(lv_event_t *e) { ui_open_numpad(EDIT_MODE_SOS); }

extern void ui_Screen6_screen_init(void);
static void edit_wifi_cb(lv_event_t *e) {
    _ui_screen_change(&ui_Screen6, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, &ui_Screen6_screen_init);
}

static lv_obj_t *create_row(lv_obj_t *parent, const char *label, int y, lv_event_cb_t toggle_cb, lv_event_cb_t edit_cb, bool current_state) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 360, 50);
    lv_obj_set_pos(row, 50, y);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, label);
    lv_obj_set_align(lbl, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_set_size(sw, 45, 25);
    lv_obj_set_align(sw, LV_ALIGN_CENTER);
    if (current_state) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, toggle_cb, LV_EVENT_VALUE_CHANGED, NULL);

    if (edit_cb) {
        lv_obj_t *btn = lv_btn_create(row);
        lv_obj_set_size(btn, 70, 30);
        lv_obj_set_align(btn, LV_ALIGN_RIGHT_MID);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x004488), 0);
        lv_obj_t *bl = lv_label_create(btn);
        lv_label_set_text(bl, "Podesi");
        lv_obj_center(bl);
        lv_obj_add_event_cb(btn, edit_cb, LV_EVENT_CLICKED, NULL);
    }
    return row;
}

// --- Gesture navigation ---
// Screen 3 is the last screen in the swipe chain (Screen1 -> Screen2 -> Screen3).
// Swipe-right goes back to Screen2. Swipe-left is intentionally disabled so the
// screensaver (AOD screen with exact time) is NOT accessible via gestures.
void ui_event_Screen3(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if (event_code == LV_EVENT_GESTURE && lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
    {
        lv_indev_wait_release(lv_indev_get_act());
        _ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_Screen2_screen_init);
    }
    // Swipe-left from Screen3 is disabled: AOD/screensaver is not part of the swipe chain.
}

void ui_Screen3_screen_init(void)
{
    ui_Screen3 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen3, lv_color_hex(0x000a14), 0);

    lv_obj_t *title = lv_label_create(ui_Screen3);
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title, 40);
    lv_label_set_text(title, "Podesavanja Sata");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);

    int sy = 90, gap = 55;
    create_row(ui_Screen3, "BLE", sy, ui_event_SwitchBLE, NULL, true);
    create_row(ui_Screen3, "WiFi", sy + gap, ui_event_SwitchWiFi, edit_wifi_cb, g_wifi_enabled);
    create_row(ui_Screen3, "SMS", sy + 2*gap, toggle_sms_cb, edit_sms_cb, g_w_enable_sms);
    create_row(ui_Screen3, "Poziv", sy + 3*gap, toggle_call_cb, edit_call_cb, g_w_enable_call);
    create_row(ui_Screen3, "SOS", sy + 4*gap, toggle_sos_cb, edit_sos_cb, g_w_enable_sos);

    lv_obj_add_event_cb(ui_Screen3, ui_event_Screen3, LV_EVENT_ALL, NULL);
}

void ui_Screen3_screen_destroy(void)
{
    if (ui_Screen3)
        lv_obj_del(ui_Screen3);
    ui_Screen3 = NULL;
    ui_SwitchBLE = NULL;
    ui_SwitchWiFi = NULL;
    ui_SwitchSMS = NULL;
    ui_SwitchCall = NULL;
    ui_SwitchSOS = NULL;
}

const char *ui_get_phone_number(void)
{
    return g_phone_number;
}
