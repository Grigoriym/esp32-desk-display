#pragma once

#include <stddef.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

// Looks for a DS3231 RTC on the bus and, if it holds a valid time, sets the
// system clock from it. Safe to skip on failure: the clock then just waits
// for NTP, as before the RTC was added.
esp_err_t clock_rtc_init(i2c_master_bus_handle_t bus);

// Starts SNTP and blocks until the first time sync completes. On success,
// also writes the synced time to the RTC (if one was found).
esp_err_t clock_sync_time(void);

// Formats the current local time as "HH:MM" into buf (needs at least 6 bytes).
void clock_format_now(char *buf, size_t buf_size);

// Formats the current local date as "DD/MM/YYYY" into buf (needs at least 11 bytes).
void clock_format_date(char *buf, size_t buf_size);
