#include "ui.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>


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

    // === NOTIFY MOBILE APP via BLE ===
    // This triggers ApiService.saveFallEvent() in the Flutter app,
    // which writes to the fall_events table in MariaDB.
    // Without this, the app never knows a fall occurred.
    extern float g_total_snapshot;
    extern int32_t heartRate;
    extern int32_t spo2;
    extern float g_latitude, g_longitude;
    extern void ble_spp_server_send_data(uint8_t *data, uint16_t len);

    char ble_msg[128];
    if (is_simulated) {
        snprintf(ble_msg, sizeof(ble_msg),
                 "FALL_DETECTED G:%.2f P:%d S:%d B:0 Lat:%.5f Lon:%.5f",
                 (double)g_total_snapshot, (int)heartRate, (int)spo2,
                 (double)g_latitude, (double)g_longitude);
    } else {
        snprintf(ble_msg, sizeof(ble_msg),
                 "FALL_ACCEPTED G:%.2f P:%d S:%d B:0 Lat:%.5f Lon:%.5f",
                 (double)g_total_snapshot, (int)heartRate, (int)spo2,
                 (double)g_latitude, (double)g_longitude);
    }
    ble_spp_server_send_data((uint8_t *)ble_msg, (uint16_t)strlen(ble_msg));

    // Switch to Screen 4 (Alert Screen)
    _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen4_screen_init);
}

void btn_simulate_fall_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        start_fall_countdown_ui(true);
    }
}
