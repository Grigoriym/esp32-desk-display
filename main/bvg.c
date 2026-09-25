#include <string.h>
#include "bvg.h"
#include "bvg_secrets.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "bvg";

#define BVG_URL                                                                                  \
    "https://v6.bvg.transport.rest/stops/" BVG_STOP_ID "/departures?direction=" BVG_DIRECTION_ID \
    "&duration=60&results=6&" BVG_PRODUCTS "&remarks=false&linesOfStops=false&pretty=false"

// ~1.2 KB of JSON per departure, 6 requested.
#define RESPONSE_BUF_SIZE 12288

static char s_response[RESPONSE_BUF_SIZE];
static int s_response_len;
static bool s_truncated;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        int copy_len = evt->data_len;
        if (s_response_len + copy_len >= RESPONSE_BUF_SIZE) {
            copy_len = RESPONSE_BUF_SIZE - s_response_len - 1;
            s_truncated = true;
        }
        if (copy_len > 0) {
            memcpy(s_response + s_response_len, evt->data, copy_len);
            s_response_len += copy_len;
        }
    }
    return ESP_OK;
}

static esp_err_t bvg_fetch(bvg_departures_t *out)
{
    s_response_len = 0;
    s_truncated = false;
    memset(s_response, 0, sizeof(s_response));

    esp_http_client_config_t config = {
        .url = BVG_URL,
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
    if (s_truncated) {
        ESP_LOGW(TAG, "response over %d bytes, truncated", RESPONSE_BUF_SIZE);
        return ESP_ERR_NO_MEM;
    }

    return bvg_parse(s_response, out) ? ESP_OK : ESP_FAIL;
}

#define BVG_REFRESH_MS (60 * 1000)

static TaskHandle_t s_task;
static SemaphoreHandle_t s_lock; // guards everything below
static volatile bool s_active;
static bool s_updated, s_have_data, s_failed;
static bvg_departures_t s_latest;

static void bvg_task(void *arg)
{
    TickType_t last_fetch = 0;
    bool fetched_once = false;
    for (;;) {
        if (!s_active) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // woken by bvg_set_active(true)
            continue;
        }
        if (fetched_once && xTaskGetTickCount() - last_fetch < pdMS_TO_TICKS(BVG_REFRESH_MS)) {
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
            continue;
        }

        bvg_departures_t deps;
        esp_err_t err = bvg_fetch(&deps);
        last_fetch = xTaskGetTickCount();
        fetched_once = true;
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "%d departure(s), first %s %s %02d:%02d", deps.count,
                     deps.count ? deps.dep[0].line : "-", deps.count ? deps.dep[0].direction : "-",
                     deps.count ? deps.dep[0].hour : 0, deps.count ? deps.dep[0].minute : 0);
        } else {
            ESP_LOGW(TAG, "fetch failed: %s", esp_err_to_name(err));
        }

        xSemaphoreTake(s_lock, portMAX_DELAY);
        if (err == ESP_OK) {
            s_latest = deps;
            s_have_data = true;
        }
        s_failed = (err != ESP_OK);
        s_updated = true;
        xSemaphoreGive(s_lock);
    }
}

esp_err_t bvg_start(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;
    // TLS handshake + cJSON parse of a ~5-12 KB response: same budget the
    // main task needed (see sdkconfig.defaults).
    if (xTaskCreate(bvg_task, "bvg", 8192, NULL, 4, &s_task) != pdPASS) return ESP_ERR_NO_MEM;
    return ESP_OK;
}

void bvg_set_active(bool active)
{
    bool was_active = s_active;
    s_active = active;
    if (active && !was_active && s_task) xTaskNotifyGive(s_task);
}

bool bvg_take_update(bvg_departures_t *out, bool *have_data, bool *failed)
{
    if (!s_lock) return false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool updated = s_updated;
    if (updated) {
        *out = s_latest;
        *have_data = s_have_data;
        *failed = s_failed;
        s_updated = false;
    }
    xSemaphoreGive(s_lock);
    return updated;
}
