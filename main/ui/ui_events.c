#include "ui.h"
#include "esp_log.h"

// External functions defined in lifelink.cpp
extern void toggle_ble(bool enable);
extern void toggle_wifi(bool enable);

void ui_event_SwitchBLE(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        bool state = lv_obj_has_state(target, LV_STATE_CHECKED);
        ESP_LOGI("UI", "BLE Switch: %d", state);
        toggle_ble(state);
    }
}

void ui_event_SwitchWiFi(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_VALUE_CHANGED)
    {
        bool state = lv_obj_has_state(target, LV_STATE_CHECKED);
        ESP_LOGI("UI", "WiFi Switch: %d", state);
        toggle_wifi(state);
    }
}

void start_fall_countdown_ui(bool is_simulated)
{
    ESP_LOGW("UI", "Starting fall countdown UI (Simulated: %d)", is_simulated);
    // Switch to Screen 4 (Alert Screen)
    if (ui_Screen4) {
        _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen4_screen_init);
    }
}

void btn_simulate_fall_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        start_fall_countdown_ui(true);
    }
}
