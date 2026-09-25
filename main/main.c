#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "display.h"
#include "wifi.h"
#include "clock.h"
#include "weather.h"
#include "bme280.h"
#include "encoder.h"
#include "bvg.h"
#include "bvg_secrets.h"
#include <time.h>

static const char *TAG = "desk_display";

#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_PORT     I2C_NUM_0

#define WIFI_CONNECT_TIMEOUT_SECONDS 15
#define NTP_RETRY_SECONDS 60

// Boot status grid: two columns, local hardware on top, network below.
// Each item is a (page, column) cell; column 0 = left, 1 = right.
#define SPLASH_OLED    2, 0
#define SPLASH_PWR     2, 1
#define SPLASH_RTC     3, 0
#define SPLASH_BME     3, 1
#define SPLASH_WIFI    5, 0
#define SPLASH_NTP     5, 1
#define SPLASH_WEATHER 6, 0

// Main screens, cycled with the encoder (press = back to home). The clock
// row sits on page 0 of every screen; draw_screen() owns pages 1-7.
#define PAGE_CLOCK 0 // time left, date right
typedef enum { SCREEN_HOME, SCREEN_OUTDOOR, SCREEN_INDOOR, SCREEN_BVG, SCREEN_COUNT } screen_t;

// Latest data the screens draw from.
static struct {
    screen_t screen;
    bool weather_ok; // false until the first fetch succeeds
    weather_t weather;
    bool indoor_ok;  // false until the first BME280 read succeeds
    bme280_reading_t indoor;
    bool bvg_ok;     // false until the first departures fetch succeeds
    bool bvg_failed; // last fetch failed (shown only while there's no data)
    bvg_departures_t bvg;
} s_ui;

// Minutes from now until a departure at hh:mm local, negative once it's gone.
// Departures are at most an hour ahead, so wrapping over midnight is safe.
static int minutes_until(int hour, int minute)
{
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    int diff = (hour * 60 + minute) - (t.tm_hour * 60 + t.tm_min);
    if (diff > 12 * 60) diff -= 24 * 60;
    if (diff < -12 * 60) diff += 24 * 60;
    return diff;
}

// Everything that's allowed to answer on the I2C bus. Anything else showing
// up means an address clash or an unplanned module -- see "Power & bus
// budget" in CLAUDE.md before wiring a new one, then add it here.
static const struct { uint8_t addr; const char *name; } KNOWN_I2C[] = {
    { 0x3C, "OLED" },
    { 0x3D, "OLED (alt addr)" },
    { 0x57, "DS3231 EEPROM (AT24C32)" },
    { 0x5F, "DS3231 module extra addr" },
    { 0x68, "DS3231 RTC" },
    { 0x76, "BME280" },
    { 0x77, "BME280 (alt addr)" },
};

// One "NAME    OK" cell of the boot status grid. Every cell is padded to
// the same width, so the status column lines up within each column.
static void splash_status(int page, int col, const char *name, const char *status)
{
    static char cells[8][2][12];
    snprintf(cells[page][col], sizeof(cells[page][col]), "%-8s%s", name, status);
    ESP_ERROR_CHECK(display_draw_text_columns(page, cells[page][0], cells[page][1]));
}

static void draw_clock_row(void)
{
    char time_str[6];
    char date_str[11];
    clock_format_now(time_str, sizeof(time_str));
    clock_format_date(date_str, sizeof(date_str));
    display_draw_text_columns(PAGE_CLOCK, time_str, date_str);
}

