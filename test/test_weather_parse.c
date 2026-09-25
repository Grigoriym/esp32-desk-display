#include <string.h>
#include "unity.h"
#include "weather_parse.h"
#include "fixtures.h"
#include "art.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_parses_real_response(void)
{
    char *json = fixture_read("weather_ok.json");
    weather_t w;
    TEST_ASSERT_TRUE(weather_parse(json, &w));
    TEST_ASSERT_EQUAL_INT(13, w.temp_c); // 12.6 rounded
    TEST_ASSERT_EQUAL_INT(3, w.weather_code);
    TEST_ASSERT_EQUAL_INT(9, w.wind_kmh);
    TEST_ASSERT_EQUAL_INT(3, w.uv_max);
    TEST_ASSERT_EQUAL_STRING("06:57", w.sunrise);
    TEST_ASSERT_EQUAL_STRING("18:58", w.sunset);
    free(json);
}

static void test_rounds_negative_temperature(void)
{
    const char *json = "{\"current_weather\":{\"temperature\":-2.5,\"weathercode\":71,\"windspeed\":0.4},"
                       "\"daily\":{\"sunrise\":[\"2026-12-21T08:14\"],\"sunset\":[\"2026-12-21T15:54\"],"
                       "\"uv_index_max\":[0.45]}}";
    weather_t w;
    TEST_ASSERT_TRUE(weather_parse(json, &w));
    TEST_ASSERT_EQUAL_INT(-3, w.temp_c); // lround: halves away from zero
    TEST_ASSERT_EQUAL_INT(0, w.wind_kmh);
    TEST_ASSERT_EQUAL_INT(0, w.uv_max);
    TEST_ASSERT_EQUAL_STRING("08:14", w.sunrise);
}

static void test_missing_field_fails_and_leaves_out_untouched(void)
{
    const char *json = "{\"current_weather\":{\"temperature\":20,\"weathercode\":0,\"windspeed\":5},"
                       "\"daily\":{\"sunrise\":[\"2026-09-25T06:57\"],\"sunset\":[\"2026-09-25T18:58\"]}}";
    weather_t w = {.temp_c = 99};
    TEST_ASSERT_FALSE(weather_parse(json, &w));
    TEST_ASSERT_EQUAL_INT(99, w.temp_c);
}

static void test_garbage_fails(void)
{
    weather_t w;
    TEST_ASSERT_FALSE(weather_parse("", &w));
    TEST_ASSERT_FALSE(weather_parse("{\"error\":true,\"reason\":\"x\"}", &w));
    TEST_ASSERT_FALSE(weather_parse("{\"current_weather\":", &w)); // truncated body
}

static void test_sun_time_without_T_fails(void)
{
    const char *json = "{\"current_weather\":{\"temperature\":20,\"weathercode\":0,\"windspeed\":5},"
                       "\"daily\":{\"sunrise\":[\"06:57\"],\"sunset\":[\"18:58\"],\"uv_index_max\":[1]}}";
    weather_t w;
    TEST_ASSERT_FALSE(weather_parse(json, &w));
}

static void test_icon_mapping(void)
{
    const uint8_t *sun = weather_icon_for_code(0);
    const uint8_t *cloud = weather_icon_for_code(3);
    const uint8_t *rain = weather_icon_for_code(61);
    const uint8_t *snow = weather_icon_for_code(71);
    const uint8_t *storm = weather_icon_for_code(95);
    TEST_ASSERT_EQUAL_PTR(sun, weather_icon_for_code(1));
    TEST_ASSERT_EQUAL_PTR(cloud, weather_icon_for_code(45));
    TEST_ASSERT_EQUAL_PTR(rain, weather_icon_for_code(82));
    TEST_ASSERT_EQUAL_PTR(snow, weather_icon_for_code(86));
    TEST_ASSERT_EQUAL_PTR(storm, weather_icon_for_code(99));
    TEST_ASSERT_EQUAL_PTR(cloud, weather_icon_for_code(1234)); // unknown code
    // All five are distinct bitmaps.
    const uint8_t *all[] = {sun, cloud, rain, snow, storm};
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) TEST_ASSERT_TRUE(all[i] != all[j]);
    }
}

// The icons as they should look on the panel (8x8, hand-derived).
static void test_icon_art(void)
{
    // clang-format off
    assert_art("sun", weather_icon_for_code(0), 8, 8, (const char *const[]){
        "..####..", ".######.", "########", "########",
        "########", "########", ".######.", "..####..",
    });
    assert_art("cloud", weather_icon_for_code(3), 8, 8, (const char *const[]){
        "........", "..####..", ".######.", "########",
        "########", "........", "........", "........",
    });
    assert_art("rain", weather_icon_for_code(61), 8, 8, (const char *const[]){
        "........", "..####..", ".######.", "########",
        "########", ".#.#.#..", "..#.#.#.", "........",
    });
    assert_art("snow", weather_icon_for_code(71), 8, 8, (const char *const[]){
        "........", "..####..", ".######.", "########",
        "########", ".#.#.#..", "#.#.#.#.", ".#.#.#..",
    });
    assert_art("storm", weather_icon_for_code(95), 8, 8, (const char *const[]){
        "........", "..####..", ".######.", "########",
        "########", "...##...", "..##....", ".##.....",
    });
    // clang-format on
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parses_real_response);
    RUN_TEST(test_rounds_negative_temperature);
    RUN_TEST(test_missing_field_fails_and_leaves_out_untouched);
    RUN_TEST(test_garbage_fails);
    RUN_TEST(test_sun_time_without_T_fails);
    RUN_TEST(test_icon_mapping);
    RUN_TEST(test_icon_art);
    return UNITY_END();
}
