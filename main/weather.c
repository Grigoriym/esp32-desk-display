#include <stdlib.h>
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

// Same spot, current European AQI + pollen (Open-Meteo's air-quality API,
// free, no key; pollen is Europe-only and 0 off season).
#define AIR_URL                                                                                  \
    "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=52.52&longitude=13.405"      \
    "&current=european_aqi,alder_pollen,birch_pollen,grass_pollen,mugwort_pollen,ragweed_pollen" \
    "&timezone=Europe%2FBerlin"

// DWD warnings for the same spot, via Bright Sky (free, no key). tz makes
// the onset times local.
#define ALERTS_URL "https://api.brightsky.dev/alerts?lat=52.52&lon=13.405&tz=Europe%2FBerlin"

#define RESPONSE_BUF_SIZE 2048
// Each warning carries German and English texts, ~1 KB each; this fits
// more than ten, so it's heap-allocated for the fetch only.
#define ALERTS_BUF_SIZE (16 * 1024)

static char s_response[RESPONSE_BUF_SIZE];

typedef struct {
    char *buf;
    int size;
    int len;
} response_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    response_t *r = evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        int copy_len = evt->data_len;
        if (r->len + copy_len >= r->size) {
            copy_len = r->size - r->len - 1;
        }
        if (copy_len > 0) {
            memcpy(r->buf + r->len, evt->data, copy_len);
            r->len += copy_len;
        }
    }
    return ESP_OK;
}

// GETs url into buf (NUL-terminated, cut at size - 1 bytes); ESP_OK only on
// HTTP 200.
static esp_err_t http_get(const char *url, char *buf, int size)
{
    response_t r = {.buf = buf, .size = size};
    memset(buf, 0, size);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = &r,
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
    if (r.len == size - 1) ESP_LOGW(TAG, "response cut at %d bytes", size - 1);
    return ESP_OK;
}

esp_err_t weather_fetch(weather_t *out)
{
    esp_err_t err = http_get(WEATHER_URL, s_response, sizeof(s_response));
    if (err != ESP_OK) return err;
    return weather_parse(s_response, out) ? ESP_OK : ESP_FAIL;
}

esp_err_t air_fetch(air_t *out)
{
    esp_err_t err = http_get(AIR_URL, s_response, sizeof(s_response));
    if (err != ESP_OK) return err;
    return air_parse(s_response, out) ? ESP_OK : ESP_FAIL;
}

esp_err_t alerts_fetch(const char *now_local, alerts_t *out)
{
    char *buf = malloc(ALERTS_BUF_SIZE);
    if (!buf) return ESP_ERR_NO_MEM;
    esp_err_t err = http_get(ALERTS_URL, buf, ALERTS_BUF_SIZE);
    if (err == ESP_OK && !alerts_parse(buf, now_local, out)) err = ESP_FAIL;
    free(buf);
    return err;
}
