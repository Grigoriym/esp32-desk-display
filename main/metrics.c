#include <string.h>
#include "metrics.h"
#include "metrics_secrets.h"
#include "backlog_ring.h"
#include "wifi.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "metrics";

// Plain HTTP: InfluxDB on the home LAN, see server/README.md. The port is
// the host port in server/docker-compose.yml.
#define METRICS_PORT "34898"
#define METRICS_URL  "http://" METRICS_HOST ":" METRICS_PORT "/api/v2/write?org=desk&bucket=desk&precision=s"

#define BACKLOG_PARTITION "backlog" // see partitions.csv
#define BACKLOG_BATCH     12
#define BACKLOG_PAUSE_MS  200 // between batches, while catching up
#define BODY_SIZE         (BACKLOG_BATCH * METRICS_RECORD_TEXT_MAX)

typedef enum {
    POST_OK,
    POST_RETRY,    // server away, or says no for now (wrong token): keep the data
    POST_REJECTED, // server took the request and refuses this data: retrying can't help
} post_result_t;

static TaskHandle_t s_task;
static SemaphoreHandle_t s_lock; // guards s_pending
static metrics_record_t s_pending;

// Only the upload task touches these.
static char s_body[BODY_SIZE];
static const esp_partition_t *s_partition;
static backlog_ring_t s_ring;
static bool s_ring_ok;
static bool s_server_up;             // the last POST got an answer
static bool s_backlog_unsent = true; // unknown after a boot, so look once

static post_result_t post(const char *body)
{
    esp_http_client_config_t config = {
        .url = METRICS_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return POST_RETRY;
    esp_http_client_set_header(client, "Authorization", "Token " METRICS_TOKEN);
    esp_http_client_set_header(client, "Content-Type", "text/plain; charset=utf-8");
    esp_http_client_set_post_field(client, body, (int)strlen(body));
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (err != ESP_OK) return POST_RETRY;
    if (status == 204) return POST_OK;
    ESP_LOGW(TAG, "unexpected HTTP status %d", status);
    // 400/422: InfluxDB refuses this data (e.g. past the bucket's retention),
    // and would refuse it forever. Anything else (401, 5xx) may clear up.
    return status == 400 || status == 422 ? POST_REJECTED : POST_RETRY;
}

static int flash_read(void *ctx, uint32_t addr, void *buf, size_t len)
{
    (void)ctx;
    return esp_partition_read(s_partition, addr, buf, len) == ESP_OK ? 0 : -1;
}

static int flash_write(void *ctx, uint32_t addr, const void *buf, size_t len)
{
    (void)ctx;
    return esp_partition_write(s_partition, addr, buf, len) == ESP_OK ? 0 : -1;
}

static int flash_erase(void *ctx, uint32_t addr)
{
    (void)ctx;
    return esp_partition_erase_range(s_partition, addr, BACKLOG_SECTOR_SIZE) == ESP_OK ? 0 : -1;
}

static void backlog_open(void)
{
    s_partition =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, BACKLOG_PARTITION);
    if (!s_partition) {
        ESP_LOGW(TAG, "no backlog partition: readings the server misses are lost");
        return;
    }
    const backlog_flash_t flash = {.read = flash_read, .write = flash_write, .erase_sector = flash_erase};
    s_ring_ok = backlog_ring_open(&s_ring, &flash, s_partition->size / BACKLOG_SECTOR_SIZE) == 0;
    if (!s_ring_ok) ESP_LOGW(TAG, "backlog unreadable");
}

// A record without a time can't be placed on the dashboard later.
static void backlog_store(const metrics_record_t *record)
{
    if (!s_ring_ok || record->time_utc == 0) return;
    if (backlog_ring_append(&s_ring, record) != 0) {
        ESP_LOGW(TAG, "backlog write failed");
        return;
    }
    s_backlog_unsent = true;
}

static void upload_latest(void)
{
    static metrics_record_t record;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    record = s_pending;
    xSemaphoreGive(s_lock);

    post_result_t result = POST_RETRY;
    if (wifi_is_connected() && metrics_format(&record, s_body, sizeof(s_body)) >= 0) result = post(s_body);
    bool up = result != POST_RETRY;
    // Log changes only: a line a minute while the server is down is noise.
    static bool first = true;
    if (first || up != s_server_up) {
        if (up) ESP_LOGI(TAG, "upload OK");
        else ESP_LOGW(TAG, "upload failed, saving readings to flash until the server is back");
    }
    first = false;
    s_server_up = up;
    if (!up) backlog_store(&record);
}

// One POST of the oldest unsent records. Stops at the first failure: what
// isn't marked sent stays for next time. A repeat after a power cut between
// POST and mark overwrites the same points on the server, so it's harmless.
static void send_backlog_batch(void)
{
    static metrics_record_t records[BACKLOG_BATCH];
    int count = s_ring_ok ? backlog_ring_read(&s_ring, records, BACKLOG_BATCH) : 0;
    if (count == 0) {
        if (s_backlog_unsent) ESP_LOGI(TAG, "backlog empty");
        s_backlog_unsent = false;
        return;
    }
    size_t used = 0;
    int formatted = 0;
    while (formatted < count) {
        int n = metrics_format(&records[formatted], s_body + used, sizeof(s_body) - used);
        if (n < 0) break;
        used += (size_t)n;
        formatted++;
    }
    post_result_t result = post(s_body);
    if (result == POST_RETRY) {
        s_server_up = false;
        return;
    }
    if (result == POST_REJECTED) ESP_LOGW(TAG, "server refused %d saved readings, dropping them", formatted);
    else ESP_LOGI(TAG, "sent %d saved readings, first at %lu", formatted, (unsigned long)records[0].time_utc);
    if (backlog_ring_mark_sent(&s_ring, formatted) != 0) ESP_LOGW(TAG, "backlog mark failed");
}

static void metrics_task(void *arg)
{
    backlog_open();
    for (;;) {
        // Catching up keeps going between minutes; otherwise wait for the next.
        bool catching_up = s_server_up && s_ring_ok && s_backlog_unsent;
        TickType_t wait = catching_up ? pdMS_TO_TICKS(BACKLOG_PAUSE_MS) : portMAX_DELAY;
        if (ulTaskNotifyTake(pdTRUE, wait)) upload_latest();
        if (s_server_up && s_ring_ok && s_backlog_unsent) send_backlog_batch();
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

void metrics_submit(const metrics_record_t *record)
{
    if (!s_task) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_pending = *record;
    xSemaphoreGive(s_lock);
    xTaskNotifyGive(s_task);
}
