#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "web_api.h"

static const char *const SEVERITY[] = {"", "minor", "moderate", "severe", "extreme"};
static const char *const POLLEN_KEY[POLLEN_COUNT] = {"alder", "birch", "grass", "mugwort", "ragweed"};

// One decimal: a float like 22.4f would otherwise print as 22.399999618530273.
static double one_decimal(float x)
{
    return round((double)x * 10) / 10;
}

static void add_outdoor(cJSON *root, const screen_data_t *d)
{
    if (!d->weather_ok) {
        cJSON_AddNullToObject(root, "outdoor");
        return;
    }
    const weather_t *w = &d->weather;
    cJSON *o = cJSON_AddObjectToObject(root, "outdoor");
    cJSON_AddNumberToObject(o, "temp_c", w->temp_c);
    cJSON_AddNumberToObject(o, "weather_code", w->weather_code);
    cJSON_AddNumberToObject(o, "wind_kmh", w->wind_kmh);
    cJSON_AddNumberToObject(o, "uv_max", w->uv_max);
    cJSON_AddStringToObject(o, "sunrise", w->sunrise);
    cJSON_AddStringToObject(o, "sunset", w->sunset);
    cJSON *rain = cJSON_AddObjectToObject(o, "rain");
    cJSON_AddNumberToObject(rain, "in_h", w->rain_in_h);
    cJSON_AddStringToObject(rain, "from", w->rain_from);
    cJSON_AddStringToObject(rain, "until", w->rain_until);
}

static void add_indoor(cJSON *root, const screen_data_t *d)
{
    if (!d->indoor_ok) {
        cJSON_AddNullToObject(root, "indoor");
        return;
    }
    cJSON *o = cJSON_AddObjectToObject(root, "indoor");
    cJSON_AddNumberToObject(o, "temp_c", one_decimal(d->indoor.temp_c));
    cJSON_AddNumberToObject(o, "humidity_pct", one_decimal(d->indoor.humidity_pct));
    cJSON_AddNumberToObject(o, "pressure_hpa", one_decimal(d->indoor.pressure_hpa));
}

// Separate sensor (SCD41) from indoor's BME280, so its own null.
static void add_co2(cJSON *root, const screen_data_t *d)
{
    if (!d->co2_ok) {
        cJSON_AddNullToObject(root, "co2");
        return;
    }
    cJSON *o = cJSON_AddObjectToObject(root, "co2");
    cJSON_AddNumberToObject(o, "ppm", d->co2.co2_ppm);
}

static void add_air(cJSON *root, const screen_data_t *d)
{
    if (!d->air_ok) {
        cJSON_AddNullToObject(root, "air");
        return;
    }
    cJSON *o = cJSON_AddObjectToObject(root, "air");
    cJSON_AddNumberToObject(o, "aqi", d->air.aqi);
    cJSON_AddStringToObject(o, "aqi_label", air_aqi_label(d->air.aqi));
    cJSON *pollen = cJSON_AddObjectToObject(o, "pollen");
    for (int i = 0; i < POLLEN_COUNT; i++) cJSON_AddNumberToObject(pollen, POLLEN_KEY[i], d->air.pollen[i]);
}

// null while unknown (the last fetch failed), count 0 when there's none.
static void add_warning(cJSON *root, const screen_data_t *d)
{
    if (!d->alerts_ok) {
        cJSON_AddNullToObject(root, "warning");
        return;
    }
    const alerts_t *a = &d->alerts;
    cJSON *o = cJSON_AddObjectToObject(root, "warning");
    cJSON_AddNumberToObject(o, "count", a->count);
    if (a->count == 0) return;
    cJSON_AddStringToObject(o, "event", a->event);
    int sev = (a->severity >= ALERT_MINOR && a->severity <= ALERT_EXTREME) ? (int)a->severity : 0;
    cJSON_AddStringToObject(o, "severity", SEVERITY[sev]);
    cJSON_AddBoolToObject(o, "started", a->started);
    cJSON_AddStringToObject(o, "onset", a->onset);
}

