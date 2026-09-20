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

    ESP_ERROR_CHECK(clock_sync_time());
    ESP_LOGI(TAG, "milestone 3: NTP synced, rendering clock");

    char weather_str[8] = "--C";
    int weather_code = 3; // default to "cloud" icon until first fetch succeeds
    esp_err_t werr = weather_fetch(weather_str, sizeof(weather_str), &weather_code);
    if (werr == ESP_OK) {
        ESP_LOGI(TAG, "milestone 4 done: weather fetched (%s)", weather_str);
    } else {
        ESP_LOGW(TAG, "weather fetch failed: %s", esp_err_to_name(werr));
    }
    ESP_ERROR_CHECK(display_draw_icon(1, weather_icon_for_code(weather_code)));
    ESP_ERROR_CHECK(display_draw_text(5, weather_str));

#define WEATHER_REFRESH_SECONDS (15 * 60)
    int seconds_since_weather = 0;
    char time_str[6];
    for (;;) {
        clock_format_now(time_str, sizeof(time_str));
        ESP_ERROR_CHECK(display_draw_text(2, time_str));

        if (++seconds_since_weather >= WEATHER_REFRESH_SECONDS) {
            seconds_since_weather = 0;
            werr = weather_fetch(weather_str, sizeof(weather_str), &weather_code);
            if (werr == ESP_OK) {
                ESP_LOGI(TAG, "weather refreshed (%s)", weather_str);
                display_draw_icon(1, weather_icon_for_code(weather_code));
                display_draw_text(5, weather_str);
            } else {
                ESP_LOGW(TAG, "weather refresh failed: %s", esp_err_to_name(werr));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
