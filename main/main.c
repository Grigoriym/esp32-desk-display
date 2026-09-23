#include <stdio.h>
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

#define SPLASH_HOLD_SECONDS 3
#define SPLASH_PAGE_OLED    2
#define SPLASH_PAGE_RTC     3
#define SPLASH_PAGE_WIFI    4
#define SPLASH_PAGE_NTP     5
#define SPLASH_PAGE_WEATHER 6

// One "NAME    OK" row of the boot status screen. Every row is padded to the
// same width, so centering them also lines the status column up.
static void splash_status(int page, const char *name, const char *status)
{
    char line[16];
    snprintf(line, sizeof(line), "%-8s%s", name, status);
    ESP_ERROR_CHECK(display_draw_text(page, line));
}

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

    ESP_LOGI(TAG, "OLED init OK");

    // Boot status screen: lights the panel right away and shows each
    // subsystem coming up, instead of a blank screen for ~6-8s.
    ESP_ERROR_CHECK(display_draw_text(0, "HELLO"));
    splash_status(SPLASH_PAGE_OLED, "OLED", "OK"); // if this is visible, it works
    splash_status(SPLASH_PAGE_RTC, "RTC", "--");
    splash_status(SPLASH_PAGE_WIFI, "WIFI", "--");
    splash_status(SPLASH_PAGE_NTP, "NTP", "--");
    splash_status(SPLASH_PAGE_WEATHER, "WEATHER", "--");

    // RTC first, so the clock is right even if NTP fails later.
    esp_err_t rerr = clock_rtc_init(bus);
    if (rerr != ESP_OK) {
        ESP_LOGW(TAG, "RTC not used: %s", esp_err_to_name(rerr));
    }
    splash_status(SPLASH_PAGE_RTC, "RTC", rerr == ESP_OK ? "OK" : "NO");

    // Blocks until connected; the WIFI row stays at "--" meanwhile.
    ESP_ERROR_CHECK(wifi_connect());
    ESP_LOGI(TAG, "WiFi connected");
    splash_status(SPLASH_PAGE_WIFI, "WIFI", "OK");

    if (ntp_sync_with_retry() == ESP_OK) {
        ESP_LOGI(TAG, "NTP synced");
        splash_status(SPLASH_PAGE_NTP, "NTP", "OK");
    } else {
        ESP_LOGE(TAG, "NTP sync failed after retries, continuing with unsynced clock");
        splash_status(SPLASH_PAGE_NTP, "NTP", "NO");
    }

    char weather_str[8] = "--C";
    int weather_code = 3; // default to "cloud" icon until first fetch succeeds
    esp_err_t werr = weather_fetch(weather_str, sizeof(weather_str), &weather_code);
    if (werr == ESP_OK) {
        ESP_LOGI(TAG, "weather fetched (%s)", weather_str);
    } else {
        ESP_LOGW(TAG, "weather fetch failed: %s", esp_err_to_name(werr));
    }
    splash_status(SPLASH_PAGE_WEATHER, "WEATHER", werr == ESP_OK ? "OK" : "NO");

    // Leave the final status up long enough to actually read it.
    vTaskDelay(pdMS_TO_TICKS(SPLASH_HOLD_SECONDS * 1000));
    ESP_ERROR_CHECK(display_clear());
    ESP_ERROR_CHECK(display_draw_icon_and_text(5, weather_icon_for_code(weather_code), weather_str));

#define WEATHER_REFRESH_SECONDS (15 * 60)
#define WEATHER_RETRY_START_SECONDS 30
    int seconds_since_weather = 0;
    // On failure, retry sooner than the normal cadence and back off toward
    // it, instead of leaving a stale reading up for a full 15 minutes.
    char time_str[6];
    char date_str[11];
    int next_weather_interval = (werr == ESP_OK) ? WEATHER_REFRESH_SECONDS : WEATHER_RETRY_START_SECONDS;
    for (;;) {
        clock_format_now(time_str, sizeof(time_str));
        ESP_ERROR_CHECK(display_draw_text(1, time_str));
        clock_format_date(date_str, sizeof(date_str));
        ESP_ERROR_CHECK(display_draw_text(3, date_str));

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
