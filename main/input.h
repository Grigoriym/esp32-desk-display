#pragma once

// What the main loop reacts to between ticks: the KY-040 knob, plus commands
// from the web API (web.c) sent through the same queue so they apply at once.
// Plain data, no ESP-IDF headers, so pure code (web_api.c) can build them.

typedef enum {
    INPUT_CW,     // knob: one detent clockwise (next screen)
    INPUT_CCW,    // knob: one detent counter-clockwise (previous screen)
    INPUT_PRESS,  // knob: button pressed (panel off/on)
    INPUT_PANEL,  // web: arg 1 = panel on, 0 = off
    INPUT_SCREEN, // web: arg = screen_t to show (also wakes the panel)
} input_type_t;

typedef struct {
    input_type_t type;
    int arg;
} input_event_t;
