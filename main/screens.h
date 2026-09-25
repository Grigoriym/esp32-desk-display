#pragma once

// What each main screen shows, as text: pure layout logic with no ESP-IDF or
// display dependencies, so it compiles on the PC for the unit tests. main.c
// draws the result.

#include <stdbool.h>
#include "weather_parse.h"
#include "bme280_reading.h"
#include "bvg_parse.h"

// Cycled with the encoder (press = back to HOME).
typedef enum { SCREEN_HOME, SCREEN_OUTDOOR, SCREEN_INDOOR, SCREEN_BVG, SCREEN_COUNT } screen_t;

// Everything the screens draw from.
typedef struct {
    bool weather_ok; // false until the first fetch succeeds
    weather_t weather;
    bool indoor_ok; // false until the first BME280 read succeeds
    bme280_reading_t indoor;
    bool bvg_ok;     // false until the first departures fetch succeeds
    bool bvg_failed; // last fetch failed (shown only while there's no data)
    bvg_departures_t bvg;
    int bvg_walk_min;     // trains sooner than this many minutes are hidden
    int bvg_walk_comfort; // "GO NOW" at exactly this many minutes left
} screen_data_t;

// Pages 1-7 of the panel, each a left and right text column. A page with an
// empty right column is drawn centred; an empty page is blanked. Rows longer
// than the panel's 21 characters just clip at the edge.
#define SCREEN_TEXT_LEN 28
typedef struct {
    char text[8][2][SCREEN_TEXT_LEN];
} screen_rows_t;

// Fills *out for the given screen. now_min is the local time as minutes
// since midnight (for the departure countdown).
void screen_layout(screen_t screen, const screen_data_t *data, int now_min, screen_rows_t *out);

// Minutes from now_min until a departure at hour:minute, negative once it's
// gone. Departures are at most an hour ahead, so wrapping over midnight is
// resolved towards the nearer day.
int screen_minutes_until(int now_min, int hour, int minute);
