#include <string.h>
#include "unity.h"
#include "cJSON.h"
#include "fixtures.h"
#include "web_api.h"

static screen_data_t data;
static web_view_t view;
static char buf[WEB_STATUS_MAX];
static cJSON *json;

void setUp(void)
{
    memset(&data, 0, sizeof(data));
    view = (web_view_t){.screen = SCREEN_HOME,
                        .panel_on = true,
                        .time = "12:05",
                        .date = "2026-09-26",
                        .now_min = 12 * 60 + 5};
    json = NULL;
}

void tearDown(void)
{
    cJSON_Delete(json);
}

// Formats, checks the length and parses it back.
static cJSON *status(void)
{
    int n = web_status_json(&data, &view, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
    json = cJSON_Parse(buf);
    TEST_ASSERT_NOT_NULL_MESSAGE(json, buf);
    return json;
}

static cJSON *get(const cJSON *o, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(o, key);
    TEST_ASSERT_NOT_NULL_MESSAGE(item, key);
    return item;
}

static void test_sections_null_before_any_data(void)
{
    cJSON *j = status();
    TEST_ASSERT_EQUAL_STRING("12:05", get(j, "time")->valuestring);
    TEST_ASSERT_EQUAL_STRING("2026-09-26", get(j, "date")->valuestring);
    TEST_ASSERT_EQUAL_STRING("home", get(j, "screen")->valuestring);
    TEST_ASSERT_TRUE(cJSON_IsTrue(get(j, "panel_on")));
    const char *sections[] = {"outdoor", "indoor", "co2", "air", "warning", "next_holiday", "bvg"};
    for (size_t i = 0; i < sizeof(sections) / sizeof(sections[0]); i++) {
        TEST_ASSERT_TRUE_MESSAGE(cJSON_IsNull(get(j, sections[i])), sections[i]);
    }
}

static void test_screen_and_panel(void)
{
    view.screen = SCREEN_BVG;
    view.panel_on = false;
    cJSON *j = status();
    TEST_ASSERT_EQUAL_STRING("bvg", get(j, "screen")->valuestring);
    TEST_ASSERT_TRUE(cJSON_IsFalse(get(j, "panel_on")));
}

static void test_outdoor_indoor_air(void)
{
    data.weather_ok = true;
    data.weather = (weather_t){.temp_c = 15,
                               .weather_code = 3,
                               .wind_kmh = 4,
                               .uv_max = 3,
                               .sunrise = "07:01",
                               .sunset = "19:02",
                               .rain_in_h = 2,
                               .rain_from = "14:00",
                               .rain_until = "16:00"};
    data.indoor_ok = true;
    data.indoor = (bme280_reading_t){.temp_c = 22.44f, .humidity_pct = 43.06f, .pressure_hpa = 1012.25f};
    data.co2_ok = true;
    data.co2.co2_ppm = 812;
    data.air_ok = true;
    data.air = (air_t){.aqi = 23, .pollen = {[POLLEN_GRASS] = 12}};
    cJSON *j = status();
    TEST_ASSERT_EQUAL_INT(812, get(get(j, "co2"), "ppm")->valueint);

    cJSON *o = get(j, "outdoor");
    TEST_ASSERT_EQUAL_INT(15, get(o, "temp_c")->valueint);
    TEST_ASSERT_EQUAL_INT(3, get(o, "weather_code")->valueint);
    TEST_ASSERT_EQUAL_STRING("07:01", get(o, "sunrise")->valuestring);
    TEST_ASSERT_EQUAL_STRING("19:02", get(o, "sunset")->valuestring);
    cJSON *rain = get(o, "rain");
    TEST_ASSERT_EQUAL_INT(2, get(rain, "in_h")->valueint);
    TEST_ASSERT_EQUAL_STRING("14:00", get(rain, "from")->valuestring);
    TEST_ASSERT_EQUAL_STRING("16:00", get(rain, "until")->valuestring);

    // One decimal, printed without float noise.
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"temp_c\":22.4,"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"humidity_pct\":43.1,"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"pressure_hpa\":1012.3}"));

    cJSON *a = get(j, "air");
    TEST_ASSERT_EQUAL_INT(23, get(a, "aqi")->valueint);
    TEST_ASSERT_EQUAL_STRING(air_aqi_label(23), get(a, "aqi_label")->valuestring);
    TEST_ASSERT_EQUAL_INT(12, get(get(a, "pollen"), "grass")->valueint);
    TEST_ASSERT_EQUAL_INT(0, get(get(a, "pollen"), "ragweed")->valueint);
}

