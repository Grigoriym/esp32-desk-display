#include <string.h>
#include "display.h"

#define OLED_WIDTH  128
#define OLED_PAGES  8

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
    0x81, 0xCF, // contrast
    0xD9, 0xF1, // precharge
    0xDB, 0x40, // VCOMH deselect level
    0xA4,       // resume to RAM content display
    0xA6,       // normal (not inverted)
    0xAF,       // display on
};

// 5x7 font, column-major (bit0 = top row). Letters are the classic
// Adafruit GFX glcdfont A-Z (C kept as this project's original open-sided
// variant); digits/symbols below are hand-derived.
static const uint8_t font_upper[26][5] = {
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x00}, // C
    {0x7F, 0x41, 0x41, 0x41, 0x3E}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x73}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x1C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x26, 0x49, 0x49, 0x49, 0x32}, // S
    {0x03, 0x01, 0x7F, 0x01, 0x03}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x59, 0x49, 0x4D, 0x43}, // Z
};

static const uint8_t glyph_0[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t glyph_1[5] = {0x00, 0x42, 0x7F, 0x40, 0x00};
static const uint8_t glyph_2[5] = {0x42, 0x61, 0x51, 0x49, 0x46};
static const uint8_t glyph_3[5] = {0x22, 0x41, 0x49, 0x49, 0x36};
static const uint8_t glyph_4[5] = {0x18, 0x14, 0x12, 0x7F, 0x10};
static const uint8_t glyph_5[5] = {0x2F, 0x49, 0x49, 0x49, 0x31};
static const uint8_t glyph_6[5] = {0x3C, 0x4A, 0x49, 0x49, 0x30};
static const uint8_t glyph_7[5] = {0x01, 0x01, 0x79, 0x05, 0x03};
static const uint8_t glyph_8[5] = {0x36, 0x49, 0x49, 0x49, 0x36};
static const uint8_t glyph_9[5] = {0x06, 0x49, 0x49, 0x29, 0x1E};
static const uint8_t glyph_colon[5] = {0x00, 0x00, 0x14, 0x00, 0x00};
static const uint8_t glyph_minus[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
static const uint8_t glyph_slash[5] = {0x20, 0x10, 0x08, 0x04, 0x02}; // glcdfont '/'

static const uint8_t *glyph_for(char c)
{
    if (c >= 'A' && c <= 'Z') return font_upper[c - 'A'];
    switch (c) {
        case '0': return glyph_0;
        case '1': return glyph_1;
        case '2': return glyph_2;
        case '3': return glyph_3;
        case '4': return glyph_4;
        case '5': return glyph_5;
        case '6': return glyph_6;
        case '7': return glyph_7;
        case '8': return glyph_8;
        case '9': return glyph_9;
        case ':': return glyph_colon;
        case '-': return glyph_minus;
        case '/': return glyph_slash;
        default:  return NULL; // incl. space: left blank
    }
}

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

esp_err_t display_draw_text(int page, const char *text)
{
    uint8_t row[OLED_WIDTH];
    memset(row, 0x00, sizeof(row));

    int width = (int)strlen(text) * 6 - 1; // 5px glyph + 1px gap, no trailing gap
    int col = (OLED_WIDTH - width) / 2;
    for (const char *p = text; *p && col + 5 <= OLED_WIDTH; p++) {
        const uint8_t *glyph = glyph_for(*p);
        if (glyph) {
            memcpy(&row[col], glyph, 5);
        }
        col += 6; // 5px glyph + 1px gap
    }

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

#define ICON_WIDTH 8
#define ICON_TEXT_GAP 4

esp_err_t display_draw_icon_and_text(int page, const uint8_t *icon, const char *text)
{
    uint8_t row[OLED_WIDTH];
    memset(row, 0x00, sizeof(row));

    int text_len = text ? (int)strlen(text) : 0;
    int text_width = text_len > 0 ? text_len * 6 - 1 : 0;
    int icon_width = icon ? ICON_WIDTH : 0;
    int gap = (icon && text_width > 0) ? ICON_TEXT_GAP : 0;

    int col = (OLED_WIDTH - (icon_width + gap + text_width)) / 2;

    if (icon) {
        memcpy(&row[col], icon, ICON_WIDTH);
        col += icon_width + gap;
    }
    for (const char *p = text; p && *p && col + 5 <= OLED_WIDTH; p++) {
        const uint8_t *glyph = glyph_for(*p);
        if (glyph) {
            memcpy(&row[col], glyph, 5);
        }
        col += 6;
    }

    return ssd1306_write_page(page, row, OLED_WIDTH);
}
