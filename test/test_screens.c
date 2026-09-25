#include <string.h>
#include "unity.h"
#include "screens.h"

#define WALK_MIN     6
#define WALK_COMFORT 11

static screen_data_t data;
static screen_rows_t rows;

void setUp(void)
{
    memset(&data, 0, sizeof(data));
    data.bvg_walk_min = WALK_MIN;
    data.bvg_walk_comfort = WALK_COMFORT;
}

void tearDown(void)
{
}

#define LEFT(page)  rows.text[page][0]
#define RIGHT(page) rows.text[page][1]

static void add_departure(int hour, int minute)
{
    bvg_departure_t *d = &data.bvg.dep[data.bvg.count++];
    strcpy(d->line, "U5");
    strcpy(d->direction, "HAUPTBAHNHOF");
    d->hour = hour;
    d->minute = minute;
}

static int at(int hour, int minute)
{
    return hour * 60 + minute;
}

// --- minutes_until ---

static void test_minutes_until(void)
{
    TEST_ASSERT_EQUAL_INT(15, screen_minutes_until(at(10, 0), 10, 15));
    TEST_ASSERT_EQUAL_INT(-2, screen_minutes_until(at(10, 0), 9, 58));
    TEST_ASSERT_EQUAL_INT(10, screen_minutes_until(at(23, 55), 0, 5)); // over midnight
    TEST_ASSERT_EQUAL_INT(-5, screen_minutes_until(at(0, 2), 23, 57)); // just gone, before midnight
}

// --- BVG ---

static void test_bvg_loading_then_no_data(void)
{
    screen_layout(SCREEN_BVG, &data, at(10, 0), &rows);
    TEST_ASSERT_EQUAL_STRING("BVG", LEFT(2));
    TEST_ASSERT_EQUAL_STRING("LOADING", LEFT(4));
    data.bvg_failed = true;
    screen_layout(SCREEN_BVG, &data, at(10, 0), &rows);
    TEST_ASSERT_EQUAL_STRING("NO DATA", LEFT(4));
}

static void test_bvg_leave_hint_boundaries(void)
{
    data.bvg_ok = true;
    add_departure(10, 12); // 12 min away: one to spare
    screen_layout(SCREEN_BVG, &data, at(10, 0), &rows);
    TEST_ASSERT_EQUAL_STRING("U5 HAUPTBAHNHOF", LEFT(2));
    TEST_ASSERT_EQUAL_STRING("LEAVE IN 1", LEFT(3));
    TEST_ASSERT_EQUAL_STRING("10:12", LEFT(5));
    TEST_ASSERT_EQUAL_STRING("12 MIN", RIGHT(5));

    screen_layout(SCREEN_BVG, &data, at(10, 1), &rows); // 11 left
    TEST_ASSERT_EQUAL_STRING("GO NOW", LEFT(3));
    screen_layout(SCREEN_BVG, &data, at(10, 2), &rows); // 10 left
    TEST_ASSERT_EQUAL_STRING("HURRY", LEFT(3));
    screen_layout(SCREEN_BVG, &data, at(10, 6), &rows); // 6 left: still shown
    TEST_ASSERT_EQUAL_STRING("HURRY", LEFT(3));
    TEST_ASSERT_EQUAL_STRING("6 MIN", RIGHT(5));
}

static void test_bvg_hides_uncatchable_and_caps_rows(void)
{
    data.bvg_ok = true;
    add_departure(10, 3); // 5 min: can't make it, hidden
    add_departure(10, 8);
    add_departure(10, 13);
    add_departure(10, 18);
    add_departure(10, 23); // 4th catchable: no room
    screen_layout(SCREEN_BVG, &data, at(10, 0), &rows);
    TEST_ASSERT_EQUAL_STRING("HURRY", LEFT(3)); // hint is for the first *shown* train
    TEST_ASSERT_EQUAL_STRING("10:08", LEFT(5));
    TEST_ASSERT_EQUAL_STRING("10:13", LEFT(6));
    TEST_ASSERT_EQUAL_STRING("10:18", LEFT(7));
    TEST_ASSERT_EQUAL_STRING("", LEFT(4));
}

static void test_bvg_no_trains(void)
{
    data.bvg_ok = true;
    screen_layout(SCREEN_BVG, &data, at(10, 0), &rows);
    TEST_ASSERT_EQUAL_STRING("BVG", LEFT(2));
    TEST_ASSERT_EQUAL_STRING("NO TRAINS", LEFT(4));

    add_departure(10, 2); // only uncatchable ones
    screen_layout(SCREEN_BVG, &data, at(10, 0), &rows);
    TEST_ASSERT_EQUAL_STRING("U5 HAUPTBAHNHOF", LEFT(2));
    TEST_ASSERT_EQUAL_STRING("NO TRAINS", LEFT(4));
    TEST_ASSERT_EQUAL_STRING("", LEFT(3));
}

// --- other screens ---

