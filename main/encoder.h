#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include "input.h"

// KY-040 rotary encoder, pins in docs/WIRING.md. Its event queue also takes
// the web API's commands (input.h).

// Creates the event queue, then configures the pins, the rotation interrupt
// and the button polling task. The queue works even if the pins fail.
esp_err_t encoder_init(void);

// Waits up to timeout for the next event. Returns false on timeout (or
// straight away after sleeping, if encoder_init() never ran).
bool encoder_wait_event(input_event_t *ev, TickType_t timeout);

// Queues an event from another task (the web API) without waiting; false if
// the queue is full or missing.
bool encoder_post(input_event_t ev);
