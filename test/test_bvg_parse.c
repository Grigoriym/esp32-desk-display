#include <string.h>
#include "unity.h"
#include "bvg_parse.h"
#include "fixtures.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_parses_fixture_skipping_cancelled(void)
{
    char *json = fixture_read("bvg_ok.json");
    bvg_departures_t d;
    TEST_ASSERT_TRUE(bvg_parse(json, &d));
    TEST_ASSERT_EQUAL_INT(3, d.count); // 4 in the file, one cancelled
    TEST_ASSERT_EQUAL_STRING("U5", d.dep[0].line);
    TEST_ASSERT_EQUAL_STRING("SU HAUPTBAHNHOF", d.dep[0].direction); // '+' can't be drawn
    TEST_ASSERT_EQUAL_INT(22, d.dep[0].hour);
    TEST_ASSERT_EQUAL_INT(41, d.dep[0].minute); // "when" (delay included), not plannedWhen
    TEST_ASSERT_EQUAL_STRING("U FRANKFURTER TOR", d.dep[1].direction);
    TEST_ASSERT_EQUAL_INT(0, d.dep[2].hour); // after midnight
    TEST_ASSERT_EQUAL_INT(5, d.dep[2].minute);
    free(json);
}

static void test_parses_real_response(void)
{
    char *json = fixture_read("bvg_real.json");
    bvg_departures_t d;
    TEST_ASSERT_TRUE(bvg_parse(json, &d));
    TEST_ASSERT_EQUAL_INT(6, d.count);
    TEST_ASSERT_EQUAL_STRING("U5", d.dep[0].line);
    TEST_ASSERT_EQUAL_STRING("HAUPTBAHNHOF", d.dep[0].direction);
    TEST_ASSERT_EQUAL_INT(10, d.dep[0].hour);
    TEST_ASSERT_EQUAL_INT(30, d.dep[0].minute);
    TEST_ASSERT_EQUAL_INT(11, d.dep[5].hour);
    TEST_ASSERT_EQUAL_INT(20, d.dep[5].minute);
    free(json);
}

static void test_caps_at_max_departures(void)
{
    char json[4096] = "{\"departures\":[";
    for (int i = 0; i < BVG_MAX_DEPARTURES + 3; i++) {
        char one[160];
        snprintf(
            one, sizeof(one),
            "%s{\"when\":\"2026-09-24T10:%02d:00+02:00\",\"direction\":\"X\",\"line\":{\"name\":\"U5\"}}",
            i ? "," : "", i);
        strcat(json, one);
    }
    strcat(json, "]}");
    bvg_departures_t d;
    TEST_ASSERT_TRUE(bvg_parse(json, &d));
    TEST_ASSERT_EQUAL_INT(BVG_MAX_DEPARTURES, d.count);
    TEST_ASSERT_EQUAL_INT(BVG_MAX_DEPARTURES - 1, d.dep[BVG_MAX_DEPARTURES - 1].minute);
}

static void test_empty_list_is_success(void)
{
    bvg_departures_t d = {.count = 42};
    TEST_ASSERT_TRUE(bvg_parse("{\"departures\":[]}", &d));
    TEST_ASSERT_EQUAL_INT(0, d.count);
}

static void test_garbage_fails_and_leaves_out_untouched(void)
{
    bvg_departures_t d = {.count = 42};
    TEST_ASSERT_FALSE(bvg_parse("", &d));
    TEST_ASSERT_FALSE(bvg_parse("{\"message\":\"upstream error\"}", &d)); // wrapper error body
    TEST_ASSERT_FALSE(bvg_parse("{\"departures\":[{\"when\":", &d));      // truncated
    TEST_ASSERT_EQUAL_INT(42, d.count);
}

static void test_display_name(void)
{
    char out[22];
    bvg_display_name("U Cottbusser Platz (Berlin)", out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("U COTTBUSSER PLATZ", out);
    bvg_display_name("U H\xC3\xB6now", out, sizeof(out)); // UTF-8 "Hönow"
    TEST_ASSERT_EQUAL_STRING("U HOENOW", out);
    bvg_display_name("Stra\xC3\x9F"
                     "e \xC3\x84 \xC3\x9C",
                     out, sizeof(out)); // "Straße Ä Ü"
    TEST_ASSERT_EQUAL_STRING("STRASSE AE UE", out);
    bvg_display_name("S+U Alexanderplatz Bhf/Dircksenstr.", out, sizeof(out));
    // Clipped at 20 chars, one short of what the buffer could hold (the loop
    // keeps room for a 2-char umlaut); harmless, the title row clips earlier.
    TEST_ASSERT_EQUAL_STRING("SU ALEXANDERPLATZ BH", out);
}

static void test_display_name_never_overflows(void)
{
    char out[4];
    memset(out, 'x', sizeof(out));
    bvg_display_name("\xC3\xB6\xC3\xB6\xC3\xB6", out, sizeof(out)); // "ööö" -> would be OEOEOE
    TEST_ASSERT_TRUE(strlen(out) < sizeof(out));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parses_fixture_skipping_cancelled);
    RUN_TEST(test_parses_real_response);
    RUN_TEST(test_caps_at_max_departures);
    RUN_TEST(test_empty_list_is_success);
    RUN_TEST(test_garbage_fails_and_leaves_out_untouched);
    RUN_TEST(test_display_name);
    RUN_TEST(test_display_name_never_overflows);
    return UNITY_END();
}