static void test_home(void)
{
    screen_layout(SCREEN_HOME, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("--C", LEFT(3));
    TEST_ASSERT_EQUAL_STRING("", LEFT(5));

    data.weather_ok = true;
    data.weather.temp_c = -4;
    data.indoor_ok = true;
    data.indoor.temp_c = 22.5f; // lroundf: halves away from zero
    data.indoor.humidity_pct = 43.4f;
    screen_layout(SCREEN_HOME, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("-4C", LEFT(3));
    TEST_ASSERT_EQUAL_STRING("IN 23C 43H", LEFT(5));
}

static void test_home_rain(void)
{
    screen_layout(SCREEN_HOME, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("", LEFT(7)); // no weather yet

    data.weather_ok = true;
    data.weather = (weather_t){.rain_in_h = -1};
    screen_layout(SCREEN_HOME, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("NO RAIN 12H", LEFT(7));

    data.weather = (weather_t){.rain_in_h = 3, .rain_from = "16:00", .rain_until = "18:00"};
    screen_layout(SCREEN_HOME, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("RAIN 16:00", LEFT(7));

    data.weather = (weather_t){.rain_in_h = 0, .rain_from = "13:00", .rain_until = "15:00"};
    screen_layout(SCREEN_HOME, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("RAIN TILL 15:00", LEFT(7));

    data.weather = (weather_t){.rain_in_h = 0, .rain_from = "13:00"};
    screen_layout(SCREEN_HOME, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("RAIN NEXT 12H", LEFT(7));
}

static void test_outdoor_and_indoor(void)
{
    screen_layout(SCREEN_OUTDOOR, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("OUTDOOR", LEFT(2));
    TEST_ASSERT_EQUAL_STRING("--", LEFT(4));

    data.weather_ok = true;
    data.weather = (weather_t){.wind_kmh = 9, .uv_max = 3, .sunrise = "06:57", .sunset = "18:58"};
    screen_layout(SCREEN_OUTDOOR, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("RISE 06:57", LEFT(4));
    TEST_ASSERT_EQUAL_STRING("SET 18:58", RIGHT(4));
    TEST_ASSERT_EQUAL_STRING("WIND 9KMH", LEFT(6));
    TEST_ASSERT_EQUAL_STRING("UV 3", RIGHT(6));

    data.indoor_ok = true;
    data.indoor = (bme280_reading_t){.temp_c = 23.3f, .humidity_pct = 47.0f, .pressure_hpa = 1014.1f};
    screen_layout(SCREEN_INDOOR, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("INDOOR", LEFT(2));
    TEST_ASSERT_EQUAL_STRING("TEMP 23C", LEFT(4));
    TEST_ASSERT_EQUAL_STRING("HUM 47H", RIGHT(4));
    TEST_ASSERT_EQUAL_STRING("1014 HPA", LEFT(6));
}

static void test_air(void)
{
    screen_layout(SCREEN_AIR, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("AIR", LEFT(2));
    TEST_ASSERT_EQUAL_STRING("--", LEFT(4));

    data.air_ok = true;
    data.air = (air_t){.aqi = 23};
    screen_layout(SCREEN_AIR, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("AQI 23", LEFT(4));
    TEST_ASSERT_EQUAL_STRING("FAIR", RIGHT(4));
    TEST_ASSERT_EQUAL_STRING("POLLEN NONE", LEFT(6));
    TEST_ASSERT_EQUAL_STRING("", LEFT(7));

    // Only the two strongest are shown, strongest first.
    data.air.pollen[POLLEN_ALDER] = 5;
    data.air.pollen[POLLEN_BIRCH] = 120;
    data.air.pollen[POLLEN_GRASS] = 30;
    screen_layout(SCREEN_AIR, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("BIRCH", LEFT(6));
    TEST_ASSERT_EQUAL_STRING("HIGH", RIGHT(6));
    TEST_ASSERT_EQUAL_STRING("GRASS", LEFT(7));
    TEST_ASSERT_EQUAL_STRING("MED", RIGHT(7));

    // A single one leaves page 7 blank.
    data.air = (air_t){.aqi = 23, .pollen[POLLEN_RAGWEED] = 1};
    screen_layout(SCREEN_AIR, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("RAGWEED", LEFT(6));
    TEST_ASSERT_EQUAL_STRING("LOW", RIGHT(6));
    TEST_ASSERT_EQUAL_STRING("", LEFT(7));
}

static void test_previous_screen_leaves_nothing_behind(void)
{
    data.indoor_ok = true;
    screen_layout(SCREEN_INDOOR, &data, 0, &rows);
    screen_layout(SCREEN_HOME, &data, 0, &rows);
    TEST_ASSERT_EQUAL_STRING("", LEFT(2));
    TEST_ASSERT_EQUAL_STRING("", RIGHT(4));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_minutes_until);
    RUN_TEST(test_bvg_loading_then_no_data);
    RUN_TEST(test_bvg_leave_hint_boundaries);
    RUN_TEST(test_bvg_hides_uncatchable_and_caps_rows);
    RUN_TEST(test_bvg_no_trains);
    RUN_TEST(test_home);
    RUN_TEST(test_home_rain);
    RUN_TEST(test_outdoor_and_indoor);
    RUN_TEST(test_air);
    RUN_TEST(test_previous_screen_leaves_nothing_behind);
    return UNITY_END();
}