static void test_warning(void)
{
    data.alerts_ok = true;
    TEST_ASSERT_EQUAL_INT(0, get(get(status(), "warning"), "count")->valueint);
    TEST_ASSERT_NULL(cJSON_GetObjectItem(get(json, "warning"), "event"));
    cJSON_Delete(json);

    data.alerts = (alerts_t){
        .count = 2, .started = false, .severity = ALERT_MODERATE, .event = "HEAVY RAIN", .onset = "18:00"};
    cJSON *w = get(status(), "warning");
    TEST_ASSERT_EQUAL_INT(2, get(w, "count")->valueint);
    TEST_ASSERT_EQUAL_STRING("HEAVY RAIN", get(w, "event")->valuestring);
    TEST_ASSERT_EQUAL_STRING("moderate", get(w, "severity")->valuestring);
    TEST_ASSERT_TRUE(cJSON_IsFalse(get(w, "started")));
    TEST_ASSERT_EQUAL_STRING("18:00", get(w, "onset")->valuestring);
}

static void test_next_holiday(void)
{
    data.today_ymd = 20260926;
    data.holidays = (holidays_t){
        .year = 2026, .count = 2, .day = {{20260501, "LABOUR DAY"}, {20261003, "GERMAN UNITY DAY"}}};
    cJSON *h = get(status(), "next_holiday");
    TEST_ASSERT_EQUAL_STRING("2026-10-03", get(h, "date")->valuestring);
    TEST_ASSERT_EQUAL_STRING("GERMAN UNITY DAY", get(h, "name")->valuestring);
}

static void test_bvg_skips_departed(void)
{
    data.bvg_ok = true;
    data.bvg_walk_min = 6;
    data.bvg_walk_comfort = 11;
    data.bvg = (bvg_departures_t){.count = 3,
                                  .dep = {{"U5", "HAUPTBAHNHOF", 12, 4},
                                          {"U5", "HAUPTBAHNHOF", 12, 9},
                                          {"U5", "HAUPTBAHNHOF", 12, 19}}};
    cJSON *b = get(status(), "bvg");
    TEST_ASSERT_EQUAL_INT(6, get(b, "walk_min")->valueint);
    cJSON *deps = get(b, "departures");
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(deps));
    cJSON *first = cJSON_GetArrayItem(deps, 0);
    TEST_ASSERT_EQUAL_STRING("U5", get(first, "line")->valuestring);
    TEST_ASSERT_EQUAL_STRING("HAUPTBAHNHOF", get(first, "direction")->valuestring);
    TEST_ASSERT_EQUAL_STRING("12:09", get(first, "time")->valuestring);
    TEST_ASSERT_EQUAL_INT(4, get(first, "in_min")->valueint);
    TEST_ASSERT_EQUAL_INT(14, get(cJSON_GetArrayItem(deps, 1), "in_min")->valueint);
}

// The firmware's WEB_STATUS_MAX buffer must hold every section at its longest.
static void test_worst_case_fits(void)
{
    data.weather_ok = data.indoor_ok = data.co2_ok = data.air_ok = data.alerts_ok = data.bvg_ok = true;
    data.co2.co2_ppm = -2147483647;
    data.weather = (weather_t){.temp_c = -2147483647,
                               .weather_code = -2147483647,
                               .wind_kmh = -2147483647,
                               .uv_max = -2147483647,
                               .rain_in_h = -2147483647};
    strcpy(data.weather.sunrise, "00:00");
    strcpy(data.weather.sunset, "00:00");
    strcpy(data.weather.rain_from, "00:00");
    strcpy(data.weather.rain_until, "00:00");
    data.indoor = (bme280_reading_t){.temp_c = -40.1f, .humidity_pct = 100.0f, .pressure_hpa = 1100.1f};
    data.air.aqi = -2147483647;
    for (int i = 0; i < POLLEN_COUNT; i++) data.air.pollen[i] = -2147483647;
    data.alerts = (alerts_t){.count = -2147483647, .severity = ALERT_EXTREME, .onset = "00:00"};
    memset(data.alerts.event, '"', sizeof(data.alerts.event) - 1); // escaped: twice as long
    data.today_ymd = 20260101;
    data.holidays = (holidays_t){.year = 2026, .count = 1, .day = {{20261231, ""}}};
    memset(data.holidays.day[0].name, '"', sizeof(data.holidays.day[0].name) - 1);
    data.bvg_walk_min = data.bvg_walk_comfort = -2147483647;
    data.bvg.count = BVG_MAX_DEPARTURES;
    for (int i = 0; i < BVG_MAX_DEPARTURES; i++) {
        bvg_departure_t *d = &data.bvg.dep[i];
        memset(d->line, '"', sizeof(d->line) - 1);
        memset(d->direction, '"', sizeof(d->direction) - 1);
        d->hour = 12;
        d->minute = 59;
    }
    TEST_ASSERT_NOT_NULL(status());
    TEST_ASSERT_LESS_THAN_INT(WEB_STATUS_MAX, (int)strlen(buf));
}

