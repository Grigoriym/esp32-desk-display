#pragma once

// The HTTP API's content (web.c serves it): status JSON and command parsing.
// Pure, no ESP-IDF dependencies, so it also compiles on the PC for the unit
// tests. The endpoints are described in README.md.

#include <stdbool.h>
#include <stddef.h>
#include "screens.h"
#include "input.h"

// Display state that isn't in screen_data_t.
typedef struct {
    screen_t screen;
    bool panel_on;
    char time[6];  // local "HH:MM"
    char date[11]; // local "YYYY-MM-DD"
    int now_min;   // local minutes since midnight (departure countdown)
} web_view_t;

// Body of GET /api/status. A section is null until its first fetch/read
// succeeded (same as "--" on the panel). Returns the length, or -1 if buf is
// too small (WEB_STATUS_MAX is enough).
#define WEB_STATUS_MAX 2048
int web_status_json(const screen_data_t *d, const web_view_t *v, char *buf, size_t len);

// POST /api/panel?set=on|off|toggle. False for any other value.
bool web_panel_command(const char *value, input_event_t *out);

// POST /api/screen?go=next|prev|<screen name>. False for any other value.
bool web_screen_command(const char *value, input_event_t *out);
