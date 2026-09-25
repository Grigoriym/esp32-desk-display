#pragma once

// Readings as InfluxDB line protocol, for the dashboard in server/. Pure, no
// ESP-IDF dependencies, so it also compiles on the PC for the unit tests.

#include <stdbool.h>
#include <stddef.h>
#include "screens.h"

typedef struct {
    long uptime_s;
    int heap_free_kb;
    bool rssi_ok; // false while WiFi is down
    int rssi;     // dBm
} metrics_device_t;

// One line per measurement (indoor, outdoor, air, device), each only once
// its data exists. All values are written as floats so a field never
// changes type. No timestamps: the server stamps the write. Returns the
// length, or -1 if buf is too small.
int metrics_format(const screen_data_t *d, const metrics_device_t *dev, char *buf, size_t len);
