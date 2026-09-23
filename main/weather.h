#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    int temp_c;       // rounded
    int weather_code; // raw WMO code, see weather_icon_for_code()
    int wind_kmh;     // rounded
    int uv_max;       // today's max UV index, rounded
    char sunrise[6];  // "HH:MM", local (Berlin) time
    char sunset[6];
} weather_t;

// Fetches current weather plus today's sunrise/sunset/UV from Open-Meteo for
// the hardcoded lat/long. *out is only written on success.
esp_err_t weather_fetch(weather_t *out);

// Maps a WMO weather code (Open-Meteo's "weathercode") to an 8x8 icon
// bitmap (column-major, one byte per column, for display_draw_icon()).
const uint8_t *weather_icon_for_code(int weather_code);
