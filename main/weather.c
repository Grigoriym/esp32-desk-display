#include <stdio.h>
#include <string.h>
#include <math.h>
#include "weather.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "weather";

// Hardcoded location (CLAUDE.md: no GPS) -- central Berlin. timezone makes
// sunrise/sunset come back in local time, DST included.
#define WEATHER_URL \
    "https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.405&current_weather=true" \
    "&daily=sunrise,sunset,uv_index_max&timezone=Europe%2FBerlin&forecast_days=1"

#define RESPONSE_BUF_SIZE 2048

static char s_response[RESPONSE_BUF_SIZE];
static int s_response_len;

// 8x8 icons, column-major (bit0 = top row). Rough pixel art -- not exact,
// good enough to distinguish sun/cloud/rain/snow/storm at a glance.
static const uint8_t icon_sun[8]   = {0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C};
static const uint8_t icon_cloud[8] = {0x18, 0x1C, 0x1E, 0x1E, 0x1E, 0x1E, 0x1C, 0x18};
static const uint8_t icon_rain[8]  = {0x18, 0x3C, 0x5E, 0x3E, 0x5E, 0x3E, 0x5C, 0x18};
static const uint8_t icon_snow[8]  = {0x58, 0xBC, 0x5E, 0xBE, 0x5E, 0xBE, 0x5C, 0x18};
static const uint8_t icon_storm[8] = {0x18, 0x9C, 0xDE, 0x7E, 0x3E, 0x1E, 0x1C, 0x18};

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        int copy_len = evt->data_len;
        if (s_response_len + copy_len >= RESPONSE_BUF_SIZE) {
            copy_len = RESPONSE_BUF_SIZE - s_response_len - 1;
        }
        if (copy_len > 0) {
            memcpy(s_response + s_response_len, evt->data, copy_len);
            s_response_len += copy_len;
        }
    }
    return ESP_OK;
}

esp_err_t weather_fetch(weather_t *out)
{
    s_response_len = 0;
    memset(s_response, 0, sizeof(s_response));

    esp_http_client_config_t config = {
        .url = WEATHER_URL,
        .event_handler = http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) return err;
    if (status != 200) {
        ESP_LOGW(TAG, "unexpected HTTP status %d", status);
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(s_response);
    if (!root) return ESP_FAIL;

    cJSON *current = cJSON_GetObjectItem(root, "current_weather");
    cJSON *temp = current ? cJSON_GetObjectItem(current, "temperature") : NULL;
    cJSON *code = current ? cJSON_GetObjectItem(current, "weathercode") : NULL;
    cJSON *wind = current ? cJSON_GetObjectItem(current, "windspeed") : NULL;
    cJSON *daily = cJSON_GetObjectItem(root, "daily");
    // Daily fields are one-element arrays (forecast_days=1).
    cJSON *sunrise = daily ? cJSON_GetArrayItem(cJSON_GetObjectItem(daily, "sunrise"), 0) : NULL;
    cJSON *sunset = daily ? cJSON_GetArrayItem(cJSON_GetObjectItem(daily, "sunset"), 0) : NULL;
    cJSON *uv = daily ? cJSON_GetArrayItem(cJSON_GetObjectItem(daily, "uv_index_max"), 0) : NULL;
    if (!cJSON_IsNumber(temp) || !cJSON_IsNumber(code) || !cJSON_IsNumber(wind)
        || !cJSON_IsString(sunrise) || !cJSON_IsString(sunset) || !cJSON_IsNumber(uv)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // Sunrise/sunset look like "2026-09-23T06:53" -- keep the "HH:MM" after 'T'.
    const char *rise_t = strchr(sunrise->valuestring, 'T');
    const char *set_t = strchr(sunset->valuestring, 'T');
    if (!rise_t || !set_t) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    out->temp_c = (int)lround(temp->valuedouble);
    out->weather_code = code->valueint;
    out->wind_kmh = (int)lround(wind->valuedouble);
    out->uv_max = (int)lround(uv->valuedouble);
    snprintf(out->sunrise, sizeof(out->sunrise), "%.5s", rise_t + 1);
    snprintf(out->sunset, sizeof(out->sunset), "%.5s", set_t + 1);

    cJSON_Delete(root);
    return ESP_OK;
}

const uint8_t *weather_icon_for_code(int weather_code)
{
    // WMO weather codes, as used by Open-Meteo's "weathercode" field.
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
}
