#include <string.h>
#include "metrics.h"
#include "metrics_secrets.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "metrics";

// Plain HTTP: InfluxDB on the home LAN, see server/README.md.
#define METRICS_URL "http://" METRICS_HOST ":8086/api/v2/write?org=desk&bucket=desk&precision=s"
#define BATCH_SIZE  512

static TaskHandle_t s_task;
static SemaphoreHandle_t s_lock; // guards s_pending
static char s_pending[BATCH_SIZE];

static esp_err_t post(const char *body)
{
    esp_http_client_config_t config = {
        .url = METRICS_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_ERR_NO_MEM;
    esp_http_client_set_header(client, "Authorization", "Token " METRICS_TOKEN);
    esp_http_client_set_header(client, "Content-Type", "text/plain; charset=utf-8");
    esp_http_client_set_post_field(client, body, (int)strlen(body));
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (err != ESP_OK) return err;
    if (status != 204) {
        ESP_LOGW(TAG, "unexpected HTTP status %d", status);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void metrics_task(void *arg)
{
    static char batch[BATCH_SIZE];
    bool first = true;
    esp_err_t last = ESP_OK;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        xSemaphoreTake(s_lock, portMAX_DELAY);
        memcpy(batch, s_pending, sizeof(batch));
        xSemaphoreGive(s_lock);

        esp_err_t err = post(batch);
        // Log changes only: a line a minute while the server is down is noise.
        if (first || err != last) {
            if (err == ESP_OK) ESP_LOGI(TAG, "upload OK");
            else ESP_LOGW(TAG, "upload failed: %s", esp_err_to_name(err));
        }
        first = false;
        last = err;
    }
}

esp_err_t metrics_start(void)
{
    if (METRICS_HOST[0] == '\0') return ESP_ERR_NOT_SUPPORTED;
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;
    if (xTaskCreate(metrics_task, "metrics", 4096, NULL, 3, &s_task) != pdPASS) return ESP_ERR_NO_MEM;
    ESP_LOGI(TAG, "uploading to %s", METRICS_HOST);
    return ESP_OK;
}

void metrics_submit(const char *lines)
{
    if (!s_task) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    strlcpy(s_pending, lines, sizeof(s_pending));
    xSemaphoreGive(s_lock);
    xTaskNotifyGive(s_task);
}
