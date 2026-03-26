#ifndef AXP2101_H
#define AXP2101_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver/i2c.h"
#include "esp_err.h"

// I2C Address
#define AXP2101_I2C_ADDR 0x34

    /**
     * @brief Initialize AXP2101 Power Management Unit
     * @param i2c_num I2C port number (must be initialized by main app)
     * @return ESP_OK on success
     */
    esp_err_t axp_init(i2c_port_t i2c_num);

    /**
     * @brief Enable AXP2101 Power Rails (LDOs/DC-DCs) required for peripherals
     * @return ESP_OK on success
     */
    esp_err_t axp_enable_power(void);

    /**
     * @brief Get Battery Voltage in millivolts
     * @return voltage in mV or -1 on error
     */
    int axp_get_batt_vol(void);

    /**
     * @brief Get Battery Percentage (0-100)
     * @return percentage or -1 on error
     */
    int axp_get_batt_percent(void);

    /**
     * @brief Set battery charge current
     * @param ma Target charge current in mA (0, 100, 125, 150, 175, 200, 300, 400, 500, 600, 700, 800, 900, 1000)
     * @return ESP_OK on success
     */
    esp_err_t axp_set_charge_current(uint16_t ma);

    /**
     * @brief Read IRQ Status register
     * @param reg IRQ status register (0x40 to 0x42)
     * @param status Pointer to store status bits
     * @return ESP_OK on success
     */
    esp_err_t axp_get_irq_status(uint8_t reg, uint8_t *status);

    /**
     * @brief Clear IRQ Status bits (by writing 1 to them)
     * @param reg IRQ status register (0x40 to 0x42)
     * @param status Bits to clear
     * @return ESP_OK on success
     */
    esp_err_t axp_clear_irq_status(uint8_t reg, uint8_t status);

    /**
     * @brief Power off the device (shutdown)
     */
    void axp_power_off(void);

    bool axp_is_charging(void);

#ifdef __cplusplus
}
#endif

#endif // AXP2101_H
