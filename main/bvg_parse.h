#pragma once

// Pure parsing for the departures data, no ESP-IDF dependencies, so it also
// compiles on the PC for the unit tests in test/.

#include <stdbool.h>
#include <stddef.h>

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

// Parses a transport.rest (hafas-client) departures response: at most
// BVG_MAX_DEPARTURES entries, cancelled trips ("when": null) and malformed
// entries skipped. *out is only written on success; false on invalid JSON or
// no "departures" array.
bool bvg_parse(const char *json, bvg_departures_t *out);

// Makes a stop name drawable with the display font (A-Z, 0-9, space, '-',
// '/'): uppercases, spells out German umlauts, drops the " (Berlin)" suffix
// and anything else the font can't draw.
void bvg_display_name(const char *src, char *dst, size_t dst_size);
