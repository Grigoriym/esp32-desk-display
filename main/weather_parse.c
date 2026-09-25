#include <stdio.h>
#include <string.h>
#include <math.h>
#include "weather_parse.h"
#include "cJSON.h"

// 8x8 icons, column-major (bit0 = top row). Rough pixel art -- not exact,
// good enough to distinguish sun/cloud/rain/snow/storm at a glance.
static const uint8_t icon_sun[8] = {0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C};
static const uint8_t icon_cloud[8] = {0x18, 0x1C, 0x1E, 0x1E, 0x1E, 0x1E, 0x1C, 0x18};
static const uint8_t icon_rain[8] = {0x18, 0x3C, 0x5E, 0x3E, 0x5E, 0x3E, 0x5C, 0x18};
static const uint8_t icon_snow[8] = {0x58, 0xBC, 0x5E, 0xBE, 0x5E, 0xBE, 0x5C, 0x18};
static const uint8_t icon_storm[8] = {0x18, 0x9C, 0xDE, 0x7E, 0x3E, 0x1E, 0x1C, 0x18};

// "2026-09-25T14:00" -> "14:00"; false without the 'T'.
static bool copy_hhmm(const cJSON *iso, char *dst, size_t dst_len)
{
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference): cJSON_IsString(NULL) is false (see below)
    const char *t = cJSON_IsString(iso) ? strchr(iso->valuestring, 'T') : NULL;
    if (!t) return false;
    snprintf(dst, dst_len, "%.5s", t + 1);
    return true;
}

// Finds the first rainy hour in the hourly arrays and the first dry one after
// it. False if the arrays are missing, differ in length or hold junk.
static bool parse_rain(const cJSON *hourly, weather_t *w)
{
    const cJSON *times = cJSON_GetObjectItem(hourly, "time");
    const cJSON *probs = cJSON_GetObjectItem(hourly, "precipitation_probability");
    int n = cJSON_GetArraySize(times);
    if (!cJSON_IsArray(times) || !cJSON_IsArray(probs) || n == 0 || n != cJSON_GetArraySize(probs))
        return false;

    w->rain_in_h = -1;
    w->rain_from[0] = '\0';
    w->rain_until[0] = '\0';
    for (int i = 0; i < n; i++) {
        const cJSON *prob = cJSON_GetArrayItem(probs, i);
        if (!cJSON_IsNull(prob) && !cJSON_IsNumber(prob)) return false;
        // null = no data for that hour: counts as dry.
        bool rainy = cJSON_IsNumber(prob) && prob->valuedouble >= WEATHER_RAIN_MIN_PROB;
        const cJSON *time = cJSON_GetArrayItem(times, i);
        if (w->rain_in_h < 0 && rainy) {
            if (!copy_hhmm(time, w->rain_from, sizeof(w->rain_from))) return false;
            w->rain_in_h = i;
        } else if (w->rain_in_h >= 0 && !rainy) {
            return copy_hhmm(time, w->rain_until, sizeof(w->rain_until));
        }
    }
    return true;
}

bool weather_parse(const char *json, weather_t *out)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;

    cJSON *current = cJSON_GetObjectItem(root, "current_weather");
    cJSON *temp = current ? cJSON_GetObjectItem(current, "temperature") : NULL;
    cJSON *code = current ? cJSON_GetObjectItem(current, "weathercode") : NULL;
    cJSON *wind = current ? cJSON_GetObjectItem(current, "windspeed") : NULL;
    cJSON *daily = cJSON_GetObjectItem(root, "daily");
    // Daily fields are one-element arrays (forecast_days=1).
    cJSON *sunrise = daily ? cJSON_GetArrayItem(cJSON_GetObjectItem(daily, "sunrise"), 0) : NULL;
    cJSON *sunset = daily ? cJSON_GetArrayItem(cJSON_GetObjectItem(daily, "sunset"), 0) : NULL;
    cJSON *uv = daily ? cJSON_GetArrayItem(cJSON_GetObjectItem(daily, "uv_index_max"), 0) : NULL;
    cJSON *hourly = cJSON_GetObjectItem(root, "hourly");
    if (!cJSON_IsNumber(temp) || !cJSON_IsNumber(code) || !cJSON_IsNumber(wind) || !cJSON_IsNumber(uv)) {
        cJSON_Delete(root);
        return false;
    }

    // Filled into a copy so *out stays untouched on failure. Sunrise/sunset
    // look like "2026-09-23T06:53" -- keep the "HH:MM" after 'T'.
    weather_t w;
    bool ok = copy_hhmm(sunrise, w.sunrise, sizeof(w.sunrise))
              && copy_hhmm(sunset, w.sunset, sizeof(w.sunset)) && parse_rain(hourly, &w);
    if (ok) {
        // The analyzer can't see that cJSON_Is*(NULL) is false (cJSON.c is
        // another translation unit), so it flags the dereferences below.
        // NOLINTBEGIN(clang-analyzer-core.NullDereference)
        w.temp_c = (int)lround(temp->valuedouble);
        w.weather_code = code->valueint;
        w.wind_kmh = (int)lround(wind->valuedouble);
        w.uv_max = (int)lround(uv->valuedouble);
        // NOLINTEND(clang-analyzer-core.NullDereference)
        *out = w;
    }

    cJSON_Delete(root);
    return ok;
}

const uint8_t *weather_icon_for_code(int weather_code)
{
    // WMO weather codes, as used by Open-Meteo's "weathercode" field.
    // clang-format off: grouped by weather type, one family per line
    switch (weather_code) {
        case 0:
        case 1:
            return icon_sun;
        case 2:
        case 3:
        case 45:
        case 48:
            return icon_cloud;
        case 51: case 53: case 55: case 56: case 57:
        case 61: case 63: case 65: case 66: case 67:
        case 80: case 81: case 82:
            return icon_rain;
        case 71: case 73: case 75: case 77:
        case 85: case 86:
            return icon_snow;
        case 95: case 96: case 99:
            return icon_storm;
        default:
            return icon_cloud;
    }
    // clang-format on
}
