#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "screens.h"

// Phone access on the home WiFi: http://desk.local (mDNS), a small page at /
// and a JSON API under /api (see web_api.h, README.md). No auth: LAN only.

// Starts mDNS and the HTTP server. Call after WiFi is initialised (it
// needn't be connected yet).
esp_err_t web_start(void);

// Hands the server a copy of what the screens show, for GET /api/status.
// Cheap; the main loop calls it every tick and after each input.
void web_publish(const screen_data_t *data, screen_t screen, bool panel_on);

// True if /api/status was read in the last couple of minutes: departures are
// then fetched even while the BVG screen isn't up, so the API's are fresh.
bool web_recently_polled(void);
