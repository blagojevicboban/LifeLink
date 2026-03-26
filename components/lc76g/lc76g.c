#include "lc76g.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "LC76G";
static i2c_port_t g_i2c_port = I2C_NUM_0;

/**
 * @brief Helper to write to TCA9554 port expander
 */
static esp_err_t tca9554_write(uint8_t reg, uint8_t val)
{
    esp_err_t ret = ESP_FAIL;
    if (g_i2c_mux && xSemaphoreTake(g_i2c_mux, pdMS_TO_TICKS(100))) {
        uint8_t buf[2] = {reg, val};
        ret = i2c_master_write_to_device(g_i2c_port, TCA9554_ADDR, buf, sizeof(buf), pdMS_TO_TICKS(100));
        xSemaphoreGive(g_i2c_mux);
    }
    return ret;
}

/**
 * @brief Helper to read from TCA9554 port expander
 */
static esp_err_t tca9554_read(uint8_t reg, uint8_t *val)
{
    esp_err_t ret = ESP_FAIL;
    if (g_i2c_mux && xSemaphoreTake(g_i2c_mux, pdMS_TO_TICKS(100))) {
        ret = i2c_master_write_read_device(g_i2c_port, TCA9554_ADDR, &reg, 1, val, 1, pdMS_TO_TICKS(100));
        xSemaphoreGive(g_i2c_mux);
    }
    return ret;
}

esp_err_t lc76g_hw_reset(void)
{
    ESP_LOGI(TAG, "Ensuring Power and Peripheral Init (TCA9554 EXIO4, EXIO6 & EXIO7)...");
    
    uint8_t config, output;
    
    if (tca9554_read(3, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read TCA9554 Config Register! Is address 0x20 correct?");
        return ESP_FAIL;
    }
    // Set pins as Output
    config &= ~(1 << TCA9554_GPS_EN_PIN);  
    config &= ~(1 << TCA9554_I2C_SEL_PIN); 
    config &= ~(1 << TCA9554_GPS_RST_PIN); 
    if (tca9554_write(3, config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write TCA9554 Config Register!");
        return ESP_FAIL;
    }
    
    if (tca9554_read(1, &output) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read TCA9554 Output Register!");
        return ESP_FAIL;
    }

    output |= (1 << TCA9554_GPS_EN_PIN);      // Enable GPS Power
    output &= ~(1 << TCA9554_I2C_SEL_PIN);    // Pull Pin 5 LOW (I2C Mode)
    output &= ~(1 << TCA9554_GPS_RST_PIN);    // Pull RESET LOW
    tca9554_write(1, output);
    
    vTaskDelay(pdMS_TO_TICKS(500)); // Reset pulse
    
    output |= (1 << TCA9554_GPS_RST_PIN); // Pull RESET HIGH to release
    if (tca9554_write(1, output) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to release GPS Reset via TCA9554!");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "GPS_RST released. Waiting for module startup (2s)...");
    vTaskDelay(pdMS_TO_TICKS(2000)); // Increased wait for typical startup
    
    return ESP_OK;
}

esp_err_t lc76g_init(i2c_port_t i2c_num)
{
    g_i2c_port = i2c_num;
    ESP_LOGI(TAG, "Initializing LC76G on I2C (H/W Reset)");
    return lc76g_hw_reset();
}

esp_err_t lc76g_read_data(uint8_t *buffer, size_t max_len, size_t *read_len)
{
    if (read_len) *read_len = 0;
    if (!buffer || max_len == 0) return ESP_ERR_INVALID_ARG;

    if (!g_i2c_mux || !xSemaphoreTake(g_i2c_mux, pdMS_TO_TICKS(500))) {
        return ESP_ERR_TIMEOUT;
    }

    // --- PHASE 1: Send Data Length Request ---
    uint8_t req_len_cmd[] = { 0x08, 0x00, 0x51, 0xAA, 0x04, 0x00, 0x00, 0x00 };
    esp_err_t ret = i2c_master_write_to_device(g_i2c_port, LC76G_ADDR_W, req_len_cmd, sizeof(req_len_cmd), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Phase 1: Write to 0x%02X failed: %s", LC76G_ADDR_W, esp_err_to_name(ret));
        xSemaphoreGive(g_i2c_mux);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay after length request

    // --- PHASE 2: Read Data Length ---
    uint8_t len_buf[4] = {0};
    ret = i2c_master_read_from_device(g_i2c_port, LC76G_ADDR_R, len_buf, 4, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Phase 2: Read from 0x%02X failed: %s", LC76G_ADDR_R, esp_err_to_name(ret));
        xSemaphoreGive(g_i2c_mux);
        return ret;
    }

    uint32_t data_len = (len_buf[0]) | (len_buf[1] << 8) | (len_buf[2] << 16) | (len_buf[3] << 24);
    if (data_len == 0) {
        xSemaphoreGive(g_i2c_mux);
        return ESP_OK; // No data yet
    }
    
    ESP_LOGI(TAG, "GPS Data Available: %lu bytes", (unsigned long)data_len);

    if (data_len > max_len) {
        ESP_LOGW(TAG, "GPS sending %lu bytes, but buffer is only %u", (unsigned long)data_len, (unsigned int)max_len);
        data_len = max_len;
    }

    // --- PHASE 3: Send Confirmation/Ack with length ---
    uint8_t ack_cmd[8] = { 0x00, 0x20, 0x51, 0xAA };
    memcpy(&ack_cmd[4], len_buf, 4);
    
    vTaskDelay(pdMS_TO_TICKS(100)); 
    ret = i2c_master_write_to_device(g_i2c_port, LC76G_ADDR_W, ack_cmd, sizeof(ack_cmd), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Phase 3: Write ACK to 0x%02X failed: %s", LC76G_ADDR_W, esp_err_to_name(ret));
        xSemaphoreGive(g_i2c_mux);
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // 100ms delay before payload read (Waveshare standard)
    
    // --- PHASE 4: Read actual NMEA Payload ---
    uint16_t to_read = (data_len > max_len) ? (uint16_t)max_len : (uint16_t)data_len;
    ret = i2c_master_read_from_device(g_i2c_port, LC76G_ADDR_R, buffer, to_read, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Phase 4: Read payload from 0x%02X failed: %s", LC76G_ADDR_R, esp_err_to_name(ret));
        xSemaphoreGive(g_i2c_mux);
        return ret;
    }
    xSemaphoreGive(g_i2c_mux);
    *read_len = to_read;
    return ESP_OK;
}