// Redraws pages 1-7 for the current screen. Every page is written in full
// (unused ones blank), so switching screens needs no display_clear() and
// doesn't flicker. Missing data shows as "--".
static void draw_screen(void)
{
    // Font has no '%' or '.': humidity is "45H", pressure whole hPa.
    char rows[8][2][28] = {0}; // >21 chars (a full row) just clips at the edge
    const weather_t *w = &s_ui.weather;
    const bme280_reading_t *in = &s_ui.indoor;

    switch (s_ui.screen) {
    case SCREEN_HOME:
        if (s_ui.weather_ok) snprintf(rows[3][0], sizeof(rows[3][0]), "%dC", w->temp_c);
        else strcpy(rows[3][0], "--C");
        if (s_ui.indoor_ok) {
            snprintf(rows[5][0], sizeof(rows[5][0]), "IN %dC %dH",
                     (int)lroundf(in->temp_c), (int)lroundf(in->humidity_pct));
        }
        break;
    case SCREEN_OUTDOOR:
        strcpy(rows[2][0], "OUTDOOR");
        if (s_ui.weather_ok) {
            snprintf(rows[4][0], sizeof(rows[4][0]), "RISE %s", w->sunrise);
            snprintf(rows[4][1], sizeof(rows[4][1]), "SET %s", w->sunset);
            snprintf(rows[6][0], sizeof(rows[6][0]), "WIND %dKMH", w->wind_kmh);
            snprintf(rows[6][1], sizeof(rows[6][1]), "UV %d", w->uv_max);
        } else {
            strcpy(rows[4][0], "--");
        }
        break;
    case SCREEN_INDOOR:
        strcpy(rows[2][0], "INDOOR");
        if (s_ui.indoor_ok) {
            snprintf(rows[4][0], sizeof(rows[4][0]), "TEMP %dC", (int)lroundf(in->temp_c));
            snprintf(rows[4][1], sizeof(rows[4][1]), "HUM %dH", (int)lroundf(in->humidity_pct));
            snprintf(rows[6][0], sizeof(rows[6][0]), "%d HPA", (int)lroundf(in->pressure_hpa));
        } else {
            strcpy(rows[4][0], "--");
        }
        break;
    case SCREEN_BVG: {
        if (!s_ui.bvg_ok) {
            strcpy(rows[2][0], "BVG");
            strcpy(rows[4][0], s_ui.bvg_failed ? "NO DATA" : "LOADING");
            break;
        }
        // Title from the data, e.g. "U5 HAUPTBAHNHOF"; then the next trains
        // that can still be caught on foot, with a leave-by hint for the first.
        if (s_ui.bvg.count > 0) {
            snprintf(rows[2][0], sizeof(rows[2][0]), "%s %s", s_ui.bvg.dep[0].line, s_ui.bvg.dep[0].direction);
        } else {
            strcpy(rows[2][0], "BVG");
        }
        int row = 5;
        for (int i = 0; i < s_ui.bvg.count && row <= 7; i++) {
            const bvg_departure_t *d = &s_ui.bvg.dep[i];
            int mins = minutes_until(d->hour, d->minute);
            if (mins < BVG_WALK_MIN_MINUTES) continue;
            if (row == 5) {
                // Before the comfortable-walk point: countdown; at it: go;
                // after it (but still catchable): hurry.
                int leave_in = mins - BVG_WALK_COMFORT_MINUTES;
                if (leave_in > 0) snprintf(rows[3][0], sizeof(rows[3][0]), "LEAVE IN %d", leave_in);
                else if (leave_in == 0) strcpy(rows[3][0], "GO NOW");
                else strcpy(rows[3][0], "HURRY");
            }
            snprintf(rows[row][0], sizeof(rows[row][0]), "%02d:%02d", d->hour, d->minute);
            snprintf(rows[row][1], sizeof(rows[row][1]), "%d MIN", mins);
            row++;
        }
        if (row == 5) strcpy(rows[4][0], "NO TRAINS");
        break;
    }
    default:
        break;
    }

    for (int page = 1; page < 8; page++) {
        if (s_ui.screen == SCREEN_HOME && page == 3) {
            // Icon + outdoor temperature, centred together.
            display_draw_icon_and_text(page, weather_icon_for_code(s_ui.weather_ok ? w->weather_code : 3),
                                       rows[page][0]);
        } else if (rows[page][1][0]) {
            display_draw_text_columns(page, rows[page][0], rows[page][1]);
        } else {
            display_draw_text(page, rows[page][0]); // centred; "" blanks the page
        }
    }
}

