#pragma once

// Pure parsing for public holidays (Nager.Date), no ESP-IDF dependencies, so
// it also compiles on the PC for the unit tests in test/.

#include <stdbool.h>

// Germany has 9 nationwide holidays plus up to ~4 per state.
#define HOLIDAYS_MAX 20

typedef struct {
    int ymd;       // date as YYYYMMDD, e.g. 20261003
    char name[22]; // English name, uppercased, only A-Z/0-9/space: "NEW YEARS DAY"
} holiday_t;

typedef struct {
    int year; // which year's list this is; 0 = none loaded
    int count;
    holiday_t day[HOLIDAYS_MAX]; // in date order, as the API sends them
} holidays_t;

// Parses a Nager.Date /PublicHolidays/<year>/DE response, keeping the
// nationwide holidays and those of region (e.g. "DE-BE" for Berlin).
// *out is only written on success; false on invalid JSON or no list.
bool holidays_parse(const char *json, int year, const char *region, holidays_t *out);

// Index of the first holiday on or after today_ymd, or -1 if none is left.
int holidays_next(const holidays_t *h, int today_ymd);

// Which year's list to fetch next: today's year while none is loaded (or an
// older one), the next year once this year's are all past; 0 when the loaded
// list is fine.
int holidays_wanted_year(const holidays_t *h, int today_ymd);
