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
    if (!cJSON_IsNumber(temp) || !cJSON_IsNumber(code) || !cJSON_IsNumber(wind) || !cJSON_IsString(sunrise)
        || !cJSON_IsString(sunset) || !cJSON_IsNumber(uv)) {
        cJSON_Delete(root);
        return false;
    }

    // Sunrise/sunset look like "2026-09-23T06:53" -- keep the "HH:MM" after 'T'.
    const char *rise_t = strchr(sunrise->valuestring, 'T');
    const char *set_t = strchr(sunset->valuestring, 'T');
    if (!rise_t || !set_t) {
        cJSON_Delete(root);
        return false;
    }

    out->temp_c = (int)lround(temp->valuedouble);
    out->weather_code = code->valueint;
    out->wind_kmh = (int)lround(wind->valuedouble);
    out->uv_max = (int)lround(uv->valuedouble);
    snprintf(out->sunrise, sizeof(out->sunrise), "%.5s", rise_t + 1);
    snprintf(out->sunset, sizeof(out->sunset), "%.5s", set_t + 1);

    cJSON_Delete(root);
    return true;
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
