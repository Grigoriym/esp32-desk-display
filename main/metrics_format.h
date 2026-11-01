#pragma once

// Readings for the dashboard in server/: one minute's values as a compact
// record (what the backlog keeps on flash), and as InfluxDB line protocol
// (what gets uploaded). Pure, no ESP-IDF dependencies, so it also compiles on
// the PC for the unit tests.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "screens.h"

// Anything before this (2026-01-01 UTC) means the clock was never set.
#define METRICS_MIN_VALID_TIME 1767225600L

// Longest text one record can format to, terminator included (checked by a
// test against the largest values each field can hold).
#define METRICS_RECORD_TEXT_MAX 420

typedef struct {
    long uptime_s;
    int heap_free_kb;
    bool rssi_ok; // false while WiFi is down
    int rssi;     // dBm
} metrics_device_t;

// Same size as BACKLOG_PAYLOAD_SIZE (backlog_ring.h). Changing the layout makes
// records already on flash unreadable: only do it together with a new magic in
// backlog_ring.c.
typedef struct __attribute__((packed)) {
    uint32_t time_utc; // 0 = clock not set: uploaded unstamped, never stored
    uint8_t flags;     // METRICS_HAS_*
    int8_t rssi;
    int16_t temp_c100;
    uint16_t humidity_x10;
    uint16_t pressure_x10;
    uint16_t co2_ppm;
    int8_t out_temp_c;
    uint8_t wind_kmh;
    uint8_t uv;
    uint8_t weather_code;
    uint16_t aqi;
    uint16_t pollen[POLLEN_COUNT];
    uint32_t uptime_s;
    uint16_t heap_free_kb;
} metrics_record_t;

// Takes one minute's values; time_utc is the current epoch time, and anything
// before METRICS_MIN_VALID_TIME is stored as "clock not set".
void metrics_record_make(const screen_data_t *d, const metrics_device_t *dev, long time_utc,
                         metrics_record_t *out);

// One line per measurement (indoor, co2, outdoor, air, device), each only if
// the record has its data. Fields are written without an integer suffix so a
// field is always a float on the server, and each line carries the record's
// time (none when the clock wasn't set: the server stamps the write).
// Appending to a buffer is the caller's: pass the rest of it. Returns the
// length, or -1 if buf is too small.
int metrics_format(const metrics_record_t *r, char *buf, size_t len);
