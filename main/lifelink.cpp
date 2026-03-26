#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/queue.h"
#include "SensorLib.h"
#include "TouchDrvCST92xx.h"
#include "lvgl.h"
#include "lv_demos.h"
#include "esp_lcd_sh8601.h"
#include "ui/ui.h"
#include "SensorQMI8658.hpp" // Ensure this path is correct
#include "ble_spp_server.h"
#include "max30102.h"
#include "algorithm.h"
#include "lc76g.h"
#include "fft_algo.h"
#include "axp2101.h"
#include "gsm_a6.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include <sys/time.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_mac.h"
#include <string.h>
#include <ctype.h>

// --- Global Settings Variables ---
int g_screen_timeout_ms = 15000;
float g_fall_threshold_low = 0.6f;
float g_fall_threshold_high = 3.5f;
float g_stillness_tolerance = 0.2f;
float g_angle_threshold_deg = 60.0f;
int g_stillness_duration_ms = 5000;

bool g_wifi_enabled = false;
char g_wifi_ssid[32] = "";
char g_wifi_pass[64] = "";
bool g_w_enable_sms = false;
char g_w_sms_numbers[128] = "";
bool g_w_enable_call = false;
char g_w_call_numbers[128] = "";
bool g_w_enable_sos = false;
char g_w_sos_number[20] = "";
uint32_t g_sleep_hr_interval_s = 10; 
uint32_t g_sleep_hr_duration_s = 10;  // 10 seconds default
int g_action_origin = 0; // 0: Watch Only, 1: App + Watch

bool g_is_aod_mode = true;
static volatile uint64_t g_last_touch_time = 0; // MICROSECONDS
static volatile uint64_t last_touch_time = 0;   // MILLISECONDS

lv_obj_t *ui_AODScreen = NULL;
lv_obj_t *ui_AODTime = NULL;

extern "C" lv_obj_t * ui_Screen1; // Dashboard
extern "C" lv_obj_t * ui_LabelWiFi_Icon; // Added for status updates
SemaphoreHandle_t g_i2c_mux = NULL; 

void create_aod_screen() {
    ui_AODScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_AODScreen, lv_color_hex(0x000000), 0);
    
    ui_AODTime = lv_label_create(ui_AODScreen);
    lv_obj_set_width(ui_AODTime, 466); // Full screen width for perfect centering
    lv_obj_set_style_text_align(ui_AODTime, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(ui_AODTime, "--:--");
    lv_obj_set_style_text_color(ui_AODTime, lv_color_hex(0x404040), 0); // Dim gray
    lv_obj_set_style_text_font(ui_AODTime, &lv_font_montserrat_48, 0);
    lv_obj_set_style_transform_zoom(ui_AODTime, 768, 0); // 3x size
    
    // Zoom symmetrically from the center of the 466px width
    lv_obj_set_style_transform_pivot_x(ui_AODTime, 233, 0); 
    lv_obj_set_style_transform_pivot_y(ui_AODTime, 24, 0); 
    lv_obj_set_align(ui_AODTime, LV_ALIGN_CENTER);
    
    // Wake up on touch
    lv_obj_add_event_cb(ui_AODScreen, [](lv_event_t * e) {
        lv_scr_load_anim(ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, false);
        g_last_touch_time = esp_timer_get_time();
    }, LV_EVENT_CLICKED, NULL);
}
static bool s_wifi_connected = false;
static bool s_wifi_init_done = false;

#define SCREEN_TIMEOUT_MS g_screen_timeout_ms // Backward compatibility with existing code
extern "C" void start_fall_countdown_ui(bool is_simulated);

// SensorQMI8658 ACCELEROMETER BEGIN
// I2C configuration
#define I2C_MASTER_SCL 14
#define I2C_MASTER_SDA 15
#define I2C_MASTER_NUM I2C_NUM_0
#define TCA9554_ADDR   0x20
#define QMI8658_ADDRESS 0x6B // Replace with your QMI8658 address
// --- Konstante za detekciju pada (Advanced) ---
#define FALL_THRESHOLD_LOW g_fall_threshold_low
#define FALL_THRESHOLD_HIGH g_fall_threshold_high
#define STILLNESS_TOLERANCE g_stillness_tolerance
#define ANGLE_THRESHOLD_DEG g_angle_threshold_deg
#define STILLNESS_DURATION_MS g_stillness_duration_ms

enum FallDetectionState
{
    IDLE,
    FREE_FALL,
    IMPACT_DETECTED,
    WAITING_FOR_STILLNESS
};
FallDetectionState fallState = IDLE;
unsigned long stateTimer = 0;
int fallCount = 0;
int potentialFallCount = 0;

// Variables for Orientation Check
float ref_ax = 0, ref_ay = 0, ref_az = 0;    // Pre-fall orientation
float curr_ax = 0, curr_ay = 0, curr_az = 0; // Current orientation

SensorQMI8658 qmi;
IMUdata acc;
IMUdata gyr;
MAX30102 max30102;

static const char *TAGA = "QMI8658"; // Define a tag for logging

// I2C configuration consolidated below
#define I2C_MASTER_FREQ_HZ 100000 // Standard speed for I2C GPS
#define I2C_MASTER_SDA_IO (gpio_num_t) I2C_MASTER_SDA
#define I2C_MASTER_SCL_IO (gpio_num_t) I2C_MASTER_SCL

// i2c_master_init removed - using shared i2c_init instead

// Variables for MAX30102 algorithm (Moved to Top for Scope Visibility)
#define MAX_BRIGHTNESS 255
#define TEST_BUFFER_LENGTH 512 // Power of 2 for FFT

uint32_t irBuffer[TEST_BUFFER_LENGTH];
uint32_t redBuffer[TEST_BUFFER_LENGTH];

// Float buffers for FFT (Global to save stack)
float irBufferFloat[TEST_BUFFER_LENGTH];
float redBufferFloat[TEST_BUFFER_LENGTH];

int32_t bufferLength = TEST_BUFFER_LENGTH;

// --- Global Metrics ---
int32_t heartRate = 0;
int32_t spo2 = 0;
float fft_hr = 0;
float fft_spo2 = 0;
float g_latitude = 0, g_longitude = 0;
float g_total_snapshot = 1.0f;
int g_batt_pct_snapshot = 100;
char g_mac_str[18] = {0}; // Global MAC string for API ID

#define MARIADB_API_URL "http://lifelink.tsp.edu.rs/api/update.php" // Zvanična produkciona adresa na tsp.edu.rs

// Forward Declarations
static bool example_lvgl_lock(int timeout_ms);
static void example_lvgl_unlock(void);

// Helper: Convert NMEA scalar (DDDMM.MMMM) to Decimal Degrees
float nmea_to_decimal(float nmea_val)
{
    int degrees = (int)(nmea_val / 100);
    float minutes = nmea_val - (degrees * 100);
    return degrees + (minutes / 60.0f);
}

// Simple NMEA Parser (Handles $GNRMC)
void parse_nmea(char *line)
{
    if (strncmp(line, "$GNRMC", 6) == 0 || strncmp(line, "$GNGGA", 6) == 0)
    {
        // Example: $GNRMC,123519.000,A,4807.038,N,01131.000,E,...
        // We will simple-scan for simplicity. For robust parsing, use minmea or similar.
        // But sscanf is often enough if format is standard.

        char type[10];
        char time[15];
        char status; // for RMC
        float raw_lat, raw_lon;
        char ns, ew;

        // Try RMC first
        if (strncmp(line, "$GNRMC", 6) == 0)
        {
            // $GNRMC,time,status,lat,NS,lon,EW,spd,cog,date,...
            // Status: A=Active, V=Void
            if (sscanf(line, "%[^,],%[^,],%c,%f,%c,%f,%c", type, time, &status, &raw_lat, &ns, &raw_lon, &ew) >= 7)
            {
                if (status == 'A')
                {
                    float lat = nmea_to_decimal(raw_lat);
                    if (ns == 'S') lat = -lat;
                    float lon = nmea_to_decimal(raw_lon);
                    if (ew == 'W') lon = -lon;

                    g_latitude = lat;
                    g_longitude = lon;
                    ESP_LOGI("GPS", "Fix Status: ACTIVE | Lat: %.5f, Lon: %.5f", g_latitude, g_longitude);

                    if (example_lvgl_lock(-1))
                    {
                        if (ui_LabelGPS) lv_obj_set_style_text_color(ui_LabelGPS, lv_color_hex(0x00FF00), LV_PART_MAIN);
                        if (ui_LabelGPS_Icon) lv_obj_set_style_text_color(ui_LabelGPS_Icon, lv_color_hex(0x00FF00), LV_PART_MAIN);
                        example_lvgl_unlock();
                    }
                }
                else
                {
                    ESP_LOGW("GPS", "Fix Status: VOID (No fix yet). Searching...");
                    if (example_lvgl_lock(-1))
                    {
                        if (ui_LabelGPS) lv_obj_set_style_text_color(ui_LabelGPS, lv_color_hex(0xFF0000), LV_PART_MAIN);
                        if (ui_LabelGPS_Icon) lv_obj_set_style_text_color(ui_LabelGPS_Icon, lv_color_hex(0xFF0000), LV_PART_MAIN);
                        example_lvgl_unlock();
                    }
                }
            }
        }
        // Try GGA (if RMC fails or we want alt) - purely as fallback for coords
        else if (strncmp(line, "$GNGGA", 6) == 0)
        {
            // $GNGGA,time,lat,NS,lon,EW,fix,...
            int fix;
            if (sscanf(line, "%[^,],%[^,],%f,%c,%f,%c,%d", type, time, &raw_lat, &ns, &raw_lon, &ew, &fix) >= 7)
            {
                if (fix > 0)
                {
                    float lat = nmea_to_decimal(raw_lat);
                    if (ns == 'S')
                        lat = -lat;
                    float lon = nmea_to_decimal(raw_lon);
                    if (ew == 'W')
                        lon = -lon;

                    g_latitude = lat;
                    g_longitude = lon;
                    // ESP_LOGI("GPS_PARSED", "GGA Lat: %.5f, Lon: %.5f", g_latitude, g_longitude);
                }
            }
        }
    }
}

