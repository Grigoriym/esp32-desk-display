#pragma once

#include "weather_parse.h"
#include "air_parse.h"
#include "esp_err.h"

// Fetches current weather plus today's sunrise/sunset/UV and the next
// hours' rain chance from Open-Meteo for
// the hardcoded lat/long. *out is only written on success.
esp_err_t weather_fetch(weather_t *out);

// Fetches the current European AQI and pollen for the same spot. *out is only
// written on success.
esp_err_t air_fetch(air_t *out);
