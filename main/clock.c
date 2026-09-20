#include <time.h>
#include "clock.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"

static const char *TAG = "clock";

// Hardcoded UTC offset (CLAUDE.md: no IP-geolocation for now), no DST
// handling -- Berlin is UTC+2 (CEST) until DST ends in late October, then
// UTC+1 (CET) until next spring. Update this by hand when it flips.
#define UTC_OFFSET_HOURS 2

esp_err_t clock_sync_time(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_err_t err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) return err;

    err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000));
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "NTP sync done");
    return ESP_OK;
}

void clock_format_now(char *buf, size_t buf_size)
{
    time_t now = time(NULL) + (time_t)(UTC_OFFSET_HOURS * 3600);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    strftime(buf, buf_size, "%H:%M", &timeinfo);
}