void read_sensor_data(void *arg); // Function declaration

void setup_accel()
{
    // i2c_master_init(); // Removed: I2C is already initialized by i2c_init() in app_main

    // Initialize QMI8658 sensor with 4 parameters (port number, address, SDA, SCL)
    // Note: SensorLib version in this project uses init() or begin() - check return type
    if (!qmi.begin(I2C_MASTER_NUM, QMI8658_ADDRESS, I2C_MASTER_SDA, I2C_MASTER_SCL))
    {
        ESP_LOGE(TAGA, "Failed to find QMI8658 - check your wiring!");
        return; 
    }

    // Get chip ID
    ESP_LOGI(TAGA, "Device ID: %x", qmi.getChipID());

    // Configure accelerometer
    qmi.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_1000Hz,
        SensorQMI8658::LPF_MODE_0,
        true);

    // Configure gyroscope
    qmi.configGyroscope(
        SensorQMI8658::GYR_RANGE_64DPS,
        SensorQMI8658::GYR_ODR_896_8Hz,
        SensorQMI8658::LPF_MODE_3,
        true);

    // Enable gyroscope and accelerometer
    qmi.enableGyroscope();
    qmi.enableAccelerometer();

    ESP_LOGI(TAGA, "Ready to read data...");
}

void setup_max30102()
{
    if (!max30102.begin(I2C_MASTER_NUM))
    {
        ESP_LOGE("MAX30102", "MAX30102 begin() FAILED - Check I2C connections");
    }
    else
    {
        uint8_t partID = max30102.readRegister8(0xFF);
        uint8_t revID = max30102.readRegister8(0xFE);
        ESP_LOGI("MAX30102", "MAX30102 Found. PartID: 0x%02X, RevID: 0x%02X", partID, revID);
        
        max30102.writeRegister8(0x09, 0x40); // MODE_CONFIG register, reset bit
        vTaskDelay(pdMS_TO_TICKS(100));
        max30102.wakeUp();
        // Power=0x24 (~7mA), Avg=1, Mode=2(Red+IR), Rate=400Hz, Width=411, Range=4096
        max30102.setup(0x24, 1, 2, 400, 411, 4096); 
        
        vTaskDelay(pdMS_TO_TICKS(100));
        max30102.clearFIFO();
        fft_init(TEST_BUFFER_LENGTH);
    }
}
// SensorQMI8658 ACCELEROMETER END

static const char *TAG = "example";
static SemaphoreHandle_t lvgl_mux = NULL;
esp_lcd_panel_handle_t panel_handle = NULL; // Moved to global for power management
esp_lcd_panel_io_handle_t io_handle = NULL;   // Global for brightness control

#define LCD_HOST SPI2_HOST
#define TOUCH_HOST I2C_NUM_0

#if CONFIG_LV_COLOR_DEPTH == 32
#define LCD_BIT_PER_PIXEL (24)
#elif CONFIG_LV_COLOR_DEPTH == 16
#define LCD_BIT_PER_PIXEL (16)
#endif

#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_LCD_CS (GPIO_NUM_12)
#define EXAMPLE_PIN_NUM_LCD_PCLK (GPIO_NUM_38)
#define EXAMPLE_PIN_NUM_LCD_DATA0 (GPIO_NUM_4)
#define EXAMPLE_PIN_NUM_LCD_DATA1 (GPIO_NUM_5)
#define EXAMPLE_PIN_NUM_LCD_DATA2 (GPIO_NUM_6)
#define EXAMPLE_PIN_NUM_LCD_DATA3 (GPIO_NUM_7)
#define EXAMPLE_PIN_NUM_LCD_RST (GPIO_NUM_39)
#define EXAMPLE_PIN_NUM_BK_LIGHT (-1)

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES 466
#define EXAMPLE_LCD_V_RES 466

#define EXAMPLE_USE_TOUCH 1

#if EXAMPLE_USE_TOUCH
#define EXAMPLE_PIN_NUM_TOUCH_SCL (GPIO_NUM_14)
#define EXAMPLE_PIN_NUM_TOUCH_SDA (GPIO_NUM_15)
#define EXAMPLE_PIN_NUM_TOUCH_RST (GPIO_NUM_40)
#define EXAMPLE_PIN_NUM_TOUCH_INT (GPIO_NUM_11)

// Consolidated definitions at the top
// #define I2C_MASTER_NUM (i2c_port_t)1
// #define I2C_MASTER_FREQ_HZ 100000
// #define I2C_MASTER_SDA_IO (gpio_num_t)15
// #define I2C_MASTER_SCL_IO (gpio_num_t)14
#define Touch_INT (gpio_num_t)11
#define Touch_RST (gpio_num_t)40

#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS 1000
uint8_t touchAddress = 0x5A;

TouchDrvCST92xx touch;
int16_t x[5], y[5];
bool isPressed = false;

#endif

#define EXAMPLE_LVGL_BUF_HEIGHT (EXAMPLE_LCD_V_RES / 20) // Reduced to save internal RAM for WiFi
 
#define EXAMPLE_LVGL_TICK_PERIOD_MS 10 
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE (12 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY 10

static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xC4, (uint8_t[]){0x80}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x53, (uint8_t[]){0x20}, 1, 0},
    {0x51, (uint8_t[]){0xFF}, 1, 0}, // RESTORE MAX BRIGHTNESS
    {0x63, (uint8_t[]){0xFF}, 1, 0},
    {0x2A, (uint8_t[]){0x00, 0x06, 0x01, 0xD7}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4, 600},
    {0x11, NULL, 0, 600}, // 命令后延时 600ms
    {0x29, NULL, 0, 0},   // 无延时
};

esp_err_t i2c_init(void)
{
    i2c_config_t i2c_conf;
    memset(&i2c_conf, 0, sizeof(i2c_conf));
    i2c_conf.mode = I2C_MODE_MASTER;
    i2c_conf.sda_io_num = I2C_MASTER_SDA_IO;
    i2c_conf.scl_io_num = I2C_MASTER_SCL_IO;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    
    return i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, ESP_INTR_FLAG_IRAM);
}

void read_sensor_data(void *arg); // Function declaration
void gps_task(void *arg);
void gsm_status_task(void *arg);
extern "C" void wifi_reconnect_now();

