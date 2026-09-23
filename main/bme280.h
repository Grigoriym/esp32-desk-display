#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

typedef struct {
    float temp_c;
    float humidity_pct;
    float pressure_hpa;
} bme280_reading_t;

// Looks for a BME280 at 0x76/0x77, checks its chip ID (rejects a BMP280,
// which has no humidity) and loads its factory calibration.
esp_err_t bme280_init(i2c_master_bus_handle_t bus);

// Triggers one forced-mode measurement and waits for it (~10ms).
esp_err_t bme280_read(bme280_reading_t *out);
