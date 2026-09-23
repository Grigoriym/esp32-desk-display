#include <time.h>
#include <sys/time.h>
#include "clock.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"

static const char *TAG = "clock";

// Hardcoded UTC offset (CLAUDE.md: no IP-geolocation for now), no DST
// handling -- Berlin is UTC+2 (CEST) until DST ends in late October, then
// UTC+1 (CET) until next spring. Update this by hand when it flips.
#define UTC_OFFSET_HOURS 2

// DS3231 RTC: stores UTC (not local time), so the DST flip above never
// needs to touch it.
#define DS3231_ADDR       0x68
#define DS3231_REG_TIME   0x00 // 7 regs: sec, min, hour, weekday, date, month, year
#define DS3231_REG_STATUS 0x0F
#define DS3231_OSF        0x80 // oscillator stopped: time in the chip is invalid

static i2c_master_dev_handle_t s_rtc; // NULL if no RTC on the bus

static uint8_t bcd_to_bin(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static uint8_t bin_to_bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

static esp_err_t rtc_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_rtc, &reg, 1, data, len, 1000);
}

static esp_err_t rtc_write_utc(time_t now)
{
    struct tm t;
    gmtime_r(&now, &t);
    uint8_t buf[8] = {
        DS3231_REG_TIME,
        bin_to_bcd(t.tm_sec),
        bin_to_bcd(t.tm_min),
        bin_to_bcd(t.tm_hour), // bit 6 clear = 24h mode
        t.tm_wday + 1,
        bin_to_bcd(t.tm_mday),
        bin_to_bcd(t.tm_mon + 1),
        bin_to_bcd(t.tm_year - 100), // chip holds 2000-2099
    };
    esp_err_t err = i2c_master_transmit(s_rtc, buf, sizeof(buf), 1000);
    if (err != ESP_OK) return err;

    // Clear OSF so the next boot trusts the stored time.
    uint8_t status;
    if ((err = rtc_read(DS3231_REG_STATUS, &status, 1)) != ESP_OK) return err;
    uint8_t clr[2] = { DS3231_REG_STATUS, status & ~DS3231_OSF };
    return i2c_master_transmit(s_rtc, clr, sizeof(clr), 1000);
}

esp_err_t clock_rtc_init(i2c_master_bus_handle_t bus)
{
    esp_err_t err = i2c_master_probe(bus, DS3231_ADDR, 50);
    if (err != ESP_OK) return err;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_ADDR,
        .scl_speed_hz = 100000,
    };
    if ((err = i2c_master_bus_add_device(bus, &dev_cfg, &s_rtc)) != ESP_OK) {
        s_rtc = NULL;
        return err;
    }

    uint8_t status;
    if ((err = rtc_read(DS3231_REG_STATUS, &status, 1)) != ESP_OK) return err;
    if (status & DS3231_OSF) {
        ESP_LOGW(TAG, "RTC found but its time is invalid (never set / battery lost), waiting for NTP");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t r[7];
    if ((err = rtc_read(DS3231_REG_TIME, r, sizeof(r))) != ESP_OK) return err;
    struct tm t = {
        .tm_sec = bcd_to_bin(r[0] & 0x7F),
        .tm_min = bcd_to_bin(r[1] & 0x7F),
        .tm_hour = bcd_to_bin(r[2] & 0x3F),
        .tm_mday = bcd_to_bin(r[4] & 0x3F),
        .tm_mon = bcd_to_bin(r[5] & 0x1F) - 1,
        .tm_year = bcd_to_bin(r[6]) + 100,
    };
    // TZ is never set in this firmware, so mktime() treats t as UTC.
    struct timeval tv = { .tv_sec = mktime(&t) };
    settimeofday(&tv, NULL);
    ESP_LOGI(TAG, "system time set from RTC: %04d-%02d-%02d %02d:%02d:%02d UTC",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    return ESP_OK;
}

esp_err_t clock_sync_time(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_err_t err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) return err;

    err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000));
    if (err != ESP_OK) {
        // Tear down so the next retry's init doesn't fail with "already initialized".
        esp_netif_sntp_deinit();
        return err;
    }

    ESP_LOGI(TAG, "NTP sync done");
    if (s_rtc) {
        err = rtc_write_utc(time(NULL));
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "RTC updated from NTP");
        } else {
            ESP_LOGW(TAG, "RTC write failed: %s", esp_err_to_name(err));
        }
    }
    return ESP_OK;
}

static void format_local(char *buf, size_t buf_size, const char *fmt)
{
    time_t now = time(NULL) + (time_t)(UTC_OFFSET_HOURS * 3600);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    strftime(buf, buf_size, fmt, &timeinfo);
}

void clock_format_now(char *buf, size_t buf_size)
{
    format_local(buf, buf_size, "%H:%M");
}

void clock_format_date(char *buf, size_t buf_size)
{
    format_local(buf, buf_size, "%d/%m/%Y");
}
