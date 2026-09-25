#pragma once

#include "weather_parse.h"
#include "esp_err.h"

// Fetches current weather plus today's sunrise/sunset/UV from Open-Meteo for
// the hardcoded lat/long. *out is only written on success.
esp_err_t weather_fetch(weather_t *out);
