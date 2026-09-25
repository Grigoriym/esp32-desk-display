#include <stdio.h>
#include <string.h>
#include <math.h>
#include "screens.h"

// Font has no '%' or '.': humidity is "45H", pressure whole hPa.

#define ROW(r, c) out->text[r][c], sizeof(out->text[r][c])

int screen_minutes_until(int now_min, int hour, int minute)
{
    int diff = (hour * 60 + minute) - now_min;
    if (diff > 12 * 60) diff -= 24 * 60;
    if (diff < -12 * 60) diff += 24 * 60;
    return diff;
}

// Page 7 of HOME: when the next rain starts, or until when the current one
// lasts, within the forecast window.
static void rain_hint(const weather_t *w, screen_rows_t *out)
{
    if (w->rain_in_h < 0) snprintf(ROW(7, 0), "NO RAIN %dH", WEATHER_RAIN_HOURS);
    else if (w->rain_in_h > 0) snprintf(ROW(7, 0), "RAIN %s", w->rain_from);
    else if (w->rain_until[0]) snprintf(ROW(7, 0), "RAIN TILL %s", w->rain_until);
    else snprintf(ROW(7, 0), "RAIN NEXT %dH", WEATHER_RAIN_HOURS);
}

static void layout_home(const screen_data_t *d, screen_rows_t *out)
{
    // Page 3 is drawn with the weather icon in front (main.c).
    if (d->weather_ok) snprintf(ROW(3, 0), "%dC", d->weather.temp_c);
    else snprintf(ROW(3, 0), "--C");
    if (d->indoor_ok) {
        snprintf(ROW(5, 0), "IN %dC %dH", (int)lroundf(d->indoor.temp_c),
                 (int)lroundf(d->indoor.humidity_pct));
    }
    if (d->weather_ok) rain_hint(&d->weather, out);
}

static void layout_outdoor(const screen_data_t *d, screen_rows_t *out)
{
    snprintf(ROW(2, 0), "OUTDOOR");
    if (!d->weather_ok) {
        snprintf(ROW(4, 0), "--");
        return;
    }
    snprintf(ROW(4, 0), "RISE %s", d->weather.sunrise);
    snprintf(ROW(4, 1), "SET %s", d->weather.sunset);
    snprintf(ROW(6, 0), "WIND %dKMH", d->weather.wind_kmh);
    snprintf(ROW(6, 1), "UV %d", d->weather.uv_max);
}

// AQI, then the two strongest pollen types that are present at all.
static void layout_air(const screen_data_t *d, screen_rows_t *out)
{
    snprintf(ROW(2, 0), "AIR");
    if (!d->air_ok) {
        snprintf(ROW(4, 0), "--");
        return;
    }
    snprintf(ROW(4, 0), "AQI %d", d->air.aqi);
    snprintf(ROW(4, 1), "%s", air_aqi_label(d->air.aqi));

    bool shown[POLLEN_COUNT] = {false};
    int row = 6;
    for (; row <= 7; row++) {
        int best = -1;
        for (int i = 0; i < POLLEN_COUNT; i++) {
            if (!shown[i] && air_pollen_level(d->air.pollen[i])
                && (best < 0 || d->air.pollen[i] > d->air.pollen[best])) {
                best = i;
            }
        }
        if (best < 0) break;
        shown[best] = true;
        snprintf(ROW(row, 0), "%s", air_pollen_name((pollen_t)best));
        snprintf(ROW(row, 1), "%s", air_pollen_level(d->air.pollen[best]));
    }
    if (row == 6) snprintf(ROW(6, 0), "POLLEN NONE");
}

static void layout_indoor(const screen_data_t *d, screen_rows_t *out)
{
    snprintf(ROW(2, 0), "INDOOR");
    if (!d->indoor_ok) {
        snprintf(ROW(4, 0), "--");
        return;
    }
    snprintf(ROW(4, 0), "TEMP %dC", (int)lroundf(d->indoor.temp_c));
    snprintf(ROW(4, 1), "HUM %dH", (int)lroundf(d->indoor.humidity_pct));
    snprintf(ROW(6, 0), "%d HPA", (int)lroundf(d->indoor.pressure_hpa));
}

// Before the comfortable-walk point: countdown; at it: go; after it (but
// still catchable): hurry.
static void leave_hint(int mins, int walk_comfort, screen_rows_t *out)
{
    int leave_in = mins - walk_comfort;
    if (leave_in > 0) snprintf(ROW(3, 0), "LEAVE IN %d", leave_in);
    else if (leave_in == 0) snprintf(ROW(3, 0), "GO NOW");
    else snprintf(ROW(3, 0), "HURRY");
}

static void layout_bvg(const screen_data_t *d, int now_min, screen_rows_t *out)
{
    if (!d->bvg_ok) {
        snprintf(ROW(2, 0), "BVG");
        snprintf(ROW(4, 0), "%s", d->bvg_failed ? "NO DATA" : "LOADING");
        return;
    }
    // Title from the data, e.g. "U5 HAUPTBAHNHOF"; then the next trains that
    // can still be caught on foot, with a leave-by hint for the first.
    if (d->bvg.count > 0) snprintf(ROW(2, 0), "%s %s", d->bvg.dep[0].line, d->bvg.dep[0].direction);
    else snprintf(ROW(2, 0), "BVG");

    int row = 5;
    for (int i = 0; i < d->bvg.count && row <= 7; i++) {
        const bvg_departure_t *dep = &d->bvg.dep[i];
        int mins = screen_minutes_until(now_min, dep->hour, dep->minute);
        if (mins < d->bvg_walk_min) continue;
        if (row == 5) leave_hint(mins, d->bvg_walk_comfort, out);
        snprintf(ROW(row, 0), "%02d:%02d", dep->hour, dep->minute);
        snprintf(ROW(row, 1), "%d MIN", mins);
        row++;
    }
    if (row == 5) snprintf(ROW(4, 0), "NO TRAINS");
}

void screen_layout(screen_t screen, const screen_data_t *data, int now_min, screen_rows_t *out)
{
    memset(out, 0, sizeof(*out));
    switch (screen) {
        case SCREEN_HOME: layout_home(data, out); break;
        case SCREEN_OUTDOOR: layout_outdoor(data, out); break;
        case SCREEN_AIR: layout_air(data, out); break;
        case SCREEN_INDOOR: layout_indoor(data, out); break;
        case SCREEN_BVG: layout_bvg(data, now_min, out); break;
        default: break;
    }
}
