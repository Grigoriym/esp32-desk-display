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
#include "screens.h"
#include "health.h"
#include "metrics.h"
#include "metrics_format.h"
#include "bvg_secrets.h"
#include <time.h>

static const char *TAG = "desk_display";

#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_PORT     I2C_NUM_0

#define WIFI_CONNECT_TIMEOUT_SECONDS 15
#define NTP_RETRY_SECONDS            60

// Boot status grid: two columns, local hardware on top, network below.
// Each item is a (page, column) cell; column 0 = left, 1 = right.
#define SPLASH_OLED    2, 0
#define SPLASH_PWR     2, 1
#define SPLASH_RTC     3, 0
#define SPLASH_BME     3, 1
#define SPLASH_WIFI    5, 0
#define SPLASH_NTP     5, 1
#define SPLASH_WEATHER 6, 0

// Main screens (see screens.h). The clock row sits on page 0 of every
// screen; draw_screen() owns pages 1-7.
#define PAGE_CLOCK 0 // time left, date right

static screen_t s_screen;
static screen_data_t s_data = {
    .bvg_walk_min = BVG_WALK_MIN_MINUTES,
    .bvg_walk_comfort = BVG_WALK_COMFORT_MINUTES,
};

// Everything that's allowed to answer on the I2C bus. Anything else showing
// up means an address clash or an unplanned module -- see "Power & bus
// budget" in CLAUDE.md before wiring a new one, then add it here.
static const struct {
    uint8_t addr;
    const char *name;
} KNOWN_I2C[] = {
    {0x3C, "OLED"},
    {0x3D, "OLED (alt addr)"},
    {0x57, "DS3231 EEPROM (AT24C32)"},
    {0x5F, "DS3231 module extra addr"},
    {0x68, "DS3231 RTC"},
    {0x76, "BME280"},
    {0x77, "BME280 (alt addr)"},
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
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    screen_rows_t rows;
    screen_layout(s_screen, &s_data, t.tm_hour * 60 + t.tm_min, &rows);

    for (int page = 1; page < 8; page++) {
        const char *left = rows.text[page][0];
        const char *right = rows.text[page][1];
        if (s_screen == SCREEN_HOME && page == 3) {
            // Icon + outdoor temperature, centred together.
            int code = s_data.weather_ok ? s_data.weather.weather_code : 3;
            display_draw_icon_and_text(page, weather_icon_for_code(code), left);
        } else if (right[0]) {
            display_draw_text_columns(page, left, right);
        } else {
            display_draw_text(page, left); // centred; "" blanks the page
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

#define WEATHER_REFRESH_SECONDS     (15 * 60)
#define WEATHER_RETRY_START_SECONDS 30
#define INDOOR_REFRESH_SECONDS      10
#define HEALTH_LOG_SECONDS          (5 * 60)
#define METRICS_SECONDS             60

// What the boot sequence found out, and the main loop's timers.
typedef struct {
    bool indoor_present; // BME280 found at boot
    bool ntp_ok;
    int seconds_since_weather;
    int next_weather_interval; // shortened after a failure, see tick_weather()
    int seconds_since_indoor;
    int seconds_since_ntp;
    int seconds_since_health;
    int seconds_since_metrics;
    int last_minute;
} app_state_t;

static i2c_master_bus_handle_t i2c_init(void)
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
    return bus;
}

// A brownout reset means the supply sagged below ~2.4V: too much load on the
// 3.3V rail or a weak USB port/cable. Shown as "PWR NO" at boot.
static bool last_reset_was_brownout(void)
{
    esp_reset_reason_t reset_reason = esp_reset_reason();
    if (reset_reason == ESP_RST_BROWNOUT) {
        ESP_LOGE(TAG, "last reset was a BROWNOUT -- check power budget in CLAUDE.md");
        return true;
    }
    ESP_LOGI(TAG, "reset reason: %d", reset_reason);
    return false;
}

// Boot status screen: lights the panel right away and shows each subsystem
// coming up, instead of a blank screen for ~6-8s.
static void splash_begin(bool brownout)
{
    ESP_ERROR_CHECK(display_draw_text(0, "HELLO"));
    splash_status(SPLASH_OLED, "OLED", "OK"); // if this is visible, it works
    splash_status(SPLASH_PWR, "PWR", brownout ? "NO" : "OK");
    splash_status(SPLASH_RTC, "RTC", "--");
    splash_status(SPLASH_BME, "BME", "--");
    splash_status(SPLASH_WIFI, "WIFI", "--");
    splash_status(SPLASH_NTP, "NTP", "--");
    splash_status(SPLASH_WEATHER, "WEATHER", "--");
}

// Local hardware: RTC first, so the clock is right even if NTP fails later.
// The BME280 is optional; its screen rows just stay "--" without it.
static void boot_local(i2c_master_bus_handle_t bus, app_state_t *st)
{
    esp_err_t rerr = clock_rtc_init(bus);
    if (rerr != ESP_OK) ESP_LOGW(TAG, "RTC not used: %s", esp_err_to_name(rerr));
    splash_status(SPLASH_RTC, "RTC", rerr == ESP_OK ? "OK" : "NO");

    esp_err_t berr = bme280_init(bus);
    if (berr != ESP_OK) ESP_LOGW(TAG, "BME280 not used: %s", esp_err_to_name(berr));
    splash_status(SPLASH_BME, "BME", berr == ESP_OK ? "OK" : "NO");
    st->indoor_present = (berr == ESP_OK);
}

// Fetches the weather, air quality and DWD warnings into s_data; returns
// whether all three worked.
static bool fetch_weather(const char *what)
{
    weather_t weather;
    esp_err_t err = weather_fetch(&weather);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "weather %s failed: %s", what, esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "weather %s (%dC, wind %dkm/h, UV %d, sun %s-%s, rain in %dh from %s till %s)", what,
             weather.temp_c, weather.wind_kmh, weather.uv_max, weather.sunrise, weather.sunset,
             weather.rain_in_h, weather.rain_from, weather.rain_until);
    s_data.weather = weather;
    s_data.weather_ok = true;

    // Air quality rides along on the same cadence; a failure keeps the last
    // reading and makes the whole fetch count as failed, so it's retried soon.
    air_t air;
    err = air_fetch(&air);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "air %s failed: %s", what, esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "air %s (AQI %d, pollen alder %d birch %d grass %d mugwort %d ragweed %d)", what, air.aqi,
             air.pollen[POLLEN_ALDER], air.pollen[POLLEN_BIRCH], air.pollen[POLLEN_GRASS],
             air.pollen[POLLEN_MUGWORT], air.pollen[POLLEN_RAGWEED]);
    s_data.air = air;
    s_data.air_ok = true;

    // Warnings too, but a failure drops them instead of keeping the last
    // ones: a stale warning on screen is worse than none.
    char now_local[17];
    time_t now = time(NULL);
    struct tm now_tm;
    localtime_r(&now, &now_tm);
    strftime(now_local, sizeof(now_local), "%Y-%m-%dT%H:%M", &now_tm);
    alerts_t alerts;
    err = alerts_fetch(now_local, &alerts);
    s_data.alerts_ok = (err == ESP_OK);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "alerts %s failed: %s", what, esp_err_to_name(err));
        return false;
    }
    if (alerts.count > 0) {
        ESP_LOGI(TAG, "alerts %s (%d, showing %s, severity %d, %s %s)", what, alerts.count, alerts.event,
                 (int)alerts.severity, alerts.started ? "since" : "from", alerts.onset);
    } else {
        ESP_LOGI(TAG, "alerts %s (none)", what);
    }
    s_data.alerts = alerts;
    return true;
}

