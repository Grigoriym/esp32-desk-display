#include "scd41.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "scd41";

#define SCD41_ADDR 0x62

// 16-bit commands, sent MSB first.
#define SCD41_CMD_STOP_PERIODIC            0x3F86 // then 500 ms before any other command
#define SCD41_CMD_START_LOW_POWER_PERIODIC 0x21AC // one reading every 30 s, ~3 mA average
#define SCD41_CMD_DATA_READY               0xE4B8
#define SCD41_CMD_READ_MEASUREMENT         0xEC05
// Automatic self-calibration (ASC) settings; readable only while idle.
#define SCD41_CMD_GET_ASC_ENABLED  0x2313
#define SCD41_CMD_GET_ASC_TARGET   0x233F // ppm
#define SCD41_CMD_GET_ASC_INITIAL  0x2340 // hours
#define SCD41_CMD_GET_ASC_STANDARD 0x234B // hours

static i2c_master_dev_handle_t s_dev; // NULL if no SCD41 on the bus

static esp_err_t send_cmd(uint16_t cmd)
{
    uint8_t buf[2] = {(uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF)};
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 1000);
}

// Sends cmd, waits its 1 ms execution time (one tick at 100 Hz), reads len bytes.
static esp_err_t read_cmd(uint16_t cmd, uint8_t *buf, size_t len)
{
    esp_err_t err = send_cmd(cmd);
    if (err != ESP_OK) return err;
    vTaskDelay(1);
    return i2c_master_receive(s_dev, buf, len, 1000);
}

// One-word setting, or -1 if it couldn't be read.
static int read_setting(uint16_t cmd)
{
    uint8_t buf[3];
    uint16_t v;
    if (read_cmd(cmd, buf, sizeof(buf)) != ESP_OK || !scd41_parse_word(buf, &v)) return -1;
    return v;
}

// Logs the ASC settings. ASC counts only stretches of >= 4 h of measuring,
// and assumes the lowest CO2 seen in each period is the target (fresh air).
static void log_asc(void)
{
    ESP_LOGI(TAG, "ASC enabled %d, target %d ppm, initial period %d h, standard period %d h",
             read_setting(SCD41_CMD_GET_ASC_ENABLED), read_setting(SCD41_CMD_GET_ASC_TARGET),
             read_setting(SCD41_CMD_GET_ASC_INITIAL), read_setting(SCD41_CMD_GET_ASC_STANDARD));
}

esp_err_t scd41_init(i2c_master_bus_handle_t bus)
{
    if (i2c_master_probe(bus, SCD41_ADDR, 50) != ESP_OK) return ESP_ERR_NOT_FOUND;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SCD41_ADDR,
        .scl_speed_hz = 100000,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) {
        s_dev = NULL;
        return err;
    }

    // After an ESP32 reset the sensor may still be measuring from before, and
    // it refuses a start while it is.
    err = send_cmd(SCD41_CMD_STOP_PERIODIC);
    if (err == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(500));
        log_asc();
        err = send_cmd(SCD41_CMD_START_LOW_POWER_PERIODIC);
    }
    if (err != ESP_OK) {
        i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
    }
    return err;
}

esp_err_t scd41_read(scd41_reading_t *out)
{
    if (!s_dev) return ESP_ERR_INVALID_STATE;

    uint8_t buf[9];
    esp_err_t err = read_cmd(SCD41_CMD_DATA_READY, buf, 3);
    if (err != ESP_OK) return err;
    bool ready;
    if (!scd41_parse_data_ready(buf, &ready)) return ESP_ERR_INVALID_CRC;
    if (!ready) return ESP_ERR_NOT_FINISHED;

    if ((err = read_cmd(SCD41_CMD_READ_MEASUREMENT, buf, 9)) != ESP_OK) return err;
    if (!scd41_parse_measurement(buf, out)) {
        ESP_LOGW(TAG, "measurement CRC mismatch");
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}
