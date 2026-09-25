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
    TEST_ASSERT_EQUAL_INT(14, w.temp_c); // 13.9 rounded
    TEST_ASSERT_EQUAL_INT(3, w.weather_code);
    TEST_ASSERT_EQUAL_INT(10, w.wind_kmh); // 9.9
    TEST_ASSERT_EQUAL_INT(3, w.uv_max);
    TEST_ASSERT_EQUAL_STRING("06:57", w.sunrise);
    TEST_ASSERT_EQUAL_STRING("18:58", w.sunset);
    TEST_ASSERT_EQUAL_INT(-1, w.rain_in_h); // at most 10% all window
    TEST_ASSERT_EQUAL_STRING("", w.rain_from);
    TEST_ASSERT_EQUAL_STRING("", w.rain_until);
    free(json);
}

// A response with the given hourly precipitation_probability array (as JSON
// text), for n hours from 13:00.
static bool parse_with_probs(const char *probs, int n, weather_t *w)
{
    char json[1024];
    int len = snprintf(json, sizeof(json),
                       "{\"current_weather\":{\"temperature\":14,\"weathercode\":3,\"windspeed\":10},"
                       "\"daily\":{\"sunrise\":[\"2026-09-25T06:57\"],\"sunset\":[\"2026-09-25T18:58\"],"
                       "\"uv_index_max\":[3]},\"hourly\":{\"time\":[");
    for (int i = 0; i < n; i++) {
        len += snprintf(json + len, sizeof(json) - len, "%s\"2026-09-%02dT%02d:00\"", i ? "," : "",
                        25 + (13 + i) / 24, (13 + i) % 24);
    }
    snprintf(json + len, sizeof(json) - len, "],\"precipitation_probability\":[%s]}}", probs);
    return weather_parse(json, w);
}

static void test_rain_later(void)
{
    weather_t w;
    TEST_ASSERT_TRUE(parse_with_probs("0,10,49,50,80,30", 6, &w)); // 50 counts, 49 doesn't
    TEST_ASSERT_EQUAL_INT(3, w.rain_in_h);
    TEST_ASSERT_EQUAL_STRING("16:00", w.rain_from);
    TEST_ASSERT_EQUAL_STRING("18:00", w.rain_until);
}

static void test_rain_now_until_end_of_window(void)
{
    weather_t w;
    TEST_ASSERT_TRUE(parse_with_probs("90,90,70,60", 4, &w));
    TEST_ASSERT_EQUAL_INT(0, w.rain_in_h);
    TEST_ASSERT_EQUAL_STRING("13:00", w.rain_from);
    TEST_ASSERT_EQUAL_STRING("", w.rain_until);
}

static void test_rain_over_midnight_and_null_is_dry(void)
{
    weather_t w;
    TEST_ASSERT_TRUE(parse_with_probs("0,0,0,0,0,0,0,0,0,0,0,60,null", 13, &w));
    TEST_ASSERT_EQUAL_INT(11, w.rain_in_h);
    TEST_ASSERT_EQUAL_STRING("00:00", w.rain_from);
    TEST_ASSERT_EQUAL_STRING("01:00", w.rain_until);
}

static void test_bad_hourly_fails(void)
{
    weather_t w;
    TEST_ASSERT_FALSE(parse_with_probs("0,0", 3, &w));       // length mismatch
    TEST_ASSERT_FALSE(parse_with_probs("", 0, &w));          // empty
    TEST_ASSERT_FALSE(parse_with_probs("0,\"x\",0", 3, &w)); // junk value
}

static void test_rounds_negative_temperature(void)
{
    const char *json = "{\"current_weather\":{\"temperature\":-2.5,\"weathercode\":71,\"windspeed\":0.4},"
                       "\"daily\":{\"sunrise\":[\"2026-12-21T08:14\"],\"sunset\":[\"2026-12-21T15:54\"],"
                       "\"uv_index_max\":[0.45]},\"hourly\":{\"time\":[\"2026-12-21T09:00\"],"
                       "\"precipitation_probability\":[0]}}";
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
                       "\"daily\":{\"sunrise\":[\"06:57\"],\"sunset\":[\"18:58\"],\"uv_index_max\":[1]},"
                       "\"hourly\":{\"time\":[\"2026-09-25T13:00\"],\"precipitation_probability\":[0]}}";
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
    RUN_TEST(test_rain_later);
    RUN_TEST(test_rain_now_until_end_of_window);
    RUN_TEST(test_rain_over_midnight_and_null_is_dry);
    RUN_TEST(test_bad_hourly_fails);
    RUN_TEST(test_icon_mapping);
    RUN_TEST(test_icon_art);
    return UNITY_END();
}
