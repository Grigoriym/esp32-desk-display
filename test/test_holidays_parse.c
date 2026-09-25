#include <string.h>
#include "unity.h"
#include "holidays_parse.h"
#include "fixtures.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static holidays_t berlin_2026(void)
{
    char *json = fixture_read("holidays_2026.json");
    holidays_t h;
    TEST_ASSERT_TRUE(holidays_parse(json, 2026, "DE-BE", &h));
    free(json);
    return h;
}

static void test_berlin_2026_from_real_response(void)
{
    holidays_t h = berlin_2026();
    TEST_ASSERT_EQUAL_INT(2026, h.year);
    // 9 nationwide + Frauentag; not e.g. Epiphany (BW/BY/ST only).
    TEST_ASSERT_EQUAL_INT(10, h.count);
    TEST_ASSERT_EQUAL_INT(20260101, h.day[0].ymd);
    TEST_ASSERT_EQUAL_STRING("NEW YEARS DAY", h.day[0].name);
    TEST_ASSERT_EQUAL_INT(20260308, h.day[1].ymd);
    TEST_ASSERT_EQUAL_STRING("INTERNATIONAL WOMENS", h.day[1].name); // clipped at 21, trailing space gone
    TEST_ASSERT_EQUAL_INT(20261003, h.day[7].ymd);
    TEST_ASSERT_EQUAL_STRING("GERMAN UNITY DAY", h.day[7].name);
    TEST_ASSERT_EQUAL_INT(20261226, h.day[9].ymd);
    TEST_ASSERT_EQUAL_STRING("ST STEPHENS DAY", h.day[9].name);
}

static void test_other_region(void)
{
    char *json = fixture_read("holidays_2026.json");
    holidays_t h;
    TEST_ASSERT_TRUE(holidays_parse(json, 2026, "DE-BY", &h));
    free(json);
    TEST_ASSERT_EQUAL_STRING("EPIPHANY", h.day[1].name); // Bavaria has it, not Frauentag
}

static void test_next(void)
{
    holidays_t h = berlin_2026();
    TEST_ASSERT_EQUAL_INT(0, holidays_next(&h, 20260101)); // on the day itself
    TEST_ASSERT_EQUAL_INT(7, holidays_next(&h, 20260925)); // -> 3 Oct
    TEST_ASSERT_EQUAL_INT(7, holidays_next(&h, 20261003));
    TEST_ASSERT_EQUAL_INT(8, holidays_next(&h, 20261004));  // -> 25 Dec
    TEST_ASSERT_EQUAL_INT(-1, holidays_next(&h, 20261227)); // year done
}

static void test_wanted_year(void)
{
    holidays_t none = {0};
    TEST_ASSERT_EQUAL_INT(2026, holidays_wanted_year(&none, 20260925));

    holidays_t h = berlin_2026();
    TEST_ASSERT_EQUAL_INT(0, holidays_wanted_year(&h, 20260925));
    TEST_ASSERT_EQUAL_INT(0, holidays_wanted_year(&h, 20261226));
    TEST_ASSERT_EQUAL_INT(2027, holidays_wanted_year(&h, 20261227)); // this year's all past
    TEST_ASSERT_EQUAL_INT(2027, holidays_wanted_year(&h, 20270101)); // loaded list is last year's

    h.year = 2027; // next year's list, fetched in late December
    TEST_ASSERT_EQUAL_INT(0, holidays_wanted_year(&h, 20261228));
}

static void test_caps_at_max(void)
{
    char json[4096] = "[";
    for (int i = 0; i < HOLIDAYS_MAX + 5; i++) {
        char item[128];
        snprintf(item, sizeof(item), "%s{\"date\":\"2026-01-%02d\",\"name\":\"Day %d\",\"global\":true}",
                 i ? "," : "", i + 1, i);
        strcat(json, item);
    }
    strcat(json, "]");
    holidays_t h;
    TEST_ASSERT_TRUE(holidays_parse(json, 2026, "DE-BE", &h));
    TEST_ASSERT_EQUAL_INT(HOLIDAYS_MAX, h.count);
}

static void test_invalid_fails_and_leaves_out_untouched(void)
{
    holidays_t h = {.year = 1999};
    TEST_ASSERT_FALSE(holidays_parse("{\"status\":404}", 2026, "DE-BE", &h));
    TEST_ASSERT_FALSE(holidays_parse("[{\"date\":", 2026, "DE-BE", &h)); // truncated
    TEST_ASSERT_FALSE(holidays_parse("", 2026, "DE-BE", &h));
    TEST_ASSERT_EQUAL_INT(1999, h.year);

    // Entries with missing fields are skipped, not fatal.
    TEST_ASSERT_TRUE(holidays_parse("[{\"global\":true},{\"date\":\"x\",\"name\":\"A\",\"global\":true}]",
                                    2026, "DE-BE", &h));
    TEST_ASSERT_EQUAL_INT(0, h.count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_berlin_2026_from_real_response);
    RUN_TEST(test_other_region);
    RUN_TEST(test_next);
    RUN_TEST(test_wanted_year);
    RUN_TEST(test_caps_at_max);
    RUN_TEST(test_invalid_fails_and_leaves_out_untouched);
    return UNITY_END();
}
