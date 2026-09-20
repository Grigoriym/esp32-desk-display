#pragma once

#include <stddef.h>
#include "esp_err.h"

// Starts SNTP and blocks until the first time sync completes.
esp_err_t clock_sync_time(void);

// Formats the current local time as "HH:MM" into buf (needs at least 6 bytes).
void clock_format_now(char *buf, size_t buf_size);
