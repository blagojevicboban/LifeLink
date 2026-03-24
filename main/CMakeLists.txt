set(SRCS 
    "gsm_a6.c" "axp2101.c" "ble_spp_server.c" "lifelink.cpp" "fft_algo.c"
    "ui/ui.c"
    "ui/ui_helpers.c"
    "ui/screens/ui_Screen1.c"
    "ui/screens/ui_Screen2.c"
    "ui/screens/ui_Screen3.c"
    "ui/screens/ui_Screen4.c"
    "ui/screens/ui_Screen5.c"
    "ui/screens/ui_Screen6.c"
    "ui/images/ui_img_logo128_png.c"
    "ui/images/ui_img_pulse36_png.c"
    "ui/images/ui_img_1716151603.c"
    "ui/ui_events.c"
)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "." "ui"
    REQUIRES nvs_flash esp_wifi esp_event esp_netif esp_http_client mbedtls json driver SensorLib esp_lcd_sh8601 lc76g max30102 bt lvgl esp-dsp esp_timer esp_lcd
)

set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE)
