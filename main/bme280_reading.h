#pragma once

// Plain data type, split from bme280.h so pure code (screens.c) and the host
// tests can use it without ESP-IDF headers.

typedef struct {
    float temp_c;
    float humidity_pct;
    float pressure_hpa;
} bme280_reading_t;
