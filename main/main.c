#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "display.h"
#include "wifi.h"
#include "clock.h"
#include "weather.h"

static const char *TAG = "desk_display";

#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_PORT     I2C_NUM_0

// NTP can transiently fail right after WiFi comes up (DNS/AP not fully
// settled yet). Retry with backoff instead of ESP_ERROR_CHECK-ing straight
// into a crash/reboot loop over what's usually a few-second hiccup.
static esp_err_t ntp_sync_with_retry(void)
{
    const int max_attempts = 5;
    int backoff_s = 2;
    for (int attempt = 1; attempt <= max_attempts; attempt++) {
        esp_err_t err = clock_sync_time();
        if (err == ESP_OK) return ESP_OK;
        ESP_LOGW(TAG, "NTP sync attempt %d/%d failed: %s", attempt, max_attempts, esp_err_to_name(err));
        if (attempt < max_attempts) {
            vTaskDelay(pdMS_TO_TICKS(backoff_s * 1000));
            backoff_s *= 2;
        }
    }
    return ESP_FAIL;
}

void app_main(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    ESP_ERROR_CHECK(display_init(bus));
    ESP_ERROR_CHECK(display_clear());

    ESP_LOGI(TAG, "milestone 1 done: OLED init OK");

    ESP_ERROR_CHECK(wifi_connect());
    ESP_LOGI(TAG, "milestone 2 done: WiFi station connected");

    if (ntp_sync_with_retry() == ESP_OK) {
        ESP_LOGI(TAG, "milestone 3: NTP synced, rendering clock");
    } else {
        ESP_LOGE(TAG, "NTP sync failed after retries, continuing with unsynced clock");
    }

    char weather_str[8] = "--C";
    int weather_code = 3; // default to "cloud" icon until first fetch succeeds
    esp_err_t werr = weather_fetch(weather_str, sizeof(weather_str), &weather_code);
    if (werr == ESP_OK) {
        ESP_LOGI(TAG, "milestone 4 done: weather fetched (%s)", weather_str);
    } else {
        ESP_LOGW(TAG, "weather fetch failed: %s", esp_err_to_name(werr));
    }
    ESP_ERROR_CHECK(display_draw_icon_and_text(5, weather_icon_for_code(weather_code), weather_str));

#define WEATHER_REFRESH_SECONDS (15 * 60)
#define WEATHER_RETRY_START_SECONDS 30
    int seconds_since_weather = 0;
    // On failure, retry sooner than the normal cadence and back off toward
    // it, instead of leaving a stale reading up for a full 15 minutes.
    int next_weather_interval = (werr == ESP_OK) ? WEATHER_REFRESH_SECONDS : WEATHER_RETRY_START_SECONDS;
    char time_str[6];
    for (;;) {
        clock_format_now(time_str, sizeof(time_str));
        ESP_ERROR_CHECK(display_draw_text(2, time_str));

        if (++seconds_since_weather >= next_weather_interval) {
            seconds_since_weather = 0;
            werr = weather_fetch(weather_str, sizeof(weather_str), &weather_code);
            if (werr == ESP_OK) {
                ESP_LOGI(TAG, "weather refreshed (%s)", weather_str);
                display_draw_icon_and_text(5, weather_icon_for_code(weather_code), weather_str);
                next_weather_interval = WEATHER_REFRESH_SECONDS;
            } else {
                ESP_LOGW(TAG, "weather refresh failed: %s", esp_err_to_name(werr));
                next_weather_interval = (next_weather_interval < WEATHER_REFRESH_SECONDS)
                    ? next_weather_interval * 2 : WEATHER_REFRESH_SECONDS;
                if (next_weather_interval > WEATHER_REFRESH_SECONDS) {
                    next_weather_interval = WEATHER_REFRESH_SECONDS;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
