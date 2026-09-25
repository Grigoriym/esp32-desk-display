#pragma once

// Pure parsing/mapping for the weather data, no ESP-IDF dependencies, so it
// also compiles on the PC for the unit tests in test/.

#include <stdbool.h>
#include <stdint.h>

// Hours of rain forecast fetched (Open-Meteo forecast_hours; the first one is
// the current hour) and the chance of precipitation that counts as rain.
#define WEATHER_RAIN_HOURS    12
#define WEATHER_RAIN_MIN_PROB 50

typedef struct {
    int temp_c;       // rounded
    int weather_code; // raw WMO code, see weather_icon_for_code()
    int wind_kmh;     // rounded
    int uv_max;       // today's max UV index, rounded
    char sunrise[6];  // "HH:MM", local (Berlin) time
    char sunset[6];
    int rain_in_h;      // hours until the first rainy hour: 0 = now, -1 = none in the window
    char rain_from[6];  // "HH:MM" of that hour, "" if none
    char rain_until[6]; // first dry hour after it, "" if it rains to the end of the window
} weather_t;

// Parses an Open-Meteo forecast response (current_weather + daily
// sunrise/sunset/uv_index_max, forecast_days=1, + hourly
// precipitation_probability). An hour is rainy at >= WEATHER_RAIN_MIN_PROB. *out is only written on
// success; false on invalid JSON or any missing field.
bool weather_parse(const char *json, weather_t *out);

// Maps a WMO weather code (Open-Meteo's "weathercode") to an 8x8 icon
// bitmap (column-major, one byte per column, for display_draw_icon()).
const uint8_t *weather_icon_for_code(int weather_code);
