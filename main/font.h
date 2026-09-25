#pragma once

// 5x7 font as column bitmaps, no ESP-IDF dependencies, so it also compiles
// on the PC for the unit tests in test/ (which render it as ASCII art).
// Covers A-Z, 0-9, ':', '-' and '/'; anything else renders as blank.

#include <stdint.h>

#define FONT_WIDTH   5 // px per glyph
#define FONT_ADVANCE 6 // glyph + 1px gap

// 5 bytes, one per column, bit0 = top row; NULL for a blank character.
const uint8_t *font_glyph(char c);

// Width in px of text drawn with font_blit(), no trailing gap. 0 for NULL/"".
int font_text_width(const char *text);

// Copies text into a row_width-byte page row starting at col (may be
// negative); glyphs that would cross either edge are dropped.
void font_blit(uint8_t *row, int row_width, int col, const char *text);
