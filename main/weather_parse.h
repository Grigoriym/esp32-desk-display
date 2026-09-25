#pragma once

// Pure parsing/mapping for the weather data, no ESP-IDF dependencies, so it
// also compiles on the PC for the unit tests in test/.

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int temp_c;       // rounded
    int weather_code; // raw WMO code, see weather_icon_for_code()
    int wind_kmh;     // rounded
    int uv_max;       // today's max UV index, rounded
    char sunrise[6];  // "HH:MM", local (Berlin) time
    char sunset[6];
} weather_t;

// Parses an Open-Meteo forecast response (current_weather + daily
// sunrise/sunset/uv_index_max, forecast_days=1). *out is only written on
// success; false on invalid JSON or any missing field.
bool weather_parse(const char *json, weather_t *out);

// Maps a WMO weather code (Open-Meteo's "weathercode") to an 8x8 icon
// bitmap (column-major, one byte per column, for display_draw_icon()).
const uint8_t *weather_icon_for_code(int weather_code);
