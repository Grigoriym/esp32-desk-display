#include <stdio.h>
#include <string.h>
#include "weather.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "weather";

// Hardcoded location (CLAUDE.md: no GPS) -- central Berlin.
#define WEATHER_URL \
    "https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.405&current_weather=true"

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

esp_err_t weather_fetch(char *temp_buf, size_t temp_buf_size, int *weather_code_out)
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
    if (!cJSON_IsNumber(temp) || !cJSON_IsNumber(code)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    int rounded = (int)(temp->valuedouble < 0 ? temp->valuedouble - 0.5 : temp->valuedouble + 0.5);
    snprintf(temp_buf, temp_buf_size, "%dC", rounded);
    if (weather_code_out) {
        *weather_code_out = code->valueint;
    }

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
