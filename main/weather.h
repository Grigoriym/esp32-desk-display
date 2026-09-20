#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

// Fetches current temperature from Open-Meteo for the hardcoded lat/long,
// formatted rounded into temp_buf (e.g. "18C" or "-3C"), plus the raw WMO
// weather code into *weather_code_out (see weather_icon_for_code()).
esp_err_t weather_fetch(char *temp_buf, size_t temp_buf_size, int *weather_code_out);

// Maps a WMO weather code (Open-Meteo's "weathercode") to an 8x8 icon
// bitmap (column-major, one byte per column, for display_draw_icon()).
const uint8_t *weather_icon_for_code(int weather_code);
