#include "bme280.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "bme280";

#define BME280_CHIP_ID          0x60 // a BMP280 reports 0x58
#define BME280_REG_CALIB_TP     0x88 // 26 regs: T1-T3, P1-P9, (unused), H1
#define BME280_REG_CHIP_ID      0xD0
#define BME280_REG_CALIB_H      0xE1 // 7 regs: H2-H6
#define BME280_REG_CTRL_HUM     0xF2
#define BME280_REG_STATUS       0xF3
#define BME280_REG_CTRL_MEAS    0xF4
#define BME280_REG_DATA         0xF7 // 8 regs: press[3], temp[3], hum[2]
#define BME280_STATUS_MEASURING 0x08

// Oversampling x1 for everything, forced mode: one measurement per request,
// then the chip sleeps -- no self-heating from continuous sampling.
#define BME280_CTRL_HUM_X1      0x01
#define BME280_CTRL_MEAS_FORCED ((1 << 5) | (1 << 2) | 0x01)

static i2c_master_dev_handle_t s_dev; // NULL if no BME280 on the bus

static struct {
    uint16_t t1;
    int16_t t2, t3;
    uint16_t p1;
    int16_t p2, p3, p4, p5, p6, p7, p8, p9;
    uint8_t h1, h3;
    int16_t h2, h4, h5;
    int8_t h6;
} s_cal;

static esp_err_t reg_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, data, len, 1000);
}

static esp_err_t reg_write(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 1000);
}

static uint16_t u16le(const uint8_t *b)
{
    return (uint16_t)(b[0] | (b[1] << 8));
}

static esp_err_t load_calibration(void)
{
    uint8_t c[26];
    esp_err_t err = reg_read(BME280_REG_CALIB_TP, c, sizeof(c));
    if (err != ESP_OK) return err;
    s_cal.t1 = u16le(&c[0]);
    s_cal.t2 = (int16_t)u16le(&c[2]);
    s_cal.t3 = (int16_t)u16le(&c[4]);
    s_cal.p1 = u16le(&c[6]);
    s_cal.p2 = (int16_t)u16le(&c[8]);
    s_cal.p3 = (int16_t)u16le(&c[10]);
    s_cal.p4 = (int16_t)u16le(&c[12]);
    s_cal.p5 = (int16_t)u16le(&c[14]);
    s_cal.p6 = (int16_t)u16le(&c[16]);
    s_cal.p7 = (int16_t)u16le(&c[18]);
    s_cal.p8 = (int16_t)u16le(&c[20]);
    s_cal.p9 = (int16_t)u16le(&c[22]);
    s_cal.h1 = c[25];

    uint8_t h[7];
    if ((err = reg_read(BME280_REG_CALIB_H, h, sizeof(h))) != ESP_OK) return err;
    s_cal.h2 = (int16_t)u16le(&h[0]);
    s_cal.h3 = h[2];
    // H4/H5 are 12-bit values sharing the nibbles of 0xE5.
    s_cal.h4 = (int16_t)(((int8_t)h[3] * 16) | (h[4] & 0x0F));
    s_cal.h5 = (int16_t)(((int8_t)h[5] * 16) | (h[4] >> 4));
    s_cal.h6 = (int8_t)h[6];
    return ESP_OK;
}

esp_err_t bme280_init(i2c_master_bus_handle_t bus)
{
    uint16_t addr;
    if (i2c_master_probe(bus, 0x76, 50) == ESP_OK) {
        addr = 0x76;
    } else if (i2c_master_probe(bus, 0x77, 50) == ESP_OK) {
        addr = 0x77;
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 100000,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) {
        s_dev = NULL;
        return err;
    }

    uint8_t id;
    if ((err = reg_read(BME280_REG_CHIP_ID, &id, 1)) == ESP_OK && id != BME280_CHIP_ID) {
        ESP_LOGW(TAG, "chip ID 0x%02X at 0x%02X is not a BME280", id, addr);
        err = ESP_ERR_NOT_SUPPORTED;
    }
    if (err == ESP_OK) err = load_calibration();
    // ctrl_hum only takes effect after the next ctrl_meas write, which
    // bme280_read() does every time.
    if (err == ESP_OK) err = reg_write(BME280_REG_CTRL_HUM, BME280_CTRL_HUM_X1);
    if (err != ESP_OK) {
        i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
    }
    return err;
}

