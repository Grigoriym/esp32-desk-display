#include <string.h>
#include "unity.h"
#include "alerts_parse.h"
#include "fixtures.h"

// alerts_warnings.json (hand-written, see fixtures/README.md), all on
// 2026-09-25: wind gusts minor from 13:40, heavy rain moderate from 14:00,
// heavy thunderstorms severe from 18:00, plus an extreme "test" message.

static char *json;

void setUp(void)
{
    json = fixture_read("alerts_warnings.json");
}

void tearDown(void)
{
    free(json);
}

static void test_real_empty_response(void)
{
    char *none = fixture_read("alerts_none.json");
    alerts_t a = {.count = 9};
    TEST_ASSERT_TRUE(alerts_parse(none, "2026-09-25T14:30", &a));
    TEST_ASSERT_EQUAL_INT(0, a.count);
    free(none);
}

static void test_started_beats_upcoming_then_severity(void)
{
    alerts_t a;
    TEST_ASSERT_TRUE(alerts_parse(json, "2026-09-25T14:30", &a));
    TEST_ASSERT_EQUAL_INT(3, a.count); // the test message isn't counted
    TEST_ASSERT_TRUE(a.started);
    TEST_ASSERT_EQUAL_INT(ALERT_MODERATE, a.severity);
    TEST_ASSERT_EQUAL_STRING("HEAVY RAIN", a.event);
    TEST_ASSERT_EQUAL_STRING("14:00", a.onset);
}

static void test_onset_minute_counts_as_started(void)
{
    alerts_t a;
    TEST_ASSERT_TRUE(alerts_parse(json, "2026-09-25T13:40", &a));
    TEST_ASSERT_TRUE(a.started);
    TEST_ASSERT_EQUAL_STRING("WIND GUSTS", a.event);
}

static void test_only_upcoming_picks_most_severe(void)
{
    alerts_t a;
    TEST_ASSERT_TRUE(alerts_parse(json, "2026-09-25T12:00", &a));
    TEST_ASSERT_FALSE(a.started);
    TEST_ASSERT_EQUAL_INT(ALERT_SEVERE, a.severity);
    TEST_ASSERT_EQUAL_STRING("HEAVY THUNDERSTORMS", a.event);
    TEST_ASSERT_EQUAL_STRING("18:00", a.onset);
}

static void test_onset_on_another_day_is_a_date(void)
{
    alerts_t a;
    TEST_ASSERT_TRUE(alerts_parse(json, "2026-09-24T23:00", &a));
    TEST_ASSERT_FALSE(a.started);
    TEST_ASSERT_EQUAL_STRING("25/09", a.onset);
}

static void test_skips_allclear_and_test_messages(void)
{
    const char *only_skipped = "{\"alerts\":["
                               "{\"status\":\"actual\",\"response_type\":\"allclear\",\"severity\":\"minor\","
                               "\"onset\":\"2026-09-25T10:00:00+02:00\",\"event_en\":\"frost\"},"
                               "{\"status\":\"test\",\"response_type\":\"prepare\",\"severity\":\"minor\","
                               "\"onset\":\"2026-09-25T10:00:00+02:00\",\"event_en\":\"frost\"}]}";
    alerts_t a;
    TEST_ASSERT_TRUE(alerts_parse(only_skipped, "2026-09-25T12:00", &a));
    TEST_ASSERT_EQUAL_INT(0, a.count);
}

static void test_long_label_is_clipped_and_missing_fields_are_tolerated(void)
{
    const char *odd = "{\"alerts\":[{\"status\":\"actual\",\"event_en\":"
                      "\"extremely heavy thunderstorms with heavy rain and hail\"}]}";
    alerts_t a;
    TEST_ASSERT_TRUE(alerts_parse(odd, "2026-09-25T12:00", &a));
    TEST_ASSERT_EQUAL_INT(1, a.count);
    TEST_ASSERT_TRUE(a.started); // no onset: treated as already started
    TEST_ASSERT_EQUAL_INT(ALERT_MINOR, a.severity);
    TEST_ASSERT_EQUAL_STRING("EXTREMELY HEAVY THUNDER", a.event);
    TEST_ASSERT_EQUAL_STRING("", a.onset);
}

static void test_invalid_fails_and_leaves_out_untouched(void)
{
    alerts_t a = {.count = 7};
    TEST_ASSERT_FALSE(alerts_parse("{\"location\":{}}", "2026-09-25T12:00", &a));
    TEST_ASSERT_FALSE(alerts_parse("{\"alerts\":[{\"status\":", "2026-09-25T12:00", &a)); // truncated
    TEST_ASSERT_FALSE(alerts_parse("", "2026-09-25T12:00", &a));
    TEST_ASSERT_EQUAL_INT(7, a.count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_real_empty_response);
    RUN_TEST(test_started_beats_upcoming_then_severity);
    RUN_TEST(test_onset_minute_counts_as_started);
    RUN_TEST(test_only_upcoming_picks_most_severe);
    RUN_TEST(test_onset_on_another_day_is_a_date);
    RUN_TEST(test_skips_allclear_and_test_messages);
    RUN_TEST(test_long_label_is_clipped_and_missing_fields_are_tolerated);
    RUN_TEST(test_invalid_fails_and_leaves_out_untouched);
    return UNITY_END();
}