// Network: bounded WiFi wait. With no network the clock still comes up on RTC
// time and WiFi keeps reconnecting in the background; NTP and weather catch
// up in the main loop once it connects.
static void boot_network(app_state_t *st)
{
    esp_err_t wferr = wifi_connect(WIFI_CONNECT_TIMEOUT_SECONDS * 1000);
    splash_status(SPLASH_WIFI, "WIFI", wferr == ESP_OK ? "OK" : "NO");

    bool weather_ok = false;
    if (wferr == ESP_OK) {
        ESP_LOGI(TAG, "WiFi connected");
        st->ntp_ok = (ntp_sync_with_retry() == ESP_OK);
        if (st->ntp_ok) ESP_LOGI(TAG, "NTP synced");
        else ESP_LOGE(TAG, "NTP sync failed after retries, continuing with unsynced clock");
        splash_status(SPLASH_NTP, "NTP", st->ntp_ok ? "OK" : "NO");
        weather_ok = fetch_weather("fetched");
    } else {
        ESP_LOGW(TAG, "no WiFi, skipping NTP and weather for now");
        splash_status(SPLASH_NTP, "NTP", "NO");
    }
    splash_status(SPLASH_WEATHER, "WEATHER", weather_ok ? "OK" : "NO");
    // On failure, retry sooner than the normal cadence and back off toward
    // it, instead of leaving a stale reading up for a full 15 minutes.
    st->next_weather_interval = weather_ok ? WEATHER_REFRESH_SECONDS : WEATHER_RETRY_START_SECONDS;
}

