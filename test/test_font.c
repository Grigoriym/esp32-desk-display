#include <string.h>
#include "unity.h"
#include "font.h"
#include "art.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void assert_glyph(char c, const char *const rows[7])
{
    char name[] = "glyph ' '";
    name[7] = c;
    const uint8_t *g = font_glyph(c);
    TEST_ASSERT_NOT_NULL_MESSAGE(g, name);
    assert_art(name, g, FONT_WIDTH, 7, rows);
}

// Row 7 (the 8th pixel row) is the gap between text lines: must stay dark.
static void assert_bottom_row_blank(char c)
{
    const uint8_t *g = font_glyph(c);
    const char name[] = {c, '\0'};
    for (int x = 0; x < FONT_WIDTH; x++) {
        TEST_ASSERT_EQUAL_HEX8_MESSAGE(0, g[x] & 0x80, name);
    }
}

// --- the hand-derived glyphs, as they should look on the panel ---

// clang-format off
static void test_digits(void)
{
    assert_glyph('0', (const char *const[]){".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."});
    assert_glyph('1', (const char *const[]){"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."});
    assert_glyph('2', (const char *const[]){".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"});
    assert_glyph('3', (const char *const[]){".###.", "#...#", "....#", "..##.", "....#", "#...#", ".###."});
    assert_glyph('4', (const char *const[]){"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."});
    assert_glyph('5', (const char *const[]){"#####", "#....", "#....", "####.", "....#", "#...#", ".###."});
    assert_glyph('6', (const char *const[]){"..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###."});
    assert_glyph('7', (const char *const[]){"#####", "....#", "...#.", "..#..", "..#..", "..#..", "..#.."});
    assert_glyph('8', (const char *const[]){".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."});
    assert_glyph('9', (const char *const[]){".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.."});
}

static void test_symbols(void)
{
    assert_glyph(':', (const char *const[]){".....", ".....", "..#..", ".....", "..#..", ".....", "....."});
    assert_glyph('-', (const char *const[]){".....", ".....", ".....", "#####", ".....", ".....", "....."});
    assert_glyph('/', (const char *const[]){".....", "....#", "...#.", "..#..", ".#...", "#....", "....."});
}

// Letters are the stock glcdfont; spot-check a few, incl. this project's C.
static void test_letters_spot_check(void)
{
    assert_glyph('A', (const char *const[]){"..#..", ".#.#.", "#...#", "#...#", "#####", "#...#", "#...#"});
    assert_glyph('C', (const char *const[]){".###.", "#....", "#....", "#....", "#....", "#....", ".###."});
    assert_glyph('M', (const char *const[]){"#...#", "##.##", "#.#.#", "#.#.#", "#.#.#", "#...#", "#...#"});
}
// clang-format on

static void test_coverage(void)
{
    const char *covered = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-/";
    for (const char *c = covered; *c; c++) {
        const char name[] = {*c, '\0'};
        TEST_ASSERT_NOT_NULL_MESSAGE(font_glyph(*c), name);
        assert_bottom_row_blank(*c);
    }
    // Everything else draws as blank (lowercase, space, what the font lacks).
    TEST_ASSERT_NULL(font_glyph(' '));
    TEST_ASSERT_NULL(font_glyph('a'));
    TEST_ASSERT_NULL(font_glyph('%'));
    TEST_ASSERT_NULL(font_glyph('.'));
}

// --- layout helpers ---

static void test_text_width(void)
{
    TEST_ASSERT_EQUAL_INT(0, font_text_width(NULL));
    TEST_ASSERT_EQUAL_INT(0, font_text_width(""));
    TEST_ASSERT_EQUAL_INT(5, font_text_width("A"));
    TEST_ASSERT_EQUAL_INT(29, font_text_width("12:34")); // 5 glyphs, 4 gaps
}

static void test_blit_places_glyphs_with_gap(void)
{
    uint8_t row[16] = {0};
    font_blit(row, sizeof(row), 1, "1-");
    // clang-format off
    assert_art("blit '1-' at col 1", row, sizeof(row), 7, (const char *const[]){
        "...#............",
        "..##............",
        "...#............",
        "...#...#####....",
        "...#............",
        "...#............",
        "..###...........",
    });
    // clang-format on
}

static void test_blit_clips_at_both_edges(void)
{
    uint8_t row[12] = {0};
    font_blit(row, sizeof(row), -3, "--"); // cols -3..1 dropped, 3..7 drawn
    font_blit(row, sizeof(row), 8, "-");   // cols 8..12 cross the edge: dropped
    TEST_ASSERT_EACH_EQUAL_HEX8(0, row, 3);
    TEST_ASSERT_EACH_EQUAL_HEX8(0x08, &row[3], 5);
    TEST_ASSERT_EACH_EQUAL_HEX8(0, &row[8], 4);
}

static void test_blit_null_and_empty(void)
{
    uint8_t row[8] = {0};
    font_blit(row, sizeof(row), 0, NULL);
    font_blit(row, sizeof(row), 0, "");
    font_blit(row, sizeof(row), 0, "  ");
    TEST_ASSERT_EACH_EQUAL_HEX8(0, row, sizeof(row));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_digits);
    RUN_TEST(test_symbols);
    RUN_TEST(test_letters_spot_check);
    RUN_TEST(test_coverage);
    RUN_TEST(test_text_width);
    RUN_TEST(test_blit_places_glyphs_with_gap);
    RUN_TEST(test_blit_clips_at_both_edges);
    RUN_TEST(test_blit_null_and_empty);
    return UNITY_END();
}
