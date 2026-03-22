#include "../ui.h"
#include <stdio.h>

lv_obj_t *ui_Screen1 = NULL;
lv_obj_t *ui_LabelTime = NULL;
lv_obj_t *ui_LabelInfo = NULL;
lv_obj_t *ui_LabelPuls = NULL;
lv_obj_t *ui_LabelSpo = NULL;
lv_obj_t *ui_LabelGPS = NULL;
lv_obj_t *ui_LabelGPS_Icon = NULL; // GPS Icon
lv_obj_t *ui_LabelGSM_Icon = NULL;
lv_obj_t *ui_LabelGSM_Text = NULL;
lv_obj_t *ui_LabelBLT = NULL;
lv_obj_t *ui_LabelBLE_Icon = NULL; // BLE Icon
lv_obj_t *ui_LabelBatt = NULL; // Battery Text

// Event for Screen1 navigation
void ui_event_Screen1(lv_event_t *e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_GESTURE && lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_LEFT) {
        lv_indev_wait_release(lv_indev_get_act());
        _ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, &ui_Screen2_screen_init);
    }
}

void ui_Screen1_screen_init(void) {
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0x000000), 0);

    // --- TOP STATUS BAR AREA (5 Icons with equal spacing) ---
    lv_obj_t *status_bar = lv_obj_create(ui_Screen1);
    lv_obj_set_size(status_bar, 400, 60);
    lv_obj_set_align(status_bar, LV_ALIGN_TOP_MID);
    lv_obj_set_y(status_bar, 80); // Adjusted for 466x466 round screen to avoid clipping
    lv_obj_set_style_bg_opa(status_bar, 0, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status_bar, 5, 0); // Minimal gap for 5 icons
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    // 1. GPS Status
    lv_obj_t *gps_cont = lv_obj_create(status_bar);
    lv_obj_set_size(gps_cont, 64, 50);
    lv_obj_set_style_bg_opa(gps_cont, 0, 0);
    lv_obj_set_style_border_width(gps_cont, 0, 0);
    lv_obj_set_flex_flow(gps_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(gps_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(gps_cont, 0, 0);
    lv_obj_clear_flag(gps_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    ui_LabelGPS = lv_label_create(gps_cont);
    lv_label_set_text(ui_LabelGPS, "GPS");
    lv_obj_set_style_text_font(ui_LabelGPS, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ui_LabelGPS, lv_color_hex(0xFFFFFF), 0);

    ui_LabelGPS_Icon = lv_label_create(gps_cont);
    lv_label_set_text(ui_LabelGPS_Icon, LV_SYMBOL_GPS);
    lv_obj_set_style_text_font(ui_LabelGPS_Icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ui_LabelGPS_Icon, lv_color_hex(0xFFFFFF), 0);

    // 2. GSM Status
    lv_obj_t *gsm_cont = lv_obj_create(status_bar);
    lv_obj_set_size(gsm_cont, 64, 50);
    lv_obj_set_style_bg_opa(gsm_cont, 0, 0);
    lv_obj_set_style_border_width(gsm_cont, 0, 0);
    lv_obj_set_flex_flow(gsm_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(gsm_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(gsm_cont, 0, 0);
    lv_obj_clear_flag(gsm_cont, LV_OBJ_FLAG_SCROLLABLE);

    ui_LabelGSM_Text = lv_label_create(gsm_cont);
    lv_label_set_text(ui_LabelGSM_Text, "GSM");
    lv_obj_set_style_text_font(ui_LabelGSM_Text, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ui_LabelGSM_Text, lv_color_hex(0xFFFFFF), 0);

    ui_LabelGSM_Icon = lv_label_create(gsm_cont);
    lv_label_set_text(ui_LabelGSM_Icon, LV_SYMBOL_WIFI); // Using WIFI symbol for GSM as SIGNAL is not available in standard fonts
    lv_obj_set_style_text_font(ui_LabelGSM_Icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ui_LabelGSM_Icon, lv_color_hex(0xFFFFFF), 0);

    // 3. WiFi Status (WiFi label above icon)
    lv_obj_t *wifi_cont = lv_obj_create(status_bar);
    lv_obj_set_size(wifi_cont, 64, 50);
    lv_obj_set_style_bg_opa(wifi_cont, 0, 0);
    lv_obj_set_style_border_width(wifi_cont, 0, 0);
    lv_obj_set_flex_flow(wifi_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(wifi_cont, 0, 0);
    lv_obj_clear_flag(wifi_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *wifi_label = lv_label_create(wifi_cont);
    lv_label_set_text(wifi_label, "WiFi");
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *wifi_icon = lv_label_create(wifi_cont);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0xFFFFFF), 0);


    // 4. BLE Status (BLE label above icon)
    lv_obj_t *ble_cont = lv_obj_create(status_bar);
    lv_obj_set_size(ble_cont, 64, 50);
    lv_obj_set_style_bg_opa(ble_cont, 0, 0);
    lv_obj_set_style_border_width(ble_cont, 0, 0);
    lv_obj_set_flex_flow(ble_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ble_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(ble_cont, 0, 0);
    lv_obj_clear_flag(ble_cont, LV_OBJ_FLAG_SCROLLABLE);

    ui_LabelBLT = lv_label_create(ble_cont);
    lv_label_set_text(ui_LabelBLT, "BLE"); // Static text as requested
    lv_obj_set_style_text_font(ui_LabelBLT, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ui_LabelBLT, lv_color_hex(0xFFFFFF), 0);

    ui_LabelBLE_Icon = lv_label_create(ble_cont);
    lv_label_set_text(ui_LabelBLE_Icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_font(ui_LabelBLE_Icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ui_LabelBLE_Icon, lv_color_hex(0xFFFFFF), 0);


    // 5. Battery status
    lv_obj_t *batt_cont = lv_obj_create(status_bar);
    lv_obj_set_size(batt_cont, 64, 50);
    lv_obj_set_style_bg_opa(batt_cont, 0, 0);
    lv_obj_set_style_border_width(batt_cont, 0, 0);
    lv_obj_set_flex_flow(batt_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(batt_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(batt_cont, 0, 0);
    lv_obj_clear_flag(batt_cont, LV_OBJ_FLAG_SCROLLABLE);

    ui_LabelBatt = lv_label_create(batt_cont);
    lv_label_set_text(ui_LabelBatt, "100%"); // This stays dynamic for % but follows the style
    lv_obj_set_style_text_font(ui_LabelBatt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ui_LabelBatt, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *batt_icon = lv_label_create(batt_cont);
    lv_label_set_text(batt_icon, LV_SYMBOL_BATTERY_3);
    lv_obj_set_style_text_font(batt_icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(batt_icon, lv_color_hex(0xFFFFFF), 0);

    // (Previous activity icon removed to make space for WiFi as requested) 
    // Container 5 was reused for Battery in the new layout above.

    // --- MAIN TIME DISPLAY ---
    ui_LabelTime = lv_label_create(ui_Screen1);
    lv_obj_set_align(ui_LabelTime, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_LabelTime, -60); // Moved up to make room
    lv_label_set_text(ui_LabelTime, "00:00");
    lv_obj_set_style_text_font(ui_LabelTime, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(ui_LabelTime, lv_color_hex(0xFFFFFF), 0);

    // --- NEW PULSE AND SPO2 LAYOUT (Side-by-side columns) ---
    lv_obj_t *health_cont = lv_obj_create(ui_Screen1);
    lv_obj_set_size(health_cont, 400, 100);
    lv_obj_set_align(health_cont, LV_ALIGN_CENTER);
    lv_obj_set_y(health_cont, 30); // Moved up from 85 to make room for bottom logo
    lv_obj_set_style_bg_opa(health_cont, 0, 0);
    lv_obj_set_style_border_width(health_cont, 0, 0);
    lv_obj_set_flex_flow(health_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(health_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(health_cont, 40, 0);
    lv_obj_clear_flag(health_cont, LV_OBJ_FLAG_SCROLLABLE);

    // --- PULSE COLUMN ---
    lv_obj_t *puls_col = lv_obj_create(health_cont);
    lv_obj_set_size(puls_col, 150, 100);
    lv_obj_set_style_bg_opa(puls_col, 0, 0);
    lv_obj_set_style_border_width(puls_col, 0, 0);
    lv_obj_set_flex_flow(puls_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(puls_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(puls_col, 0, 0);
    lv_obj_clear_flag(puls_col, LV_OBJ_FLAG_SCROLLABLE);

    // Row 1: Pulse Icon + Label
    lv_obj_t *puls_hdr = lv_obj_create(puls_col);
    lv_obj_set_size(puls_hdr, 120, 35);
    lv_obj_set_style_bg_opa(puls_hdr, 0, 0);
    lv_obj_set_style_border_width(puls_hdr, 0, 0);
    lv_obj_set_flex_flow(puls_hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(puls_hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(puls_hdr, 5, 0);
    lv_obj_clear_flag(puls_hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *puls_img_obj = lv_img_create(puls_hdr);
    lv_img_set_src(puls_img_obj, &ui_img_pulse36_png);
    lv_img_set_zoom(puls_img_obj, 180); // Scale down slightly from 36px

    lv_obj_t *puls_title = lv_label_create(puls_hdr);
    lv_label_set_text(puls_title, "PULSE");
    lv_obj_set_style_text_font(puls_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(puls_title, lv_color_hex(0xFFFFFF), 0);

    // Row 2: Value + BPM
    lv_obj_t *puls_val_row = lv_obj_create(puls_col);
    lv_obj_set_size(puls_val_row, 120, 45);
    lv_obj_set_style_bg_opa(puls_val_row, 0, 0);
    lv_obj_set_style_border_width(puls_val_row, 0, 0);
    lv_obj_set_flex_flow(puls_val_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(puls_val_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(puls_val_row, LV_OBJ_FLAG_SCROLLABLE);

    ui_LabelPuls = lv_label_create(puls_val_row);
    lv_label_set_text(ui_LabelPuls, "--");
    lv_obj_set_style_text_font(ui_LabelPuls, &lv_font_montserrat_32, 0); // Large value
    lv_obj_set_style_text_color(ui_LabelPuls, lv_color_hex(0xFFFFFF), 0);
    
    lv_obj_t *bpm_lbl = lv_label_create(puls_val_row);
    lv_label_set_text(bpm_lbl, "BPM");
    lv_obj_set_style_text_font(bpm_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(bpm_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_left(bpm_lbl, 5, 0);

    // --- SPO2 COLUMN ---
    lv_obj_t *spo_col = lv_obj_create(health_cont);
    lv_obj_set_size(spo_col, 150, 100);
    lv_obj_set_style_bg_opa(spo_col, 0, 0);
    lv_obj_set_style_border_width(spo_col, 0, 0);
    lv_obj_set_flex_flow(spo_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(spo_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(spo_col, 0, 0);
    lv_obj_clear_flag(spo_col, LV_OBJ_FLAG_SCROLLABLE);

    // Row 1: SPO2 Icon + Label
    lv_obj_t *spo_hdr = lv_obj_create(spo_col);
    lv_obj_set_size(spo_hdr, 120, 35);
    lv_obj_set_style_bg_opa(spo_hdr, 0, 0);
    lv_obj_set_style_border_width(spo_hdr, 0, 0);
    lv_obj_set_flex_flow(spo_hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(spo_hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(spo_hdr, 5, 0);
    lv_obj_clear_flag(spo_hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *spo_img_obj = lv_img_create(spo_hdr);
    lv_img_set_src(spo_img_obj, &ui_img_1716151603);
    lv_img_set_zoom(spo_img_obj, 180);

    lv_obj_t *spo_title = lv_label_create(spo_hdr);
    lv_label_set_text(spo_title, "SPO2");
    lv_obj_set_style_text_font(spo_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(spo_title, lv_color_hex(0xFFFFFF), 0);

    // Row 2: Value + %
    lv_obj_t *spo_val_row = lv_obj_create(spo_col);
    lv_obj_set_size(spo_val_row, 120, 45);
    lv_obj_set_style_bg_opa(spo_val_row, 0, 0);
    lv_obj_set_style_border_width(spo_val_row, 0, 0);
    lv_obj_set_flex_flow(spo_val_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(spo_val_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(spo_val_row, LV_OBJ_FLAG_SCROLLABLE);

    ui_LabelSpo = lv_label_create(spo_val_row);
    lv_label_set_text(ui_LabelSpo, "--");
    lv_obj_set_style_text_font(ui_LabelSpo, &lv_font_montserrat_32, 0); // Large value
    lv_obj_set_style_text_color(ui_LabelSpo, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *pct_lbl = lv_label_create(spo_val_row);
    lv_label_set_text(pct_lbl, "%");
    lv_obj_set_style_text_font(pct_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(pct_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_left(pct_lbl, 5, 0);


    // --- LOGO AREA AT THE BOTTOM (Style from Screen 2) ---
    lv_obj_t *ui_LogoImage_S1 = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_LogoImage_S1, &ui_img_logo128_png);
    lv_obj_set_size(ui_LogoImage_S1, 100, 100); // Slightly smaller than Screen 2 for S1
    lv_obj_set_align(ui_LogoImage_S1, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_LogoImage_S1, 160);
    lv_img_set_zoom(ui_LogoImage_S1, 120);

    lv_obj_t *lab_life = lv_label_create(ui_Screen1);
    lv_label_set_text(lab_life, "Life");
    lv_obj_set_style_text_font(lab_life, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(lab_life, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_align(lab_life, LV_ALIGN_CENTER);
    lv_obj_set_pos(lab_life, -85, 160);

    lv_obj_t *lab_link = lv_label_create(ui_Screen1);
    lv_label_set_text(lab_link, "Link");
    lv_obj_set_style_text_font(lab_link, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(lab_link, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_align(lab_link, LV_ALIGN_CENTER);
    lv_obj_set_pos(lab_link, 85, 160);

    // --- INFO LABEL (Status Message/Alerts) ---
    // Repositioned to sit ABOVE the logo
    ui_LabelInfo = lv_label_create(ui_Screen1);
    lv_obj_set_align(ui_LabelInfo, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_LabelInfo, 105);
    lv_label_set_text(ui_LabelInfo, "LifeLink Aktivan"); // Initial text
    lv_obj_set_style_text_font(ui_LabelInfo, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ui_LabelInfo, lv_color_hex(0x00FF00), 0);
    lv_obj_clear_flag(ui_LabelInfo, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_Screen1, ui_event_Screen1, LV_EVENT_ALL, NULL);
}

void ui_Screen1_screen_destroy(void) {
    if (ui_Screen1) lv_obj_del(ui_Screen1);
    ui_Screen1 = NULL;
}
