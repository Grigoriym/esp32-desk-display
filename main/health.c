#include "health.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "health";

// A TLS handshake (weather, BVG) needs roughly 40 KB of heap at once.
#define HEAP_WARN_BYTES 40000
// Headroom below this on any task stack is one bigger JSON away from a
// stack overflow reboot (seen 2026-09-24 on the first BVG fetch).
#define STACK_WARN_BYTES 1024

// Tasks this firmware creates, plus the system event task that runs its
// WiFi/IP event handler. Names as passed to xTaskCreate().
static const char *const TASKS[] = {"bvg", "enc_button", "sys_evt"};

static void log_stack(const char *name, TaskHandle_t task)
{
    // ESP-IDF's FreeRTOS reports this in bytes (upstream: words).
    UBaseType_t headroom = uxTaskGetStackHighWaterMark(task);
    if (headroom < STACK_WARN_BYTES) {
        ESP_LOGW(TAG, "stack %s: only %u bytes left at its worst -- raise its stack size", name,
                 (unsigned)headroom);
    } else {
        ESP_LOGI(TAG, "stack %s: %u bytes left at its worst", name, (unsigned)headroom);
    }
}

void health_log(void)
{
    size_t free_now = esp_get_free_heap_size();
    size_t free_min = esp_get_minimum_free_heap_size();
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    if (free_min < HEAP_WARN_BYTES) {
        ESP_LOGW(TAG, "heap low: %u free, %u at its lowest since boot, largest block %u -- leak?",
                 (unsigned)free_now, (unsigned)free_min, (unsigned)largest);
    } else {
        ESP_LOGI(TAG, "heap %u free, %u at its lowest since boot, largest block %u", (unsigned)free_now,
                 (unsigned)free_min, (unsigned)largest);
    }

    log_stack(pcTaskGetName(NULL), NULL); // the calling (main) task
    for (size_t i = 0; i < sizeof(TASKS) / sizeof(TASKS[0]); i++) {
        TaskHandle_t task = xTaskGetHandle(TASKS[i]);
        if (task) log_stack(TASKS[i], task);
    }
}
