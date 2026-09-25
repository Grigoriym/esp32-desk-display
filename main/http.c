#include <string.h>
#include "http.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

static const char *TAG = "http";

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

esp_err_t http_get(const char *url, char *buf, int size)
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