// Background parts that don't block boot.
static void start_background(void)
{
    esp_err_t bverr = bvg_start();
    if (bverr != ESP_OK) ESP_LOGW(TAG, "BVG not used: %s", esp_err_to_name(bverr));
    esp_err_t eerr = encoder_init();
    if (eerr != ESP_OK) ESP_LOGW(TAG, "encoder not used: %s", esp_err_to_name(eerr));
    esp_err_t merr = metrics_start();
    if (merr != ESP_OK) ESP_LOGI(TAG, "dashboard upload off: %s", esp_err_to_name(merr));
}

// The per-second jobs below return true when the screen needs a redraw.

// Minute-based text (BVG countdown) goes stale on the minute.
static bool tick_minute(app_state_t *st)
{
    time_t now = time(NULL);
    struct tm now_tm;
    localtime_r(&now, &now_tm);
    if (now_tm.tm_min == st->last_minute) return false;
    st->last_minute = now_tm.tm_min;
    return true;
}

// Departures are fetched in the background, only while their screen is up;
// set from the 1s tick so spinning past it doesn't fetch.
static bool tick_bvg(void)
{
    bvg_set_active(s_screen == SCREEN_BVG);
    return bvg_take_update(&s_data.bvg, &s_data.bvg_ok, &s_data.bvg_failed);
}

static bool tick_indoor(app_state_t *st)
{
    if (!st->indoor_present || ++st->seconds_since_indoor < INDOOR_REFRESH_SECONDS) return false;
    st->seconds_since_indoor = 0;
    bme280_reading_t r;
    if (bme280_read(&r) != ESP_OK) {
        ESP_LOGW(TAG, "BME280 read failed");
        return false;
    }
    ESP_LOGI(TAG, "indoor %.1fC %.0f%% %.1fhPa", r.temp_c, r.humidity_pct, r.pressure_hpa);
    s_data.indoor = r;
    s_data.indoor_ok = true;
    return true;
}

// Booted without WiFi (or NTP failed): sync once it's up. The RTC keeps the
// clock right meanwhile, if one is fitted.
static void tick_ntp(app_state_t *st)
{
    if (st->ntp_ok || ++st->seconds_since_ntp < NTP_RETRY_SECONDS || !wifi_is_connected()) return;
    st->seconds_since_ntp = 0;
    st->ntp_ok = (clock_sync_time() == ESP_OK);
    ESP_LOGI(TAG, "late NTP sync %s", st->ntp_ok ? "done" : "failed");
}

