#pragma once

#include <stdbool.h>
#include "esp_err.h"

#define BVG_MAX_DEPARTURES 6

typedef struct {
    char line[6];       // e.g. "U5"
    char direction[22]; // display-ready: uppercase, ASCII-only, e.g. "HAUPTBAHNHOF"
    int hour, minute;   // real (delay included) local departure time
} bvg_departure_t;

typedef struct {
    int count;
    bvg_departure_t dep[BVG_MAX_DEPARTURES];
} bvg_departures_t;

// Starts the background fetch task. Fetching runs off the main loop, so a
// slow or down API (it has outages; timeouts take ~15s) never freezes the
// clock or the encoder.
esp_err_t bvg_start(void);

// Fetch only while the departures screen is showing: every 60s while
// active, straight away on activation if the last result is older than that.
void bvg_set_active(bool active);

// If a fetch finished since the last call, copies its outcome and returns
// true. have_data stays true once any fetch has succeeded (last good list is
// kept on failure); failed reflects the latest attempt. Departures are the
// next ones from the stop/direction in bvg_secrets.h, cancelled trips
// skipped; count 0 means nothing runs in the next hour.
bool bvg_take_update(bvg_departures_t *out, bool *have_data, bool *failed);