void setup_sensor()
{
    uint8_t touchAddress = 0x5A;

    touch.setPins(Touch_RST, Touch_INT);
    touch.begin(I2C_MASTER_NUM, touchAddress, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    touch.reset();
    touch.setMaxCoordinates(466, 466);
    touch.setMirrorXY(true, true);
}

// --- Screen Timeout Variables ---
static bool screen_is_on = true;
// volatile last_touch_time moved to global section above

void reset_screen_timer()
{
    g_last_touch_time = esp_timer_get_time(); // Update global touch time
    last_touch_time = esp_timer_get_time() / 1000; // Keep local for screen_is_on logic

    if (!screen_is_on)
    {
        screen_is_on = true;
        // Wake up LCD (AMOLED uses commands, not just backlight)
        if (panel_handle)
        {
            esp_lcd_panel_disp_on_off(panel_handle, true);
        }
        // Turn on backlight (if exists)
#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
        gpio_set_level((gpio_num_t)EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);
#endif
        ESP_LOGI("PWR", "Screen WAKE");
        
        // Restore brightness on wake
        uint8_t brightness = 0xFF; 
        esp_lcd_panel_io_tx_param(io_handle, 0x51, &brightness, 1);
        
        if (lv_scr_act() == ui_AODScreen) {
            lv_scr_load_anim(ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
        }
    }

    // AOD logic
    uint64_t now = esp_timer_get_time();
    if (g_is_aod_mode && (now - g_last_touch_time > (uint64_t)SCREEN_TIMEOUT_MS * 1000)) {
        if (lv_scr_act() != ui_AODScreen) {
            if (ui_AODScreen == NULL) create_aod_screen();
            lv_scr_load_anim(ui_AODScreen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
        }
    } else {
        if (lv_scr_act() == ui_AODScreen) {
            lv_scr_load_anim(ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
        }
    }
}

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    const int offsetx1 = area->x1; //+ 0x16;
    const int offsetx2 = area->x2; //+ 0x16;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;

#if LCD_BIT_PER_PIXEL == 24
    uint8_t *to = (uint8_t *)color_map;
    uint8_t temp = 0;
    uint16_t pixel_num = (offsetx2 - offsetx1 + 1) * (offsety2 - offsety1 + 1);

    // Special dealing for first pixel
    temp = color_map[0].ch.blue;
    *to++ = color_map[0].ch.red;
    *to++ = color_map[0].ch.green;
    *to++ = temp;
    // Normal dealing for other pixels
    for (int i = 1; i < pixel_num; i++)
    {
        *to++ = color_map[i].ch.red;
        *to++ = color_map[i].ch.green;
        *to++ = color_map[i].ch.blue;
    }
#endif

    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void example_lvgl_update_cb(lv_disp_drv_t *drv)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;

    switch (drv->rotated)
    {
    case LV_DISP_ROT_NONE:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;
    case LV_DISP_ROT_90:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
        break;
    case LV_DISP_ROT_180:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;
    case LV_DISP_ROT_270:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);
        break;
    }
}

void example_lvgl_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;

    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;

    // round the start of coordinate down to the nearest 2M number
    area->x1 = (x1 >> 1) << 1;
    area->y1 = (y1 >> 1) << 1;
    // round the end of coordinate up to the nearest 2N+1 number
    area->x2 = ((x2 >> 1) << 1) + 1;
    area->y2 = ((y2 >> 1) << 1) + 1;
}

#if EXAMPLE_USE_TOUCH
static void example_lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint8_t touched = 0;
    if (g_i2c_mux && xSemaphoreTake(g_i2c_mux, pdMS_TO_TICKS(100))) {
        touched = touch.getPoint(x, y, 2);
        xSemaphoreGive(g_i2c_mux);
    }

    if (touched)
    {
        data->point.x = x[0];
        data->point.y = y[0];
        data->state = LV_INDEV_STATE_PRESSED;
        reset_screen_timer();
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
#endif

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static bool example_lvgl_lock(int timeout_ms)
{
    assert(lvgl_mux && "bsp_display_start must be called first");

    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

static void example_lvgl_unlock(void)
{
    assert(lvgl_mux && "bsp_display_start must be called first");
    xSemaphoreGive(lvgl_mux);
}

extern "C" void gsm_update_ui_status(const char *text)
{
    if (lvgl_mux != NULL && example_lvgl_lock(-1))
    {
        if (ui_LabelGSM)
        {
            lv_label_set_text(ui_LabelGSM, text);
        }
        if (ui_LabelGSM_Icon)
        {
            lv_obj_set_style_text_color(ui_LabelGSM_Icon, lv_color_hex(0xFFA500), LV_PART_MAIN); // Orange indicating connecting/processing
            if (ui_LabelGSM_Text) lv_obj_set_style_text_color(ui_LabelGSM_Text, lv_color_hex(0xFFA500), LV_PART_MAIN);
        }
        example_lvgl_unlock();
    }
}

static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    while (1)
    {
        // Lock the mutex due to the LVGL APIs are not thread-safe
        if (example_lvgl_lock(-1))
        {
            task_delay_ms = lv_timer_handler();
            // Release the mutex
            example_lvgl_unlock();
        }
        if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS)
        {
            task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
        }
        else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS)
        {
            task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}
// --- Settings Management ---
void load_settings()
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK)
    {
        nvs_get_i32(my_handle, "screen_timeout", (int32_t *)&g_screen_timeout_ms);
        if (g_screen_timeout_ms < 5000) g_screen_timeout_ms = 15000; // Sanitize: min 5s, default 15s

        uint32_t val;
        if (nvs_get_u32(my_handle, "fall_low", &val) == ESP_OK)
            memcpy(&g_fall_threshold_low, &val, sizeof(float));
        if (nvs_get_u32(my_handle, "fall_high", &val) == ESP_OK)
            memcpy(&g_fall_threshold_high, &val, sizeof(float));
        if (nvs_get_u32(my_handle, "still_tol", &val) == ESP_OK)
            memcpy(&g_stillness_tolerance, &val, sizeof(float));
        if (nvs_get_u32(my_handle, "angle_thr", &val) == ESP_OK)
            memcpy(&g_angle_threshold_deg, &val, sizeof(float));
        nvs_get_i32(my_handle, "still_dur", (int32_t *)&g_stillness_duration_ms);

        size_t sz = sizeof(g_wifi_ssid);
        nvs_get_str(my_handle, "wifi_ssid", g_wifi_ssid, &sz);
        sz = sizeof(g_wifi_pass);
        nvs_get_str(my_handle, "wifi_pass", g_wifi_pass, &sz);
        sz = sizeof(g_w_sms_numbers);
        nvs_get_str(my_handle, "sms_nums", g_w_sms_numbers, &sz);
        sz = sizeof(g_w_call_numbers);
        nvs_get_str(my_handle, "call_nums", g_w_call_numbers, &sz);
        sz = sizeof(g_w_sos_number);
        nvs_get_str(my_handle, "sos_num", g_w_sos_number, &sz);

        uint8_t b_val;
        if (nvs_get_u8(my_handle, "wifi_en", &b_val) == ESP_OK) g_wifi_enabled = b_val;
        if (nvs_get_u8(my_handle, "en_sms", &b_val) == ESP_OK) g_w_enable_sms = b_val;
        if (nvs_get_u8(my_handle, "en_call", &b_val) == ESP_OK) g_w_enable_call = b_val;
        nvs_get_u8(my_handle, "en_sos", &b_val);
        if (nvs_get_u8(my_handle, "en_sos", &b_val) == ESP_OK) g_w_enable_sos = b_val;
        nvs_get_i32(my_handle, "act_orig", (int32_t *)&g_action_origin);
        
        nvs_get_i32(my_handle, "hr_slp_int", (int32_t *)&g_sleep_hr_interval_s);
        nvs_get_i32(my_handle, "hr_slp_dur", (int32_t *)&g_sleep_hr_duration_s);

        nvs_close(my_handle);
        ESP_LOGI("SETTINGS", "Loaded settings from NVS");
    }
}

extern "C" void save_settings()
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK)
    {
        nvs_set_i32(my_handle, "screen_timeout", g_screen_timeout_ms);

        uint32_t val;
        memcpy(&val, &g_fall_threshold_low, sizeof(float));
        nvs_set_u32(my_handle, "fall_low", val);
        memcpy(&val, &g_fall_threshold_high, sizeof(float));
        nvs_set_u32(my_handle, "fall_high", val);
        memcpy(&val, &g_stillness_tolerance, sizeof(float));
        nvs_set_u32(my_handle, "still_tol", val);
        memcpy(&val, &g_angle_threshold_deg, sizeof(float));
        nvs_set_u32(my_handle, "angle_thr", val);
        nvs_set_i32(my_handle, "still_dur", g_stillness_duration_ms);

        nvs_set_str(my_handle, "wifi_ssid", g_wifi_ssid);
        nvs_set_str(my_handle, "wifi_pass", g_wifi_pass);
        nvs_set_str(my_handle, "sms_nums", g_w_sms_numbers);
        nvs_set_str(my_handle, "call_nums", g_w_call_numbers);
        nvs_set_str(my_handle, "sos_num", g_w_sos_number);

        nvs_set_u8(my_handle, "wifi_en", g_wifi_enabled);
        nvs_set_u8(my_handle, "en_sms", g_w_enable_sms);
        nvs_set_u8(my_handle, "en_call", g_w_enable_call);
        nvs_set_u8(my_handle, "en_sos", g_w_enable_sos);
        nvs_set_i32(my_handle, "act_orig", g_action_origin);
        nvs_set_i32(my_handle, "hr_slp_int", g_sleep_hr_interval_s);
        nvs_set_i32(my_handle, "hr_slp_dur", g_sleep_hr_duration_s);

        nvs_commit(my_handle);
        nvs_close(my_handle);
        ESP_LOGI("SETTINGS", "Saved settings to NVS");
    }
}

extern "C" void spp_read_cb(uint8_t **data, uint16_t *len)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "screen_timeout", g_screen_timeout_ms);
    cJSON_AddNumberToObject(root, "fall_low", g_fall_threshold_low);
    cJSON_AddNumberToObject(root, "fall_high", g_fall_threshold_high);
    cJSON_AddNumberToObject(root, "still_tol", g_stillness_tolerance);
    cJSON_AddNumberToObject(root, "angle_thr", g_angle_threshold_deg);
    cJSON_AddNumberToObject(root, "still_dur", g_stillness_duration_ms);

    cJSON_AddBoolToObject(root, "wifi_en", g_wifi_enabled);
    cJSON_AddBoolToObject(root, "en_sms", g_w_enable_sms);
    cJSON_AddBoolToObject(root, "en_call", g_w_enable_call);
    cJSON_AddBoolToObject(root, "en_sos", g_w_enable_sos);
    cJSON_AddNumberToObject(root, "act_orig", g_action_origin);
    cJSON_AddStringToObject(root, "wifi_ssid", g_wifi_ssid);
    cJSON_AddStringToObject(root, "wifi_pass", g_wifi_pass);
    cJSON_AddStringToObject(root, "sms_nums", g_w_sms_numbers);
    cJSON_AddStringToObject(root, "call_nums", g_w_call_numbers);
    cJSON_AddStringToObject(root, "sos_num", g_w_sos_number);
    cJSON_AddNumberToObject(root, "hr_sleep_interval", g_sleep_hr_interval_s);
    cJSON_AddNumberToObject(root, "hr_sleep_duration", g_sleep_hr_duration_s);

    char *json_str = cJSON_PrintUnformatted(root);
    *len = strlen(json_str);
    *data = (uint8_t *)strdup(json_str);

    free(json_str);
    cJSON_Delete(root);
}

