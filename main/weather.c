#include <string.h>
#include "weather.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

static const char *TAG = "weather";

// Hardcoded location (CLAUDE.md: no GPS) -- central Berlin. timezone makes
// sunrise/sunset and the hourly times come back in local time, DST included.
// Hourly data starts at the current hour.
#define STR_(x) #x
#define STR(x)  STR_(x)
#define WEATHER_URL                                                                               \
    "https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.405&current_weather=true" \
    "&daily=sunrise,sunset,uv_index_max&hourly=precipitation_probability"                         \
    "&forecast_hours=" STR(WEATHER_RAIN_HOURS) "&timezone=Europe%2FBerlin&forecast_days=1"

#define RESPONSE_BUF_SIZE 2048

static char s_response[RESPONSE_BUF_SIZE];
static int s_response_len;

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

    return weather_parse(s_response, out) ? ESP_OK : ESP_FAIL;
}
