#include <stdlib.h>
#include <string.h>
#include "weather.h"
#include "http.h"

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