extern "C" void spp_write_cb(uint8_t *data, uint16_t len)
{
    ESP_LOGI("SETTINGS", "Received write: %.*s", len, data);
    cJSON *root = cJSON_Parse((char *)data);
    if (root)
    {
        cJSON *item = cJSON_GetObjectItem(root, "screen_timeout");
        if (item)
            g_screen_timeout_ms = item->valueint;

        item = cJSON_GetObjectItem(root, "fall_low");
        if (item)
            g_fall_threshold_low = item->valuedouble;

        item = cJSON_GetObjectItem(root, "fall_high");
        if (item)
            g_fall_threshold_high = item->valuedouble;

        item = cJSON_GetObjectItem(root, "still_tol");
        if (item)
            g_stillness_tolerance = item->valuedouble;

        item = cJSON_GetObjectItem(root, "angle_thr");
        if (item)
            g_angle_threshold_deg = item->valuedouble;

        item = cJSON_GetObjectItem(root, "still_dur");
        if (item)
            g_stillness_duration_ms = item->valueint;

        item = cJSON_GetObjectItem(root, "wifi_en");
        if (!item) item = cJSON_GetObjectItem(root, "en_wifi");
        if (item) g_wifi_enabled = cJSON_IsTrue(item) || (item->type == cJSON_Number && item->valueint != 0);

        item = cJSON_GetObjectItem(root, "sms_en");
        if (!item) item = cJSON_GetObjectItem(root, "en_sms");
        if (item) g_w_enable_sms = cJSON_IsTrue(item) || (item->type == cJSON_Number && item->valueint != 0);

        item = cJSON_GetObjectItem(root, "call_en");
        if (!item) item = cJSON_GetObjectItem(root, "en_call");
        if (item) g_w_enable_call = cJSON_IsTrue(item) || (item->type == cJSON_Number && item->valueint != 0);

        item = cJSON_GetObjectItem(root, "sos_en");
        if (!item) item = cJSON_GetObjectItem(root, "en_sos");
        if (item) g_w_enable_sos = cJSON_IsTrue(item) || (item->type == cJSON_Number && item->valueint != 0);

        item = cJSON_GetObjectItem(root, "origin");
        if (!item) item = cJSON_GetObjectItem(root, "act_orig");
        if (item) g_action_origin = item->valueint;
        
        item = cJSON_GetObjectItem(root, "hr_sleep_interval");
        if (item) g_sleep_hr_interval_s = item->valueint;

        item = cJSON_GetObjectItem(root, "hr_sleep_duration");
        if (item) g_sleep_hr_duration_s = item->valueint;

        item = cJSON_GetObjectItem(root, "wifi_ssid");
        if (item && item->valuestring) strncpy(g_wifi_ssid, item->valuestring, sizeof(g_wifi_ssid) - 1);
        
        item = cJSON_GetObjectItem(root, "wifi_pass");
        if (item && item->valuestring) strncpy(g_wifi_pass, item->valuestring, sizeof(g_wifi_pass) - 1);
        
        item = cJSON_GetObjectItem(root, "sms_nums");
        if (item && item->valuestring) strncpy(g_w_sms_numbers, item->valuestring, sizeof(g_w_sms_numbers) - 1);
        
        item = cJSON_GetObjectItem(root, "call_nums");
        if (item && item->valuestring) strncpy(g_w_call_numbers, item->valuestring, sizeof(g_w_call_numbers) - 1);
        
        item = cJSON_GetObjectItem(root, "sos_num");
        if (item && item->valuestring) strncpy(g_w_sos_number, item->valuestring, sizeof(g_w_sos_number) - 1);
        
        item = cJSON_GetObjectItem(root, "sync_time");
        if (item && item->valuestring) {
            // Expected format: "YYYY-MM-DD HH:MM:SS"
            int year, month, day, hour, min, sec;
            if (sscanf(item->valuestring, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6) {
                struct tm t = {0};
                t.tm_year = year - 1900;
                t.tm_mon = month - 1;
                t.tm_mday = day;
                t.tm_hour = hour;
                t.tm_min = min;
                t.tm_sec = sec;
                t.tm_isdst = -1; // Let mktime determine DST
                
                time_t epoch = mktime(&t);
                if (epoch != (time_t)-1) {
                    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
                    settimeofday(&tv, NULL);
                    ESP_LOGI("TIME", "System time synced to: %s", item->valuestring);
                    
                    // Update UI immediately (optional, read_sensor_data does it at 1Hz)
                    if (example_lvgl_lock(-1)) {
                        char clock_str[10];
                        snprintf(clock_str, sizeof(clock_str), "%02d:%02d", hour, min);
                        if (ui_LabelTime) lv_label_set_text(ui_LabelTime, clock_str);
                        example_lvgl_unlock();
                    }
                } else {
                    ESP_LOGE("TIME", "Failed to convert time to epoch: %s", item->valuestring);
                }
            } else {
                ESP_LOGE("TIME", "Invalid time format received: %s", item->valuestring);
            }
        }

        save_settings();

        // Apply WiFi changes immediately if enabled or credentials changed
        if (g_wifi_enabled) {
            wifi_reconnect_now();
        } else if (s_wifi_init_done) {
            esp_wifi_stop();
        }

        cJSON_Delete(root);
    }
}

// --- WiFi & Firestore REST Logic ---
void wifi_init_sta(void); 
void wifi_upload_task(void *pvParameters); 

// Helper: Trim spaces from start/end of string
void str_trim(char *s) {
    char *p = s;
    int l = strlen(p);
    while (l > 0 && isspace((unsigned char)p[l - 1])) p[--l] = 0;
    while (*p && isspace((unsigned char)*p)) p++, l--;
    memmove(s, p, l + 1);
}

void scan_wifi_and_print(void) {
    ESP_LOGI("WIFI", "Starting manual scan to see what's available...");
    
    // We must ensure WiFi is in STA mode and started for scanning
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_NULL) esp_wifi_set_mode(WIFI_MODE_STA);
    
    wifi_scan_config_t scan_config = {};
    scan_config.show_hidden = true;
    
    esp_err_t res = esp_wifi_scan_start(&scan_config, true);
    if (res != ESP_OK) {
        ESP_LOGE("WIFI", "Scan failed: %s", esp_err_to_name(res));
        return;
    }
    
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    wifi_ap_record_t *ap_info = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
    if (ap_info) {
        esp_wifi_scan_get_ap_records(&ap_count, ap_info);
        ESP_LOGI("WIFI", "---------- SCAN RESULTS (%d found) ----------", ap_count);
        for (int i = 0; i < ap_count; i++) {
            ESP_LOGI("WIFI", "SSID: [%s] | RSSI: %d | CH: %d", 
                     (char*)ap_info[i].ssid, ap_info[i].rssi, ap_info[i].primary);
        }
        ESP_LOGI("WIFI", "---------------------------------------------");
        free(ap_info);
    }
}

