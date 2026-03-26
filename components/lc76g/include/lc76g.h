#ifndef LC76G_H
#define LC76G_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"

extern SemaphoreHandle_t g_i2c_mux;

// I2C Addresses for Waveshare LC76G (Protocol specific)
#define LC76G_ADDR_W 0x50
#define LC76G_ADDR_R 0x54

// TCA9554 Port Expander Address
#define TCA9554_ADDR 0x20
#define TCA9554_GPS_EN_PIN  4  // EXIO4: GPS Enable (Active HIGH)
#define TCA9554_I2C_SEL_PIN 5  // EXIO5: I2C Selection (LOW = I2C, HIGH = UART)
#define TCA9554_GPS_RST_PIN 7  // EXIO7: GPS Reset Pin (Active LOW)

    /**
     * @brief Initialize the LC76G driver and reset the module.
     *
     * @param i2c_num The I2C port number to use (e.g., I2C_NUM_0).
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t lc76g_init(i2c_port_t i2c_num);

    /**
     * @brief Send a reset pulse to the GPS module via TCA9554 port expander.
     * @return esp_err_t ESP_OK on success.
     */
    esp_err_t lc76g_hw_reset(void);

    /**
     * @brief Read available NMEA data from the LC76G.
     *
     * @param buffer Pointer to the buffer to store data.
     * @param max_len Maximum length of the buffer.
     * @param read_len Pointer to store the actual number of bytes read.
     * @return esp_err_t ESP_OK on success (even if 0 bytes read), ESP_FAIL on I2C error.
     */
    esp_err_t lc76g_read_data(uint8_t *buffer, size_t max_len, size_t *read_len);

#ifdef __cplusplus
}
#endif

#endif // LC76G_H
