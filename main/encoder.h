#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

// KY-040 rotary encoder, pins in docs/WIRING.md.
typedef enum {
    ENCODER_EV_CW,    // one detent clockwise
    ENCODER_EV_CCW,   // one detent counter-clockwise
    ENCODER_EV_PRESS, // button pressed (debounced)
} encoder_event_t;

// Configures the pins, the rotation interrupt and the button polling task.
esp_err_t encoder_init(void);

// Waits up to timeout for the next event. Returns false on timeout (or
// straight away after sleeping, if encoder_init() failed).
bool encoder_wait_event(encoder_event_t *ev, TickType_t timeout);
