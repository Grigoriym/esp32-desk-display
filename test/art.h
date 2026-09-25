#pragma once

// Column bitmaps (one byte per column, bit0 = top row, as the OLED takes
// them) as ASCII art, so a glyph or icon can be checked by eye before
// flashing. '#' = lit pixel, '.' = dark.

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"

#define ART_MAX_W 16
#define ART_MAX_H 8

// Row y of the bitmap as "#..#."-style text into out (width + 1 bytes).
static inline void art_row(const uint8_t *cols, int width, int y, char *out)
{
    for (int x = 0; x < width; x++) {
        out[x] = (cols && (cols[x] >> y & 1)) ? '#' : '.';
    }
    out[width] = '\0';
}

// Compares the bitmap against one expected string per row. On a mismatch
// prints the whole picture as drawn, next to what was expected.
static inline void assert_art(const char *name, const uint8_t *cols, int width, int height,
                              const char *const *expected)
{
    char row[ART_MAX_W + 1];
    int bad = 0;
    for (int y = 0; y < height; y++) {
        art_row(cols, width, y, row);
        bad |= strcmp(row, expected[y]) != 0;
    }
    if (!bad) return;
    printf("\n%s: drawn vs expected\n", name);
    for (int y = 0; y < height; y++) {
        art_row(cols, width, y, row);
        printf("  %s   %s%s\n", row, expected[y], strcmp(row, expected[y]) ? "  <--" : "");
    }
    TEST_FAIL_MESSAGE(name);
}