// Logs every device on the bus and warns about any not in KNOWN_I2C.
// Missing required devices already show up as OLED/RTC failures.
static void i2c_bus_check(i2c_master_bus_handle_t bus)
{
    int found = 0;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c_master_probe(bus, addr, 20) != ESP_OK) continue;
        found++;
        const char *name = NULL;
        for (size_t i = 0; i < sizeof(KNOWN_I2C) / sizeof(KNOWN_I2C[0]); i++) {
            if (KNOWN_I2C[i].addr == addr) name = KNOWN_I2C[i].name;
        }
        if (name) {
            ESP_LOGI(TAG, "I2C 0x%02X: %s", addr, name);
        } else {
            ESP_LOGW(TAG, "I2C 0x%02X: UNKNOWN device -- address clash or unplanned module?", addr);
        }
    }
    ESP_LOGI(TAG, "I2C bus: %d address(es) answering", found);
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

    // A brownout reset means the supply sagged below ~2.4V: too much load on
    // the 3.3V rail or a weak USB port/cable. Shown as "PWR NO" at boot.
    esp_reset_reason_t reset_reason = esp_reset_reason();
    bool brownout = (reset_reason == ESP_RST_BROWNOUT);
    if (brownout) {
        ESP_LOGE(TAG, "last reset was a BROWNOUT -- check power budget in CLAUDE.md");
    } else {
        ESP_LOGI(TAG, "reset reason: %d", reset_reason);
    }
    i2c_bus_check(bus);

    ESP_ERROR_CHECK(display_init(bus));
    ESP_ERROR_CHECK(display_clear());

    ESP_LOGI(TAG, "OLED init OK");

    // Boot status screen: lights the panel right away and shows each
    // subsystem coming up, instead of a blank screen for ~6-8s.
    ESP_ERROR_CHECK(display_draw_text(0, "HELLO"));
    splash_status(SPLASH_OLED, "OLED", "OK"); // if this is visible, it works
    splash_status(SPLASH_PWR, "PWR", brownout ? "NO" : "OK");
    splash_status(SPLASH_RTC, "RTC", "--");
    splash_status(SPLASH_BME, "BME", "--");
    splash_status(SPLASH_WIFI, "WIFI", "--");
    splash_status(SPLASH_NTP, "NTP", "--");
    splash_status(SPLASH_WEATHER, "WEATHER", "--");

    // RTC first, so the clock is right even if NTP fails later.
    esp_err_t rerr = clock_rtc_init(bus);
    if (rerr != ESP_OK) {
        ESP_LOGW(TAG, "RTC not used: %s", esp_err_to_name(rerr));
    }
    splash_status(SPLASH_RTC, "RTC", rerr == ESP_OK ? "OK" : "NO");

    // Indoor sensor: optional, the main screen just leaves its row blank
    // without it.
    esp_err_t berr = bme280_init(bus);
    if (berr != ESP_OK) {
        ESP_LOGW(TAG, "BME280 not used: %s", esp_err_to_name(berr));
    }
    splash_status(SPLASH_BME, "BME", berr == ESP_OK ? "OK" : "NO");

    // Bounded wait: with no network the clock still comes up on RTC time and
    // WiFi keeps reconnecting in the background; NTP and weather catch up in
    // the main loop once it connects.
    esp_err_t wferr = wifi_connect(WIFI_CONNECT_TIMEOUT_SECONDS * 1000);
    splash_status(SPLASH_WIFI, "WIFI", wferr == ESP_OK ? "OK" : "NO");

    bool ntp_ok = false;
    weather_t weather;
    esp_err_t werr = ESP_ERR_INVALID_STATE;
    if (wferr == ESP_OK) {
        ESP_LOGI(TAG, "WiFi connected");
        ntp_ok = (ntp_sync_with_retry() == ESP_OK);
        if (ntp_ok) {
            ESP_LOGI(TAG, "NTP synced");
        } else {
            ESP_LOGE(TAG, "NTP sync failed after retries, continuing with unsynced clock");
        }
        splash_status(SPLASH_NTP, "NTP", ntp_ok ? "OK" : "NO");

        werr = weather_fetch(&weather);
        if (werr == ESP_OK) {
            s_ui.weather = weather;
            s_ui.weather_ok = true;
            ESP_LOGI(TAG, "weather fetched (%dC, wind %dkm/h, UV %d, sun %s-%s)", weather.temp_c,
                     weather.wind_kmh, weather.uv_max, weather.sunrise, weather.sunset);
        } else {
            ESP_LOGW(TAG, "weather fetch failed: %s", esp_err_to_name(werr));
        }
    } else {
        ESP_LOGW(TAG, "no WiFi, skipping NTP and weather for now");
        splash_status(SPLASH_NTP, "NTP", "NO");
    }
    splash_status(SPLASH_WEATHER, "WEATHER", werr == ESP_OK ? "OK" : "NO");

    // No hold: the fast cells are read while WiFi/NTP/weather are still
    // pending, so the main screens follow the last status straight away.
    esp_err_t bverr = bvg_start();
    if (bverr != ESP_OK) {
        ESP_LOGW(TAG, "BVG not used: %s", esp_err_to_name(bverr));
    }

    esp_err_t eerr = encoder_init();
    if (eerr != ESP_OK) {
        ESP_LOGW(TAG, "encoder not used: %s", esp_err_to_name(eerr));
    }

    ESP_ERROR_CHECK(display_clear());
    s_ui.screen = SCREEN_HOME;
    draw_screen();