// Compensation formulas below are the integer versions from the Bosch
// BME280 datasheet (section 4.2.3 / 8.2), transcribed as-is.

static int32_t compensate_temp(int32_t adc_t, int32_t *t_fine)
{
    int32_t var1 = ((((adc_t >> 3) - ((int32_t)s_cal.t1 << 1))) * (int32_t)s_cal.t2) >> 11;
    int32_t var2 = (((((adc_t >> 4) - (int32_t)s_cal.t1) * ((adc_t >> 4) - (int32_t)s_cal.t1)) >> 12)
                    * (int32_t)s_cal.t3)
                   >> 14;
    *t_fine = var1 + var2;
    return (*t_fine * 5 + 128) >> 8; // 0.01 degC
}

static uint32_t compensate_press(int32_t adc_p, int32_t t_fine)
{
    int64_t var1 = (int64_t)t_fine - 128000;
    int64_t var2 = var1 * var1 * (int64_t)s_cal.p6;
    var2 = var2 + ((var1 * (int64_t)s_cal.p5) << 17);
    var2 = var2 + (((int64_t)s_cal.p4) << 35);
    var1 = ((var1 * var1 * (int64_t)s_cal.p3) >> 8) + ((var1 * (int64_t)s_cal.p2) << 12);
    var1 = ((((int64_t)1) << 47) + var1) * ((int64_t)s_cal.p1) >> 33;
    if (var1 == 0) return 0; // avoid division by zero
    int64_t p = 1048576 - adc_p;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)s_cal.p9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)s_cal.p8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)s_cal.p7) << 4);
    return (uint32_t)p; // Pa, Q24.8
}

static uint32_t compensate_hum(int32_t adc_h, int32_t t_fine)
{
    int32_t v = t_fine - (int32_t)76800;
    v = (((((adc_h << 14) - (((int32_t)s_cal.h4) << 20) - (((int32_t)s_cal.h5) * v)) + (int32_t)16384) >> 15)
         * (((((((v * (int32_t)s_cal.h6) >> 10) * (((v * (int32_t)s_cal.h3) >> 11) + (int32_t)32768)) >> 10)
              + (int32_t)2097152)
                 * (int32_t)s_cal.h2
             + 8192)
            >> 14));
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * (int32_t)s_cal.h1) >> 4);
    v = v < 0 ? 0 : v;
    v = v > 419430400 ? 419430400 : v;
    return (uint32_t)(v >> 12); // %RH, Q22.10
}

esp_err_t bme280_read(bme280_reading_t *out)
{
    if (!s_dev) return ESP_ERR_INVALID_STATE;

    esp_err_t err = reg_write(BME280_REG_CTRL_MEAS, BME280_CTRL_MEAS_FORCED);
    if (err != ESP_OK) return err;

    // x1 oversampling on all three takes ~8-10ms; poll rather than guess.
    uint8_t status;
    for (int i = 0; i < 10; i++) {
        vTaskDelay(pdMS_TO_TICKS(5));
        if ((err = reg_read(BME280_REG_STATUS, &status, 1)) != ESP_OK) return err;
        if (!(status & BME280_STATUS_MEASURING)) break;
    }

    uint8_t d[8];
    if ((err = reg_read(BME280_REG_DATA, d, sizeof(d))) != ESP_OK) return err;
    int32_t adc_p = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    int32_t adc_t = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    int32_t adc_h = ((int32_t)d[6] << 8) | d[7];

    int32_t t_fine;
    out->temp_c = compensate_temp(adc_t, &t_fine) / 100.0f;
    out->pressure_hpa = compensate_press(adc_p, t_fine) / 25600.0f; // Q24.8 Pa -> hPa
    out->humidity_pct = compensate_hum(adc_h, t_fine) / 1024.0f;
    return ESP_OK;
}
