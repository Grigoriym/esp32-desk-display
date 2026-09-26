#pragma once

#include "esp_err.h"
#include "scd41_parse.h"
#include "driver/i2c_master.h"

// Looks for an SCD41 at 0x62 and starts low-power periodic measurement: one
// reading every 30 s, the first ~30 s after this call. Blocks ~0.5 s.
esp_err_t scd41_init(i2c_master_bus_handle_t bus);

// The latest reading if the sensor has a new one, ESP_ERR_NOT_FINISHED if
// not yet (normal, poll again later).
esp_err_t scd41_read(scd41_reading_t *out);
