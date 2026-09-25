#include <string.h>
#include "display.h"
#include "font.h"

#define OLED_WIDTH 128
#define OLED_PAGES 8

static i2c_master_dev_handle_t s_dev;

static const uint8_t ssd1306_init_cmds[] = {
    0xAE,       // display off
    0xD5, 0x80, // clock divide
    0xA8, 0x3F, // multiplex ratio = 63 (64px tall)
    0xD3, 0x00, // display offset = 0
    0x40,       // start line = 0
    0x8D, 0x14, // charge pump on
    0x20, 0x02, // page addressing mode
    0xA0,       // segment remap (rotated 180 from panel default)
    0xC0,       // COM scan direction (rotated 180 from panel default)
    0xDA, 0x12, // COM pins config for 128x64
    0x81, 0x40, // contrast (0x00-0xFF; was 0xCF, 0x40 looks the same indoors)
    0xD9, 0xF1, // precharge
    0xDB, 0x40, // VCOMH deselect level
    0xA4,       // resume to RAM content display
    0xA6,       // normal (not inverted)
    0xAF,       // display on
};

static esp_err_t ssd1306_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd}; // control byte 0x00 = command follows
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 1000);
}

static esp_err_t ssd1306_write_page(int page, const uint8_t *row, size_t len)
{
    uint8_t buf[1 + OLED_WIDTH];
    buf[0] = 0x40; // control byte 0x40 = data follows
    memcpy(&buf[1], row, len);

    esp_err_t err;
    if ((err = ssd1306_cmd(0xB0 | page)) != ESP_OK) return err; // set page
    if ((err = ssd1306_cmd(0x00)) != ESP_OK) return err;        // lower column = 0
    if ((err = ssd1306_cmd(0x10)) != ESP_OK) return err;        // upper column = 0
    return i2c_master_transmit(s_dev, buf, len + 1, 1000);
}

esp_err_t display_init(i2c_master_bus_handle_t bus)
{
    uint16_t addr = 0;
    if (i2c_master_probe(bus, 0x3C, 50) == ESP_OK) {
        addr = 0x3C;
    } else if (i2c_master_probe(bus, 0x3D, 50) == ESP_OK) {
        addr = 0x3D;
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 100000,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) return err;

    for (size_t i = 0; i < sizeof(ssd1306_init_cmds); i++) {
        if ((err = ssd1306_cmd(ssd1306_init_cmds[i])) != ESP_OK) return err;
    }
    return ESP_OK;
}

esp_err_t display_clear(void)
{
    uint8_t blank[OLED_WIDTH];
    memset(blank, 0x00, sizeof(blank));
    for (int page = 0; page < OLED_PAGES; page++) {
        esp_err_t err = ssd1306_write_page(page, blank, OLED_WIDTH);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

esp_err_t display_set_on(bool on)
{
    return ssd1306_cmd(on ? 0xAF : 0xAE);
}

esp_err_t display_draw_text(int page, const char *text)
{
    uint8_t row[OLED_WIDTH];
    memset(row, 0x00, sizeof(row));
    font_blit(row, OLED_WIDTH, (OLED_WIDTH - font_text_width(text)) / 2, text);
    return ssd1306_write_page(page, row, OLED_WIDTH);
}

esp_err_t display_draw_text_columns(int page, const char *left, const char *right)
{
    uint8_t row[OLED_WIDTH];
    memset(row, 0x00, sizeof(row));
    font_blit(row, OLED_WIDTH, 0, left);
    font_blit(row, OLED_WIDTH, OLED_WIDTH - font_text_width(right), right);
    return ssd1306_write_page(page, row, OLED_WIDTH);
}

esp_err_t display_draw_icon(int page, const uint8_t *icon)
{
    uint8_t row[OLED_WIDTH];
    memset(row, 0x00, sizeof(row));
    if (icon) {
        memcpy(&row[(OLED_WIDTH - 8) / 2], icon, 8);
    }
    return ssd1306_write_page(page, row, OLED_WIDTH);
}

#define ICON_WIDTH    8
#define ICON_TEXT_GAP 4

esp_err_t display_draw_icon_and_text(int page, const uint8_t *icon, const char *text)
{
    uint8_t row[OLED_WIDTH];
    memset(row, 0x00, sizeof(row));

    int text_width = font_text_width(text);
    int icon_width = icon ? ICON_WIDTH : 0;
    int gap = (icon && text_width > 0) ? ICON_TEXT_GAP : 0;

    int col = (OLED_WIDTH - (icon_width + gap + text_width)) / 2;

    if (icon) {
        memcpy(&row[col], icon, ICON_WIDTH);
        col += icon_width + gap;
    }
    font_blit(row, OLED_WIDTH, col, text);

    return ssd1306_write_page(page, row, OLED_WIDTH);
}