extern "C" void wifi_reconnect_now() {
    ESP_LOGI("WIFI", "Manual reconnect requested via settings...");
    if (!s_wifi_init_done) {
        wifi_init_sta();
        str_trim(g_wifi_ssid);
        str_trim(g_wifi_pass);
        
        wifi_config_t wifi_config = {};
        strncpy((char*)wifi_config.sta.ssid, g_wifi_ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, g_wifi_pass, sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_start();
        esp_wifi_connect();
        ESP_LOGI("WIFI", "WiFi credentials updated and reconnecting to [%s]...", g_wifi_ssid);
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI("WIFI", "WiFi station started, connecting to %s...", g_wifi_ssid);
        esp_wifi_connect();
        if (lvgl_mux && example_lvgl_lock(10)) {
            if (ui_LabelWiFi_Icon) lv_obj_set_style_text_color(ui_LabelWiFi_Icon, lv_color_hex(0xFFA500), 0); // Orange
            example_lvgl_unlock();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI("WIFI", "Connected to AP successfully!");
        if (lvgl_mux && example_lvgl_lock(10)) {
            if (ui_LabelWiFi_Icon) lv_obj_set_style_text_color(ui_LabelWiFi_Icon, lv_color_hex(0xFFFF00), 0); // Yellow (Wait for IP)
            example_lvgl_unlock();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_connected = false;
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW("WIFI", "Disconnected from %s, reason: %d", g_wifi_ssid, event->reason);
        
        if (lvgl_mux && example_lvgl_lock(10)) {
            if (ui_LabelWiFi_Icon) lv_obj_set_style_text_color(ui_LabelWiFi_Icon, lv_color_hex(0xFF0000), 0); // Red
            example_lvgl_unlock();
        }

        if (g_wifi_enabled && event->reason != WIFI_REASON_ASSOC_LEAVE) {
            ESP_LOGI("WIFI", "Attempting reconnection in 2 seconds...");
            if (event->reason == 201) {
                scan_wifi_and_print();
            }
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WIFI", "Successfully got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_connected = true;
        if (lvgl_mux && example_lvgl_lock(10)) {
            if (ui_LabelWiFi_Icon) lv_obj_set_style_text_color(ui_LabelWiFi_Icon, lv_color_hex(0x00FF00), 0); // Bright Green
            example_lvgl_unlock();
        }
    }
}

void wifi_init_sta(void) {
    if (strlen(g_wifi_ssid) == 0) {
        ESP_LOGW("WIFI", "SSID empty, skipping WiFi init");
        return;
    }

    if (!s_wifi_init_done) {
        ESP_ERROR_CHECK(esp_netif_init());
        esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
        esp_netif_set_hostname(sta_netif, "LifeLink-Watch");

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        cfg.static_rx_buf_num = 4;
        cfg.dynamic_rx_buf_num = 16;
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_got_ip));
        s_wifi_init_done = true;
        xTaskCreate(wifi_upload_task, "wifi_upload_task", 8192, NULL, 5, NULL);
    }

    str_trim(g_wifi_ssid);
    str_trim(g_wifi_pass);

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, g_wifi_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, g_wifi_pass, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN; // Scan all channels for better discovery
    
    // WPA3 Support (SAE) if available in this IDF version
    #if CONFIG_ESP_WIFI_PW_ID
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    #endif

    ESP_LOGI("WIFI", "Initializing WiFi STA with SSID: %s", g_wifi_ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WIFI", "wifi_init_sta sequence completed.");
}

esp_err_t send_to_api(const char* post_data) {
    if (!s_wifi_connected) return ESP_FAIL;

    esp_http_client_config_t config = {};
    config.url = MARIADB_API_URL;
    config.method = HTTP_METHOD_POST;
    // Za lokalni server obično ne treba crt_bundle, ali ostavljamo za HTTPS podršku
    config.crt_bundle_attach = esp_crt_bundle_attach; 
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("API", "HTTP POST MariaDB Status = %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE("API", "HTTP POST to MariaDB failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}



void wifi_upload_task(void *pvParameters) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(g_mac_str, sizeof(g_mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    while (1) {
        if (s_wifi_connected && g_wifi_enabled) {
            // 1. Pripremi podatak za MariaDB PHP API (Ravan JSON format)
            cJSON *root_api = cJSON_CreateObject();
            cJSON_AddStringToObject(root_api, "device_id", g_mac_str);
            cJSON_AddStringToObject(root_api, "name", "LifeLink Watch");
            cJSON_AddNumberToObject(root_api, "pulse", (int)heartRate);
            cJSON_AddNumberToObject(root_api, "spo2", (int)spo2);
            cJSON_AddNumberToObject(root_api, "battery", (int)g_batt_pct_snapshot);
            cJSON_AddNumberToObject(root_api, "gForce", (double)g_total_snapshot);
            cJSON_AddStringToObject(root_api, "source", "wifi");
            if (g_latitude != 0.0f || g_longitude != 0.0f) {
                cJSON_AddNumberToObject(root_api, "lat", (double)g_latitude);
                cJSON_AddNumberToObject(root_api, "lon", (double)g_longitude);
            }
            char *post_data_api = cJSON_PrintUnformatted(root_api);
            send_to_api(post_data_api);
            ESP_LOGI("WIFI", "Autonomni upload na MariaDB zavrsen (izvor: wifi)");
            free(post_data_api);
            cJSON_Delete(root_api);
        }
        vTaskDelay(pdMS_TO_TICKS(30000)); // Svakih 30 sekundi
    }
}

extern "C" void toggle_wifi(bool enable) {
    g_wifi_enabled = enable;
    if (enable) {
        ESP_LOGI("WIFI", "WiFi Enabled via UI (SSID: %s)", g_wifi_ssid);
        if (!s_wifi_init_done) {
            wifi_init_sta();
        } else if (!s_wifi_connected) {
            esp_wifi_connect();
        }
    } else {
        ESP_LOGI("WIFI", "WiFi Disabled via UI");
        esp_wifi_disconnect();
    }
}

extern "C" void trigger_sos_alarm(void) {
    ESP_LOGE("ALARM", "!!! TRIGGERING SOS ALARM !!!");
    
    // 1. Notify Mobile App via BLE
    char ble_msg[64];
    snprintf(ble_msg, sizeof(ble_msg), "SOS_ALARM Lat:%.5f Lon:%.5f", g_latitude, g_longitude);
    ble_spp_server_send_data((uint8_t *)ble_msg, strlen(ble_msg));

    // 2. Send Fall Event to MariaDB via WiFi if available
    if (s_wifi_connected && g_wifi_enabled) {
        cJSON *root_api = cJSON_CreateObject();
        cJSON_AddStringToObject(root_api, "device_id", g_mac_str);
        cJSON_AddBoolToObject(root_api, "is_fall", true);
        cJSON_AddNumberToObject(root_api, "gForce", (double)g_total_snapshot);
        if (g_latitude != 0.0f || g_longitude != 0.0f) {
            cJSON_AddNumberToObject(root_api, "lat", (double)g_latitude);
            cJSON_AddNumberToObject(root_api, "lon", (double)g_longitude);
        }
        char *post_data_api = cJSON_PrintUnformatted(root_api);
        send_to_api(post_data_api);
        free(post_data_api);
        cJSON_Delete(root_api);
    }

    // 3. Trigger GSM SMS alert if enabled
    if (g_w_enable_sos && strlen(g_w_sos_number) > 0) {
        char sms_msg[160];
        snprintf(sms_msg, sizeof(sms_msg),
                 "LifeLink UPOZORENJE: PAD DETEKTOVAN! g=%.2f. Lokacija: https://maps.google.com/?q=%.5f,%.5f",
                 g_total_snapshot, g_latitude, g_longitude);
        ESP_LOGI("ALARM", "Slanje SMS na SOS broj: %s", g_w_sos_number);
        gsm_send_sms_async(g_w_sos_number, sms_msg);
    }
}

extern "C" void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Set Timezone to Serbia (CET/CEST)
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0", 1);
    tzset();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize I2C first
    ESP_ERROR_CHECK(i2c_init());

    load_settings(); // Load WiFi etc.

    if (g_wifi_enabled) {
        wifi_init_sta();
    }


    // === WAVESHARE TCA9554 POWER ENABLE ===
    // This chip at 0x20 controls the AMOLED Power Enable and Reset pins!
    // P2 is LCD_VCC_EN (Needs to be HIGH)
    // P0/P1 are Touch Reset/INT (Needs to be HIGH)
    // P7 is GPS_RST (Needs to be HIGH for operational)
    g_i2c_mux = xSemaphoreCreateMutex();
    
    uint8_t tca_cfg[] = {0x03, 0x00}; // Reg 0x03 = Configuration (0=All Output)
    uint8_t tca_val[] = {0x01, 0xDF}; // Reg 0x01 = Output (0xDF: P5 LOW for I2C, others HIGH)
    
    if (xSemaphoreTake(g_i2c_mux, pdMS_TO_TICKS(1000))) {
        i2c_master_write_to_device(I2C_MASTER_NUM, TCA9554_ADDR, tca_cfg, 2, pdMS_TO_TICKS(100));
        i2c_master_write_to_device(I2C_MASTER_NUM, TCA9554_ADDR, tca_val, 2, pdMS_TO_TICKS(100));
        xSemaphoreGive(g_i2c_mux);
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for VDD to stabilize

#if EXAMPLE_USE_TOUCH
    // UN-JAM I2C BUS
    gpio_set_direction((gpio_num_t)Touch_RST, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)Touch_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level((gpio_num_t)Touch_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Re-enable AXP init
    if (axp_init(I2C_MASTER_NUM) == ESP_OK)
    {
        ESP_LOGI("PMU", "AXP2101 Initialized - Enabling power rails...");
        axp_enable_power(); 
    }
#endif

    // Initialize BLE first to ensure resources are available
    ble_spp_server_init();

    // Load settings from NVS AFTER flash init in ble_spp_server_init
    load_settings();
    ble_spp_server_register_callbacks(spp_write_cb, spp_read_cb);

    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif

    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = GPIO_NUM_38;
    buscfg.data0_io_num = GPIO_NUM_4;
    buscfg.data1_io_num = GPIO_NUM_5;
    buscfg.data2_io_num = GPIO_NUM_6;
    buscfg.data3_io_num = GPIO_NUM_7;
    buscfg.max_transfer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t);
    buscfg.flags = SPICOMMON_BUSFLAG_QUAD;
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    // esp_lcd_panel_io_handle_t io_handle = NULL; // Removed local to use global
    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .dc_gpio_num = -1,
        .spi_mode = 0,
        .pclk_hz = 20 * 1000 * 1000, // Reduced to 20MHz for stability
        .trans_queue_depth = 30,    // Stable value for DMA
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
        .lcd_cmd_bits = 32,
        .lcd_param_bits = 8,
        .flags = {
            .quad_mode = true,
        },
    };
    sh8601_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // esp_lcd_panel_handle_t panel_handle = NULL; // Removed local declaration
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_LOGI(TAG, "Install SH8601 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight

#if EXAMPLE_USE_TOUCH

    setup_sensor();
    setup_max30102();
#endif

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);
    reset_screen_timer(); // Initialize timer
#endif

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    // Use MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL for standard SPI DMA support on S3
    // Note: buffers reduced in height (to 1/20) to save internal RAM for WiFi/BLE
    lv_color_t *buf1 = static_cast<lv_color_t *>(heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    assert(buf1);
    lv_color_t *buf2 = static_cast<lv_color_t *>(heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.rounder_cb = example_lvgl_rounder_cb;
    disp_drv.drv_update_cb = example_lvgl_update_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

#if EXAMPLE_USE_TOUCH
    static lv_indev_drv_t indev_drv; // Input device driver (Touch)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = example_lvgl_touch_cb;
    indev_drv.user_data = &touch;
    lv_indev_drv_register(&indev_drv);
#endif

    lvgl_mux = xSemaphoreCreateMutex();
    assert(lvgl_mux);

    // --- Initialize GSM AFTER LVGL Mutex is created ---
#if EXAMPLE_USE_TOUCH
    // Spawn a task for GSM Init so it runs continuously in the background
    xTaskCreate([](void *arg)
                {
        while(1) 
        {
            if (gsm_a6_init() == ESP_OK)
            {
                ESP_LOGI("MAIN", "GSM A6 Module Initialized Successfully.");
                if (example_lvgl_lock(-1))
                {
                    if (ui_LabelGSM) {
                        lv_label_set_text(ui_LabelGSM, "GSM: OK!");
                        lv_obj_set_style_text_color(ui_LabelGSM, lv_color_hex(0x00FF00), LV_PART_MAIN);
                    }
                    if (ui_LabelGSM_Text) {
                        lv_obj_set_style_text_color(ui_LabelGSM_Text, lv_color_hex(0x00FF00), LV_PART_MAIN);
                    }
                    if (ui_LabelGSM_Icon) {
                        lv_obj_set_style_text_color(ui_LabelGSM_Icon, lv_color_hex(0x00FF00), LV_PART_MAIN);
                    }
                    example_lvgl_unlock();
                }
                xTaskCreate(gsm_status_task, "gsm_status_task", 8192, NULL, 5, NULL);
                break; // Exit loop on connection success
            }
            else
            {
                ESP_LOGE("MAIN", "GSM Init Failed. Retrying in 3 seconds...");
                if (example_lvgl_lock(-1))
                {
                    if (ui_LabelGSM) {
                        lv_label_set_text(ui_LabelGSM, "GSM: RETRY...");
                        lv_obj_set_style_text_color(ui_LabelGSM, lv_color_hex(0xFF0000), LV_PART_MAIN);
                    }
                    if (ui_LabelGSM_Text) {
                        lv_obj_set_style_text_color(ui_LabelGSM_Text, lv_color_hex(0xFF0000), LV_PART_MAIN);
                    }
                    if (ui_LabelGSM_Icon) {
                        lv_obj_set_style_text_color(ui_LabelGSM_Icon, lv_color_hex(0xFF0000), LV_PART_MAIN);
                    }
                    example_lvgl_unlock();
                }
                vTaskDelay(pdMS_TO_TICKS(3000));
            }
        }
        vTaskDelete(NULL); }, "gsm_init_task", 8192, NULL, 5, NULL);

#endif

    xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "Display LVGL demos");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (example_lvgl_lock(-1))
    {

        // lv_demo_widgets();      /* A widgets example */
        // lv_demo_music(); /* A modern, smartphone-like music player demo. */
        // lv_demo_stress();       /* A stress test for LVGL. */
        // lv_demo_benchmark();    /* A demo to measure the performance of LVGL or to compare different settings. */
        ui_init(); // LifeLink UI initialization

        // Initial values
        if (ui_LabelTime) lv_label_set_text(ui_LabelTime, "--:--");
        if (ui_LabelInfo) lv_label_set_text(ui_LabelInfo, "LifeLink");
        if (ui_LabelBatt) lv_label_set_text(ui_LabelBatt, "100%");

        // Setup Sensor & Tasks immediately for responsive UI
        setup_accel();
        xTaskCreate(read_sensor_data, "sensor_read_task", 12288, NULL, 3, NULL);
        xTaskCreate(gps_task, "gps_task", 4096, NULL, 2, NULL); 

        // --- Power Management Task (or just add to loop if lightweight) ---
        // For simplicity, we can do it in read_sensor_data or a timer.
        // Let's rely on read_sensor_data for battery updates and main loop/timer for timeout.

        // NOW it's safe to turn the brightness back up to MAX!
        // We do it after LVGL has already pushed the first black/UI frame.
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
        uint8_t brightness = 0xFF; // Max brightness
        esp_lcd_panel_io_tx_param(io_handle, 0x51, &brightness, 1);
        ESP_LOGI(TAG, "AMOLED Display turned ON safely AFTER UI rendered.");

        // Release the mutex
        example_lvgl_unlock();
    }
}

void read_sensor_data(void *arg)
{
    char x_str[20], y_str[20], z_str[20], g_str[20];
    char info_str[128] = "Monitoring...";
    float g_total = 1.0f; 
    static int g_batt_pct = 0; 
    bool validHeartRate = false;
    bool validSPO2 = false;
    
    extern float g_total_snapshot;
    extern int g_batt_pct_snapshot;

    int samplesCollected = 0;
    static bool s_max30102_sensing = false;
    uint64_t last_batt_check = 0;
    uint64_t last_button_check = 0;
    uint64_t last_sensor_log = 0;
    uint64_t last_ui_report = 0;
    static int skipCount = 0;

    while (1)
    {
        uint64_t now_ms_abs = esp_timer_get_time() / 1000;

        // 1. MAX30102 logic (Screen-based or Periodic)
        bool on_sensor_screen = false;
        if (example_lvgl_lock(-1)) {
            if (ui_Screen1 && lv_scr_act() == ui_Screen1) on_sensor_screen = true;
            example_lvgl_unlock();
        }

        bool background_hr_due = false;
        if (!screen_is_on) {
            uint32_t current_cycle_s = (now_ms_abs / 1000) % g_sleep_hr_interval_s;
            if (current_cycle_s < (uint32_t)g_sleep_hr_duration_s) background_hr_due = true;
        }

        bool sensor_active_needed = (screen_is_on && on_sensor_screen) || (fallState != IDLE) || background_hr_due;

        if (sensor_active_needed)
        {
            if (!s_max30102_sensing) {
                ESP_LOGI("MAX30102", "Waking up for %s sensing...", 
                         (fallState != IDLE) ? "EMERGENCY" : (background_hr_due ? "BACKGROUND" : "ACTIVE"));
                max30102.wakeUp();
                s_max30102_sensing = true;
                vTaskDelay(pdMS_TO_TICKS(10));
            }

            if (xSemaphoreTake(g_i2c_mux, pdMS_TO_TICKS(100))) {
                max30102.check();
                while (max30102.available()) {
                    uint32_t r = max30102.getRed();
                    uint32_t ir = max30102.getIR();
                    max30102.nextSample();
                    
                    if (skipCount % 4 == 0 && samplesCollected < TEST_BUFFER_LENGTH) {
                        redBuffer[samplesCollected] = r;
                        irBuffer[samplesCollected] = ir;
                        samplesCollected++;
                    }
                    skipCount++;
                }
                xSemaphoreGive(g_i2c_mux);
            }

            if (samplesCollected == TEST_BUFFER_LENGTH)
            {
                for (int i = 0; i < TEST_BUFFER_LENGTH; i++) {
                    redBufferFloat[i] = (float)redBuffer[i];
                    irBufferFloat[i] = (float)irBuffer[i];
                }
                fft_process(redBufferFloat, irBufferFloat, TEST_BUFFER_LENGTH, 100, &fft_hr, &fft_spo2);
                heartRate = (int32_t)fft_hr;
                spo2 = (int32_t)fft_spo2;
                validHeartRate = (heartRate >= 45 && heartRate <= 220);
                validSPO2 = (spo2 >= 50 && spo2 <= 100);
                ESP_LOGD("SENSORS", "HR valid: %d, SPO2 valid: %d", validHeartRate, validSPO2);

                // Shift buffer (sliding window)
                int shift = 50;
                for (int i = shift; i < TEST_BUFFER_LENGTH; i++) {
                    redBuffer [i - shift] = redBuffer[i];
                    irBuffer  [i - shift] = irBuffer [i];
                }
                samplesCollected = TEST_BUFFER_LENGTH - shift;
            }

            if (now_ms_abs - last_sensor_log > 3000) {
                last_sensor_log = now_ms_abs;
                ESP_LOGI("MAX30102", "[%s] HR:%d SpO2:%d%% (S:%d)", 
                         (background_hr_due ? "BG" : (fallState != IDLE ? "EMG" : "ACT")),
                         (int)heartRate, (int)spo2, samplesCollected);
            }
        }
        else if (s_max30102_sensing)
        {
            ESP_LOGI("MAX30102", "Sensing window over. Shutting down.");
            max30102.shutDown();
            s_max30102_sensing = false;
            samplesCollected = 0; 
        }

        // 2. QMI8658
        bool qmi_ready = false;
        if (xSemaphoreTake(g_i2c_mux, pdMS_TO_TICKS(100))) {
            if (qmi.getDataReady()) {
                if (qmi.getAccelerometer(acc.x, acc.y, acc.z) && qmi.getGyroscope(gyr.x, gyr.y, gyr.z)) {
                    g_total = sqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);
                    g_total_snapshot = g_total;
                    qmi_ready = true;
                }
            }
            xSemaphoreGive(g_i2c_mux);
        }

        // 3. Power
        if (now_ms_abs - last_batt_check > 10000) {
            last_batt_check = now_ms_abs;
            int pct = axp_get_batt_percent();
            if (pct >= 0) {
                g_batt_pct = pct;
                g_batt_pct_snapshot = g_batt_pct;
            }
        }

        // Button Polling
        if (now_ms_abs - last_button_check > 200) {
            last_button_check = now_ms_abs;
            uint8_t irq_status;
            if (axp_get_irq_status(0x41, &irq_status) == ESP_OK && irq_status != 0) {
                if (irq_status & 0x01) { // Short press
                    ESP_LOGI("PMU", "Short press detected - Toggle Screen");
                    if (screen_is_on) {
                         // Force sleep by setting an old touch time
                         last_touch_time = now_ms_abs - (g_screen_timeout_ms + 1000);
                    } else {
                         reset_screen_timer(); // This will set screen_is_on = true and reset timers
                    }
                }
                if (irq_status & 0x02) { // Long press
                    ESP_LOGI("PMU", "Long press detected - SHUTTING DOWN...");
                    axp_power_off();
                }
                axp_clear_irq_status(0x41, irq_status); // Clear those bits
            }
        }

        // 4. UI & Fall Logic
        if (example_lvgl_lock(-1))
        {
            bool update_now = (now_ms_abs - last_ui_report > 200);
            if (update_now) {
                last_ui_report = now_ms_abs;
                if (ui_LabelBatt) {
                    char batt_str[16];
                    snprintf(batt_str, sizeof(batt_str), "%d%%", g_batt_pct);
                    lv_label_set_text(ui_LabelBatt, batt_str);
                    if (g_batt_pct < 20) lv_obj_set_style_text_color(ui_LabelBatt, lv_color_hex(0xFF0000), LV_PART_MAIN);
                    else lv_obj_set_style_text_color(ui_LabelBatt, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                }
                if (on_sensor_screen) {
                    char hr_display[16], spo_display[16];
                    snprintf(hr_display, sizeof(hr_display), "%d", (int)heartRate);
                    snprintf(spo_display, sizeof(spo_display), "%d%%", (int)spo2);
                    if (ui_LabelPuls) lv_label_set_text(ui_LabelPuls, hr_display);
                    if (ui_LabelSpo) lv_label_set_text(ui_LabelSpo, spo_display);
                }
                time_t now_time = time(NULL);
                struct tm *timeinfo = localtime(&now_time);
                char clock_str[10];
                snprintf(clock_str, sizeof(clock_str), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
                if (ui_LabelTime) lv_label_set_text(ui_LabelTime, clock_str);
                if (ui_AODTime) lv_label_set_text(ui_AODTime, clock_str);

                bool debug_active = (ui_BtnDebug != NULL) && lv_obj_has_state(ui_BtnDebug, LV_STATE_CHECKED);
                if (debug_active) {
                    snprintf(g_str, sizeof(g_str), "%.2f", g_total);
                    if (ui_LabelG) lv_label_set_text(ui_LabelG, g_str);
                    snprintf(x_str, sizeof(x_str), "%.2f", acc.x);
                    snprintf(y_str, sizeof(y_str), "%.2f", acc.y);
                    snprintf(z_str, sizeof(z_str), "%.2f", acc.z);
                    if (ui_LabelX) lv_label_set_text(ui_LabelX, x_str);
                    if (ui_LabelY) lv_label_set_text(ui_LabelY, y_str);
                    if (ui_LabelZ) lv_label_set_text(ui_LabelZ, z_str);
                }
            }

            switch (fallState)
            {
                case IDLE:
                    if (qmi_ready && g_total < FALL_THRESHOLD_LOW) {
                        ref_ax = acc.x; ref_ay = acc.y; ref_az = acc.z;
                        potentialFallCount++;
                        fallState = FREE_FALL;
                        stateTimer = (unsigned long)now_ms_abs;
                        snprintf(info_str, sizeof(info_str), "Pad? (Pot:%d)", potentialFallCount);
                        lv_label_set_text(ui_LabelInfo, info_str);
                    } else if (qmi_ready && fabsf(g_total - 1.0f) < 0.1f) {
                        ref_ax = acc.x; ref_ay = acc.y; ref_az = acc.z;
                    }
                    break;
                case FREE_FALL:
                    if (g_total > FALL_THRESHOLD_HIGH) {
                        fallState = IMPACT_DETECTED;
                        stateTimer = (unsigned long)now_ms_abs;
                        snprintf(info_str, sizeof(info_str), "IMPACT! (G:%.1f)", g_total);
                        lv_label_set_text(ui_LabelInfo, info_str);
                    } else if (now_ms_abs - stateTimer > 1000) fallState = IDLE;
                    break;
                case IMPACT_DETECTED:
                    if (now_ms_abs - stateTimer > 500) {
                        fallState = WAITING_FOR_STILLNESS;
                        stateTimer = (unsigned long)now_ms_abs;
                    }
                    break;
                case WAITING_FOR_STILLNESS:
                    if (g_total > (1.0f + STILLNESS_TOLERANCE) || g_total < (1.0f - STILLNESS_TOLERANCE)) {
                        stateTimer = (unsigned long)now_ms_abs; 
                    } else if (now_ms_abs - stateTimer > (uint64_t)STILLNESS_DURATION_MS) {
                        float dot = (acc.x * ref_ax + acc.y * ref_ay + acc.z * ref_az);
                        float n1 = sqrt(acc.x*acc.x + acc.y*acc.y + acc.z*acc.z);
                        float n2 = sqrt(ref_ax*ref_ax + ref_ay*ref_ay + ref_az*ref_az);
                        float cos_theta = dot / (n1 * n2);
                        if (cos_theta > 1.0f) cos_theta = 1.0f; else if (cos_theta < -1.0f) cos_theta = -1.0f;
                        float angle_deg = acosf(cos_theta) * 180.0f / 3.14159f;

                        if (angle_deg > ANGLE_THRESHOLD_DEG) {
                            fallCount++;
                            start_fall_countdown_ui(false);
                            fallState = IDLE;
                        } else {
                            fallState = IDLE;
                            snprintf(info_str, sizeof(info_str), "Smetnja (%.0f deg)", angle_deg);
                            lv_label_set_text(ui_LabelInfo, info_str);
                        }
                    } else if (now_ms_abs - stateTimer > 5000) fallState = IDLE;
                    break;
            }

            uint64_t current_now_ms = esp_timer_get_time() / 1000;
            if (screen_is_on && (current_now_ms - last_touch_time > (uint64_t)g_screen_timeout_ms))
            {
                screen_is_on = false;
                if (g_is_aod_mode) {
                    if (ui_AODScreen == NULL) create_aod_screen();
                    lv_scr_load_anim(ui_AODScreen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
                    uint8_t br = 0x20; esp_lcd_panel_io_tx_param(io_handle, 0x51, &br, 1);
                } else {
                    if (panel_handle) esp_lcd_panel_disp_on_off(panel_handle, false);
                }
            }

            static uint64_t last_ble_beat = 0;
            if (now_ms_abs - last_ble_beat > 1000) {
                last_ble_beat = now_ms_abs;
                char beat_msg[128];
                snprintf(beat_msg, sizeof(beat_msg), "STATUS G:%.2f P:%d S:%d B:%d Lat:%.5f Lon:%.5f",
                         g_total, (int)heartRate, (int)spo2, g_batt_pct, g_latitude, g_longitude);
                ble_spp_server_send_data((uint8_t *)beat_msg, strlen(beat_msg));
            }
            example_lvgl_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

extern "C" void update_ble_connection_status(bool connected)
{
    if (example_lvgl_lock(-1))
    {
        if (connected)
        {
            lv_obj_set_style_text_color(ui_LabelBLT, lv_color_hex(0x00FF00), LV_PART_MAIN);
            lv_obj_set_style_text_color(ui_LabelBLE_Icon, lv_color_hex(0x00FF00), LV_PART_MAIN); // BLE Icon
        }
        else
        {
            lv_obj_set_style_text_color(ui_LabelBLT, lv_color_hex(0xFF0000), LV_PART_MAIN); // Red for disconnect
            lv_obj_set_style_text_color(ui_LabelBLE_Icon, lv_color_hex(0xFF0000), LV_PART_MAIN); // BLE Icon
        }
        example_lvgl_unlock();
    }
}

void gps_task(void *arg)
{
    ESP_LOGI("GPS", "Starting GPS Task");

    // Ensure GPS Label is Red initially
    if (example_lvgl_lock(-1))
    {
        lv_obj_set_style_text_color(ui_LabelGPS, lv_color_hex(0xFF0000), LV_PART_MAIN);
        lv_obj_set_style_text_color(ui_LabelGPS_Icon, lv_color_hex(0xFF0000), LV_PART_MAIN); // GPS Icon
        example_lvgl_unlock();
    }

    lc76g_init(I2C_NUM_0); 
    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t *buffer = (uint8_t *)heap_caps_malloc(10240, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); 
    if (!buffer) {
        ESP_LOGE("GPS", "Failed to allocate memory for GPS buffer in internal RAM!");
        vTaskDelete(NULL);
    }
    size_t read_len = 0;

    // Line buffer for assembling sentences
    static char line_buf[128];
    static int line_idx = 0;

    // The previous `if (!buffer)` block was redundant and removed.

    while (1)
    {
        // Read up to 2560 bytes (our allocated buffer size)
        esp_err_t ret = lc76g_read_data(buffer, 10240, &read_len);
        if (ret == ESP_OK && read_len > 0)
        {
            static uint32_t total_gps_bytes = 0;
            total_gps_bytes += read_len;
            if (total_gps_bytes % 500 < 50) { // Log occasionally
                ESP_LOGI("GPS_DIAG", "Total bytes received via I2C: %lu. First char: '%c'", total_gps_bytes, buffer[0]);
            }
            // Process byte by byte to find newlines
            for (int i = 0; i < read_len; i++)
            {
                char c = (char)buffer[i];
                if (c == '\r' || c == '\n')
                {
                    if (line_idx > 0)
                    {
                        line_buf[line_idx] = 0; // Null terminate
                        parse_nmea(line_buf);   // Parse the complete line
                        line_idx = 0;           // Reset
                    }
                }
                else
                {
                    if (line_idx < sizeof(line_buf) - 1)
                    {
                        line_buf[line_idx++] = c;
                    }
                    else
                    {
                        // Buffer overflow, reset
                        line_idx = 0;
                    }
                }
            }
            // Raw logging (optional, maybe too noisy now)
            // buffer[read_len] = 0;
            // ESP_LOGI("GPS_NMEA", "%s", (char *)buffer);
        }
        else if (ret == ESP_OK && read_len == 0)
        {
            static uint64_t last_silent_log = 0;
            if (esp_timer_get_time() / 1000 - last_silent_log > 5000)
            {
                last_silent_log = esp_timer_get_time() / 1000;
                ESP_LOGW("GPS_DIAG", "GPS I2C is SILENT (No data in 5s) - Check TCA9554 and Address 0x54");
            }
        }
        else if (ret != ESP_OK)
        {
            static uint32_t last_gps_err = 0;
            if (esp_timer_get_time() / 1000 - last_gps_err > 5000)
            {
                ESP_LOGD("GPS", "I2C Comm Fail or Timeout: %s", esp_err_to_name(ret));
                last_gps_err = esp_timer_get_time() / 1000;
            }
        }

        // Poll every 0.1s (GPS usually updates at 1Hz or 10Hz)
        vTaskDelay(pdMS_TO_TICKS(100));

        // --- DEMO MODE FALLBACK (Disabled for real testing) ---
        /*
        if (g_latitude == 0.0f && g_longitude == 0.0f)
        {
            g_latitude = 44.7866f;
            g_longitude = 20.4489f;
        }
        */
    }

    free(buffer);
}

void gsm_status_task(void *arg)
{
    ESP_LOGI("GSM_TASK", "Starting GSM Status Task");
    int consecutive_failures = 0;
    const int MAX_FAILURES_BEFORE_REINIT = 3;
    static bool s_time_synced = false;
    static uint32_t s_last_sync_tick = 0;

    while (1)
    {
        // Periodic check (every 10 seconds)
        esp_err_t ret = gsm_check_network();

        if (ret == ESP_OK)
        {
            consecutive_failures = 0; // Reset on success

            // Sync time on first registration or every hour (3600000 ms)
            uint32_t now_tick = pdTICKS_TO_MS(xTaskGetTickCount());
            if (!s_time_synced || (now_tick - s_last_sync_tick > 3600000))
            {
                struct tm net_time;
                if (gsm_get_network_time(&net_time) == ESP_OK)
                {
                    struct timeval tv;
                    tv.tv_sec = mktime(&net_time);
                    tv.tv_usec = 0;
                    settimeofday(&tv, NULL);
                    s_time_synced = true;
                    s_last_sync_tick = now_tick;
                    ESP_LOGI("GSM_TASK", "System time synchronized with GSM network");
                }
            }
        }
        else
        {
            consecutive_failures++;
            ESP_LOGW("GSM_TASK", "Network check failed (%d/%d)", consecutive_failures, MAX_FAILURES_BEFORE_REINIT);
        }

        // Auto-recovery: if too many consecutive failures, power cycle and re-init GSM
        if (consecutive_failures >= MAX_FAILURES_BEFORE_REINIT)
        {
            ESP_LOGE("GSM_TASK", "=== GSM RECOVERY: %d consecutive failures, re-initializing module ===", consecutive_failures);
            consecutive_failures = 0;

            if (example_lvgl_lock(-1))
            {
                if (ui_LabelGSM)
                    lv_label_set_text(ui_LabelGSM, "GSM: Restarting...");
                if (ui_LabelGSM_Icon)
                {
                    lv_obj_set_style_text_color(ui_LabelGSM_Icon, lv_color_hex(0xFFA500), LV_PART_MAIN);
                    lv_obj_set_style_text_color(ui_LabelGSM_Text, lv_color_hex(0xFFA500), LV_PART_MAIN);
                }
                example_lvgl_unlock();
            }

            // Full re-initialization (power cycle + AT sync + network registration)
            if (gsm_a6_init() == ESP_OK)
            {
                ESP_LOGI("GSM_TASK", "GSM Recovery successful!");
                if (example_lvgl_lock(-1))
                {
                    if (ui_LabelGSM)
                    {
                        lv_label_set_text(ui_LabelGSM, "GSM: Recovered!");
                        lv_obj_set_style_text_color(ui_LabelGSM, lv_color_hex(0x00FF00), LV_PART_MAIN);
                    }
                    if (ui_LabelGSM_Icon)
                    {
                        lv_obj_set_style_text_color(ui_LabelGSM_Icon, lv_color_hex(0x00FF00), LV_PART_MAIN);
                        lv_obj_set_style_text_color(ui_LabelGSM_Text, lv_color_hex(0x00FF00), LV_PART_MAIN);
                    }
                    example_lvgl_unlock();
                }
            }
            else
            {
                ESP_LOGE("GSM_TASK", "GSM Recovery FAILED! Will retry next cycle.");
            }
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        if (example_lvgl_lock(-1))
        {
            if (ui_LabelGSM)
            {
                if (ret == ESP_OK)
                {
                    lv_label_set_text(ui_LabelGSM, "GSM: Registered");
                    lv_obj_set_style_text_color(ui_LabelGSM, lv_color_hex(0x00FF00), LV_PART_MAIN);

                    if (ui_LabelGSM_Icon)
                    {
                        lv_obj_set_style_text_color(ui_LabelGSM_Icon, lv_color_hex(0x00FF00), LV_PART_MAIN);
                        lv_obj_set_style_text_color(ui_LabelGSM_Text, lv_color_hex(0x00FF00), LV_PART_MAIN);
                    }
                }
                else
                {
                    lv_label_set_text(ui_LabelGSM, "GSM: No Network");
                    lv_obj_set_style_text_color(ui_LabelGSM, lv_color_hex(0xFF0000), LV_PART_MAIN);

                    if (ui_LabelGSM_Icon)
                    {
                        lv_obj_set_style_text_color(ui_LabelGSM_Icon, lv_color_hex(0xFF0000), LV_PART_MAIN);
                        lv_obj_set_style_text_color(ui_LabelGSM_Text, lv_color_hex(0xFF0000), LV_PART_MAIN);
                    }
                }
            }
            example_lvgl_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// --- SETTINGS TOGGLES (External C) ---
bool g_sound_enabled = true;

extern "C" void toggle_ble(bool enable)
{
    // Use the TAG defined at top of file, or define local
    ESP_LOGI("SETTINGS", "Toggle BLE: %d", enable);
    if (enable)
    {
        ble_spp_server_advertise();
    }
    else
    {
        ble_spp_server_stop_advertising();
        update_ble_connection_status(false);
    }
}

extern "C" void trigger_fall_sms(void)
{
    ESP_LOGE("ALERT", "=== FALL SIMULATION TRIGGERED ===");

    char sms_msg[160];
    snprintf(sms_msg, sizeof(sms_msg), "UPOZORENJE! Detektovan SIMULIRAN pad!\nLokacija: https://maps.google.com/?q=%.6f,%.6f\nPuls: %d",
             g_latitude, g_longitude, (int)heartRate);

    // For simulation, we still use the single number from UI for safety/test
    gsm_send_sms_async(ui_get_phone_number(), sms_msg);
}

extern "C" void trigger_real_fall_sms(void)
{
    ESP_LOGE("ALERT", "=== REAL FALL DETECTED (Timer reached 0) ===");

    // 0: Watch Only, 1: App + Watch
    if (g_action_origin == 1) {
        ESP_LOGI("ALERT", "Action Origin is 'App + Watch'. Watch deferred action to App.");
        // The watch already sent BLE message so App should be handling it.
        return;
    }

    char google_maps_url[128];
    snprintf(google_maps_url, sizeof(google_maps_url), "https://maps.google.com/?q=%.6f,%.6f", g_latitude, g_longitude);

    // 1. Send SMS if enabled
    if (g_w_enable_sms && strlen(g_w_sms_numbers) > 0) {
        char *nums = strdup(g_w_sms_numbers);
        char *token = strtok(nums, ",");
        while (token != NULL) {
            char sms_msg[160];
            snprintf(sms_msg, sizeof(sms_msg), "SOS! LifeLink detektovao pad!\nLokacija: %s\nPuls: %d",
                     google_maps_url, (int)heartRate);
            
            // Trim whitespace if any
            while(*token == ' ') token++;
            
            ESP_LOGI("ALERT", "Sending SOS SMS to: %s", token);
            gsm_send_sms_async(token, sms_msg);
            token = strtok(NULL, ",");
        }
        free(nums);
    }

    // 2. Sequential Calls if enabled
    if (g_w_enable_call && strlen(g_w_call_numbers) > 0) {
        char *nums = strdup(g_w_call_numbers);
        char *token = strtok(nums, ",");
        while (token != NULL) {
             // Trim whitespace if any
            while(*token == ' ') token++;
            
            ESP_LOGI("ALERT", "Initiating sequential call to: %s", token);
            gsm_make_call(token);
            
            // Wait for call (A6/SIM800 requires some time or manual hangup check)
            // For simplicity, we wait 15s then hang up and try next, unless answered?
            // Actually, gsm_make_call is synchronous if it waits for OK.
            vTaskDelay(pdMS_TO_TICKS(15000));
            gsm_hang_up();
            
            token = strtok(NULL, ",");
        }
        free(nums);
    }

    // 3. SOS Call if enabled (Highest Priority / Final Action)
    if (g_w_enable_sos && strlen(g_w_sos_number) > 0) {
        ESP_LOGI("ALERT", "Initiating emergency SOS call to: %s", g_w_sos_number);
        gsm_make_call(g_w_sos_number);
    }
}

extern "C" void toggle_sound(bool enable)
{
    g_sound_enabled = enable;
    ESP_LOGI("SETTINGS", "Toggle Sound: %d", enable);
}
