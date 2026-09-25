#pragma once

// Pure time/RTC-register helpers, no ESP-IDF dependencies, so they also
// compile on the PC for the unit tests in test/.

#include <stdint.h>
#include <time.h>

// POSIX TZ rule for Berlin: CET (UTC+1), CEST (UTC+2) from the last Sunday
// of March 02:00 until the last Sunday of October 03:00. The C library
// applies the DST switch itself, so no manual flip twice a year.
#define LOCAL_TZ "CET-1CEST,M3.5.0,M10.5.0/3"

// struct tm (UTC) -> epoch seconds, independent of TZ (mktime() would treat
// t as local time once TZ is set). Only the date/time fields are read.
time_t utc_to_epoch(const struct tm *t);

// DS3231 registers hold packed BCD: 0x59 = 59.
uint8_t bcd_to_bin(uint8_t v);
uint8_t bin_to_bcd(uint8_t v);