// docs/api/status.example.json is the API contract the phone app is built
// against (docs/API.md): the firmware must produce exactly that for this data.
static void test_matches_documented_example(void)
{
    view = (web_view_t){.screen = SCREEN_HOME,
                        .panel_on = true,
                        .time = "17:42",
                        .date = "2026-09-26",
                        .now_min = 17 * 60 + 42};
    data.weather_ok = true;
    data.weather = (weather_t){.temp_c = 14,
                               .weather_code = 61,
                               .wind_kmh = 18,
                               .uv_max = 3,
                               .sunrise = "06:58",
                               .sunset = "18:55",
                               .rain_in_h = 0,
                               .rain_from = "17:00",
                               .rain_until = "20:00"};
    data.indoor_ok = true;
    data.indoor = (bme280_reading_t){.temp_c = 23.08f, .humidity_pct = 44.93f, .pressure_hpa = 1014.21f};
    data.co2_ok = true;
    data.co2.co2_ppm = 863;
    data.air_ok = true;
    data.air = (air_t){.aqi = 31, .pollen = {[POLLEN_GRASS] = 12, [POLLEN_MUGWORT] = 3}};
    data.alerts_ok = true;
    data.alerts = (alerts_t){
        .count = 1, .started = false, .severity = ALERT_MODERATE, .event = "HEAVY RAIN", .onset = "19:00"};
    data.today_ymd = 20260926;
    data.holidays = (holidays_t){.year = 2026, .count = 1, .day = {{20261003, "GERMAN UNITY DAY"}}};
    data.bvg_ok = true;
    data.bvg_walk_min = 6;
    data.bvg_walk_comfort = 11;
    data.bvg = (bvg_departures_t){.count = 3,
                                  .dep = {{"U5", "HAUPTBAHNHOF", 17, 35},
                                          {"U5", "HAUPTBAHNHOF", 17, 45},
                                          {"U5", "HAUPTBAHNHOF", 17, 55}}};
    cJSON *actual = status();

    char *text = fixture_read("../../docs/api/status.example.json");
    cJSON *expected = cJSON_Parse(text);
    free(text);
    TEST_ASSERT_NOT_NULL(expected);
    bool same = cJSON_Compare(expected, actual, true);
    cJSON_Delete(expected);
    TEST_ASSERT_TRUE_MESSAGE(same, buf); // prints what the firmware produced
}

static void test_too_small_buffer_fails(void)
{
    char small[32];
    TEST_ASSERT_EQUAL_INT(-1, web_status_json(&data, &view, small, sizeof(small)));
}

static void test_panel_command(void)
{
    input_event_t ev;
    TEST_ASSERT_TRUE(web_panel_command("on", &ev));
    TEST_ASSERT_EQUAL_INT(INPUT_PANEL, ev.type);
    TEST_ASSERT_EQUAL_INT(1, ev.arg);
    TEST_ASSERT_TRUE(web_panel_command("off", &ev));
    TEST_ASSERT_EQUAL_INT(INPUT_PANEL, ev.type);
    TEST_ASSERT_EQUAL_INT(0, ev.arg);
    TEST_ASSERT_TRUE(web_panel_command("toggle", &ev));
    TEST_ASSERT_EQUAL_INT(INPUT_PRESS, ev.type);
    TEST_ASSERT_FALSE(web_panel_command("ON", &ev));
    TEST_ASSERT_FALSE(web_panel_command("", &ev));
}

static void test_screen_command(void)
{
    input_event_t ev;
    TEST_ASSERT_TRUE(web_screen_command("next", &ev));
    TEST_ASSERT_EQUAL_INT(INPUT_CW, ev.type);
    TEST_ASSERT_TRUE(web_screen_command("prev", &ev));
    TEST_ASSERT_EQUAL_INT(INPUT_CCW, ev.type);
    TEST_ASSERT_TRUE(web_screen_command("air", &ev));
    TEST_ASSERT_EQUAL_INT(INPUT_SCREEN, ev.type);
    TEST_ASSERT_EQUAL_INT(SCREEN_AIR, ev.arg);
    TEST_ASSERT_FALSE(web_screen_command("weather", &ev));
    TEST_ASSERT_FALSE(web_screen_command("", &ev));
}

static void test_screen_names_round_trip(void)
{
    for (int s = 0; s < SCREEN_COUNT; s++) {
        TEST_ASSERT_EQUAL_INT(s, screen_from_name(screen_name((screen_t)s)));
    }
    TEST_ASSERT_NULL(screen_name(SCREEN_COUNT));
    TEST_ASSERT_EQUAL_INT(-1, screen_from_name("HOME"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sections_null_before_any_data);
    RUN_TEST(test_screen_and_panel);
    RUN_TEST(test_outdoor_indoor_air);
    RUN_TEST(test_warning);
    RUN_TEST(test_next_holiday);
    RUN_TEST(test_bvg_skips_departed);
    RUN_TEST(test_worst_case_fits);
    RUN_TEST(test_matches_documented_example);
    RUN_TEST(test_too_small_buffer_fails);
    RUN_TEST(test_panel_command);
    RUN_TEST(test_screen_command);
    RUN_TEST(test_screen_names_round_trip);
    return UNITY_END();
}
