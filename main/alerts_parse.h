#pragma once

// Pure parsing for DWD weather warnings (via Bright Sky), no ESP-IDF
// dependencies, so it also compiles on the PC for the unit tests in test/.

#include <stdbool.h>

// DWD warning levels: yellow, orange, red, violet on their map.
typedef enum { ALERT_MINOR = 1, ALERT_MODERATE, ALERT_SEVERE, ALERT_EXTREME } alert_severity_t;

typedef struct {
    int count; // warnings in effect or announced; 0 = none
    // The one worth showing, only valid when count > 0: one that has started
    // beats an upcoming one, then the more severe, then the first listed.
    bool started; // onset is at or before now
    alert_severity_t severity;
    char event[24]; // DWD's English label uppercased, "HEAVY RAIN"
    char onset[6];  // local "HH:MM" if it starts today, else "DD/MM"
} alerts_t;

// Parses a Bright Sky /alerts response requested with tz=Europe/Berlin.
// now_local is the local time as "YYYY-MM-DDTHH:MM", compared with each
// onset. Test messages and all-clears are skipped. *out is only written on
// success; false on invalid JSON or a missing "alerts" list.
bool alerts_parse(const char *json, const char *now_local, alerts_t *out);