#define WEATHER_REFRESH_SECONDS (15 * 60)
#define WEATHER_RETRY_START_SECONDS 30
#define INDOOR_REFRESH_SECONDS 10
    int seconds_since_weather = 0;
    int seconds_since_ntp = 0;
    int seconds_since_indoor = INDOOR_REFRESH_SECONDS; // read on the first pass
    // On failure, retry sooner than the normal cadence and back off toward
    // it, instead of leaving a stale reading up for a full 15 minutes.
    int next_weather_interval = (werr == ESP_OK) ? WEATHER_REFRESH_SECONDS : WEATHER_RETRY_START_SECONDS;
    int last_minute = -1;
    TickType_t next_second = xTaskGetTickCount();
    for (;;) {
        draw_clock_row();
        bool data_changed = false;

        // Minute-based text (BVG countdown) goes stale on the minute.
        time_t now = time(NULL);
        struct tm now_tm;
        localtime_r(&now, &now_tm);
        if (now_tm.tm_min != last_minute) {
            last_minute = now_tm.tm_min;
            data_changed = true;
        }

        // Departures are fetched in the background, only while their screen
        // is up; set from the 1s tick so spinning past it doesn't fetch.
        bvg_set_active(s_ui.screen == SCREEN_BVG);
        if (bvg_take_update(&s_ui.bvg, &s_ui.bvg_ok, &s_ui.bvg_failed)) data_changed = true;

        if (berr == ESP_OK && ++seconds_since_indoor >= INDOOR_REFRESH_SECONDS) {
            seconds_since_indoor = 0;
            bme280_reading_t r;
            if (bme280_read(&r) == ESP_OK) {
                ESP_LOGI(TAG, "indoor %.1fC %.0f%% %.1fhPa", r.temp_c, r.humidity_pct, r.pressure_hpa);
                s_ui.indoor = r;
                s_ui.indoor_ok = true;
                data_changed = true;
            } else {
                ESP_LOGW(TAG, "BME280 read failed");
            }
        }

        // Booted without WiFi (or NTP failed): sync once it's up. The RTC
        // keeps the clock right meanwhile, if one is fitted.
        if (!ntp_ok && ++seconds_since_ntp >= NTP_RETRY_SECONDS && wifi_is_connected()) {
            seconds_since_ntp = 0;
            ntp_ok = (clock_sync_time() == ESP_OK);
            ESP_LOGI(TAG, "late NTP sync %s", ntp_ok ? "done" : "failed");
        }

        // Offline: hold the fetch until WiFi is back, then it fires right away.
        if (++seconds_since_weather >= next_weather_interval && wifi_is_connected()) {
            seconds_since_weather = 0;
            werr = weather_fetch(&weather);
            if (werr == ESP_OK) {
                ESP_LOGI(TAG, "weather refreshed (%dC, wind %dkm/h, UV %d, sun %s-%s)", weather.temp_c,
                         weather.wind_kmh, weather.uv_max, weather.sunrise, weather.sunset);
                s_ui.weather = weather;
                s_ui.weather_ok = true;
                data_changed = true;
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

        if (data_changed) draw_screen();

        // Wait out the rest of this second, reacting to the encoder
        // straight away instead of on the next tick.
        next_second += pdMS_TO_TICKS(1000);
        if ((int32_t)(xTaskGetTickCount() - next_second) > 0) {
            next_second = xTaskGetTickCount(); // overran (slow fetch): don't try to catch up
        }
        for (;;) {
            TickType_t now = xTaskGetTickCount();
            if ((int32_t)(next_second - now) <= 0) break;
            encoder_event_t ev;
            if (!encoder_wait_event(&ev, next_second - now)) continue;
            // A quick spin queues several clicks: apply them all, draw once.
            int screen = s_ui.screen;
            do {
                if (ev == ENCODER_EV_CW) screen = (screen + 1) % SCREEN_COUNT;
                else if (ev == ENCODER_EV_CCW) screen = (screen + SCREEN_COUNT - 1) % SCREEN_COUNT;
                else screen = SCREEN_HOME;
            } while (encoder_wait_event(&ev, 0));
            if (screen != (int)s_ui.screen) {
                s_ui.screen = screen;
                ESP_LOGI(TAG, "screen %d", screen);
                draw_screen();
            }
        }
    }
}