// Offline: hold the fetch until WiFi is back, then it fires right away.
static bool tick_weather(app_state_t *st)
{
    if (++st->seconds_since_weather < st->next_weather_interval || !wifi_is_connected()) return false;
    st->seconds_since_weather = 0;
    if (fetch_weather("refreshed")) {
        st->next_weather_interval = WEATHER_REFRESH_SECONDS;
        return true;
    }
    st->next_weather_interval *= 2;
    if (st->next_weather_interval > WEATHER_REFRESH_SECONDS)
        st->next_weather_interval = WEATHER_REFRESH_SECONDS;
    return false;
}

// Heap and stack headroom, every few minutes and once a minute after boot
// (by then the first TLS fetch has run, so the numbers mean something).
static void tick_health(app_state_t *st)
{
    if (++st->seconds_since_health < HEALTH_LOG_SECONDS) return;
    st->seconds_since_health = 0;
    health_log();
}

// Readings to the dashboard (server/), handed to the upload task.
static void tick_metrics(app_state_t *st)
{
    if (++st->seconds_since_metrics < METRICS_SECONDS || !wifi_is_connected()) return;
    st->seconds_since_metrics = 0;
    metrics_device_t dev = {
        .uptime_s = (long)(xTaskGetTickCount() / configTICK_RATE_HZ), // wraps after ~497 days
        .heap_free_kb = (int)(esp_get_free_heap_size() / 1024),
    };
    dev.rssi_ok = wifi_rssi(&dev.rssi);
    char lines[512];
    if (metrics_format(&s_data, &dev, lines, sizeof(lines)) < 0) {
        ESP_LOGW(TAG, "metrics batch too big");
        return;
    }
    metrics_submit(lines);
}

// Waits out the rest of this second, reacting to the encoder straight away
// instead of on the next tick. A quick spin queues several clicks: they're
// all applied, then drawn once.
static void wait_second_handling_encoder(TickType_t *next_second)
{
    *next_second += pdMS_TO_TICKS(1000);
    if ((int32_t)(xTaskGetTickCount() - *next_second) > 0) {
        *next_second = xTaskGetTickCount(); // overran (slow fetch): don't try to catch up
    }
    for (;;) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(*next_second - now) <= 0) return;
        encoder_event_t ev;
        if (!encoder_wait_event(&ev, *next_second - now)) continue;
        int screen = s_screen;
        do {
            if (ev == ENCODER_EV_CW) screen = (screen + 1) % SCREEN_COUNT;
            else if (ev == ENCODER_EV_CCW) screen = (screen + SCREEN_COUNT - 1) % SCREEN_COUNT;
            else screen = SCREEN_HOME;
        } while (encoder_wait_event(&ev, 0));
        if (screen != (int)s_screen) {
            s_screen = screen;
            ESP_LOGI(TAG, "screen %d", screen);
            draw_screen();
        }
    }
}

void app_main(void)
{
    // Indoor read on the first pass, health report 60s in.
    app_state_t st = {
        .seconds_since_indoor = INDOOR_REFRESH_SECONDS,
        .seconds_since_health = HEALTH_LOG_SECONDS - 60,
        .last_minute = -1,
    };

    i2c_master_bus_handle_t bus = i2c_init();
    bool brownout = last_reset_was_brownout();
    i2c_bus_check(bus);

    ESP_ERROR_CHECK(display_init(bus));
    ESP_ERROR_CHECK(display_clear());
    ESP_LOGI(TAG, "OLED init OK");

    splash_begin(brownout);
    boot_local(bus, &st);
    boot_network(&st);
    // No hold: the fast cells are read while WiFi/NTP/weather are still
    // pending, so the main screens follow the last status straight away.
    start_background();

    ESP_ERROR_CHECK(display_clear());
    s_screen = SCREEN_HOME;
    draw_screen();

    TickType_t next_second = xTaskGetTickCount();
    for (;;) {
        draw_clock_row();
        bool changed = tick_minute(&st);
        changed |= tick_bvg();
        changed |= tick_indoor(&st);
        tick_ntp(&st);
        changed |= tick_weather(&st);
        tick_health(&st);
        tick_metrics(&st);
        if (changed) draw_screen();
        wait_second_handling_encoder(&next_second);
    }
}
