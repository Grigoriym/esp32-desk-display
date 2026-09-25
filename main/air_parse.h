#pragma once

// Pure parsing/mapping for the air-quality data, no ESP-IDF dependencies, so
// it also compiles on the PC for the unit tests in test/.

#include <stdbool.h>

typedef enum {
    POLLEN_ALDER,
    POLLEN_BIRCH,
    POLLEN_GRASS,
    POLLEN_MUGWORT,
    POLLEN_RAGWEED,
    POLLEN_COUNT
} pollen_t;

typedef struct {
    int aqi;                  // European AQI, rounded
    int pollen[POLLEN_COUNT]; // grains/m3, rounded; 0 when there's no data (off season)
} air_t;

// Parses an Open-Meteo air-quality response ("current" with european_aqi and
// the <name>_pollen fields). *out is only written on success; false on
// invalid JSON or a missing AQI. A missing or null pollen value counts as 0.
bool air_parse(const char *json, air_t *out);

// "GOOD" .. "EXTREME", the European AQI bands.
const char *air_aqi_label(int aqi);

// "ALDER", "BIRCH", ...
const char *air_pollen_name(pollen_t p);

// "LOW" / "MED" / "HIGH", or NULL below 1 grain/m3. One rough scale for all
// types: 1-10 low, 11-50 medium, above 50 high.
const char *air_pollen_level(int grains);
