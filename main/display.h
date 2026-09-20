#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

// SSD1315/SSD1306 128x64 OLED, page-addressing mode.
// Font currently only covers the letters this project has needed so far
// (H, E, L, O) -- extend as later milestones need more characters
// (e.g. digits for the clock in milestone 3).

// Probes the bus for the display at 0x3C/0x3D, runs the init sequence.
esp_err_t display_init(i2c_master_bus_handle_t bus);

// Blanks the whole panel.
esp_err_t display_clear(void);

// Draws text centered on the given page (0-7), 8px rows each.
esp_err_t display_draw_text(int page, const char *text);

// Draws an 8x8 bitmap (column-major, one byte per column) centered on the
// given page. icon may be NULL to blank the page instead.
esp_err_t display_draw_icon(int page, const uint8_t *icon);

// Draws an 8x8 icon immediately followed by text, the pair centered together
// on the given page. Either icon or text may be NULL/empty to draw just the
// other one.
esp_err_t display_draw_icon_and_text(int page, const uint8_t *icon, const char *text);