static void add_holiday(cJSON *root, const screen_data_t *d)
{
    int i = d->today_ymd ? holidays_next(&d->holidays, d->today_ymd) : -1;
    if (i < 0) {
        cJSON_AddNullToObject(root, "next_holiday");
        return;
    }
    const holiday_t *h = &d->holidays.day[i];
    char date[16];
    snprintf(date, sizeof(date), "%04d-%02d-%02d", h->ymd / 10000, h->ymd / 100 % 100, h->ymd % 100);
    cJSON *o = cJSON_AddObjectToObject(root, "next_holiday");
    cJSON_AddStringToObject(o, "date", date);
    cJSON_AddStringToObject(o, "name", h->name);
}

// Departures still ahead; in_min >= walk_min means it can be caught on foot.
static void add_bvg(cJSON *root, const screen_data_t *d, int now_min)
{
    if (!d->bvg_ok) {
        cJSON_AddNullToObject(root, "bvg");
        return;
    }
    cJSON *o = cJSON_AddObjectToObject(root, "bvg");
    cJSON_AddNumberToObject(o, "walk_min", d->bvg_walk_min);
    cJSON_AddNumberToObject(o, "walk_comfort", d->bvg_walk_comfort);
    cJSON *list = cJSON_AddArrayToObject(o, "departures");
    for (int i = 0; i < d->bvg.count; i++) {
        const bvg_departure_t *dep = &d->bvg.dep[i];
        int mins = screen_minutes_until(now_min, dep->hour, dep->minute);
        if (mins < 0) continue;
        char hhmm[16];
        snprintf(hhmm, sizeof(hhmm), "%02d:%02d", dep->hour, dep->minute);
        cJSON *e = cJSON_CreateObject();
        cJSON_AddStringToObject(e, "line", dep->line);
        cJSON_AddStringToObject(e, "direction", dep->direction);
        cJSON_AddStringToObject(e, "time", hhmm);
        cJSON_AddNumberToObject(e, "in_min", mins);
        cJSON_AddItemToArray(list, e);
    }
}

int web_status_json(const screen_data_t *d, const web_view_t *v, char *buf, size_t len)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return -1;
    cJSON_AddStringToObject(root, "time", v->time);
    cJSON_AddStringToObject(root, "date", v->date);
    const char *name = screen_name(v->screen);
    cJSON_AddStringToObject(root, "screen", name ? name : "");
    cJSON_AddBoolToObject(root, "panel_on", v->panel_on);
    add_outdoor(root, d);
    add_indoor(root, d);
    add_co2(root, d);
    add_air(root, d);
    add_warning(root, d);
    add_holiday(root, d);
    add_bvg(root, d, v->now_min);

    bool ok = len <= INT_MAX && cJSON_PrintPreallocated(root, buf, (int)len, false);
    cJSON_Delete(root);
    return ok ? (int)strlen(buf) : -1;
}

bool web_panel_command(const char *value, input_event_t *out)
{
    if (strcmp(value, "on") == 0) *out = (input_event_t){.type = INPUT_PANEL, .arg = 1};
    else if (strcmp(value, "off") == 0) *out = (input_event_t){.type = INPUT_PANEL, .arg = 0};
    else if (strcmp(value, "toggle") == 0) *out = (input_event_t){.type = INPUT_PRESS};
    else return false;
    return true;
}

bool web_screen_command(const char *value, input_event_t *out)
{
    if (strcmp(value, "next") == 0) {
        *out = (input_event_t){.type = INPUT_CW};
        return true;
    }
    if (strcmp(value, "prev") == 0) {
        *out = (input_event_t){.type = INPUT_CCW};
        return true;
    }
    int screen = screen_from_name(value);
    if (screen < 0) return false;
    *out = (input_event_t){.type = INPUT_SCREEN, .arg = screen};
    return true;
}
