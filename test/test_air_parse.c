#include <string.h>
#include "unity.h"
#include "air_parse.h"
#include "fixtures.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_parses_real_response(void)
{
    char *json = fixture_read("air_ok.json");
    air_t a;
    TEST_ASSERT_TRUE(air_parse(json, &a));
    TEST_ASSERT_EQUAL_INT(23, a.aqi);
    TEST_ASSERT_EQUAL_INT(0, a.pollen[POLLEN_BIRCH]);
    TEST_ASSERT_EQUAL_INT(0, a.pollen[POLLEN_RAGWEED]); // 0.1 rounded
    free(json);
}

static void test_pollen_values_and_null_or_missing_as_zero(void)
{
    const char *json = "{\"current\":{\"european_aqi\":41.6,\"alder_pollen\":null,\"birch_pollen\":87.5,"
                       "\"grass_pollen\":12.2,\"mugwort_pollen\":0.4}}";
    air_t a;
    TEST_ASSERT_TRUE(air_parse(json, &a));
    TEST_ASSERT_EQUAL_INT(42, a.aqi);
    TEST_ASSERT_EQUAL_INT(0, a.pollen[POLLEN_ALDER]); // null (off season)
    TEST_ASSERT_EQUAL_INT(88, a.pollen[POLLEN_BIRCH]);
    TEST_ASSERT_EQUAL_INT(12, a.pollen[POLLEN_GRASS]);
    TEST_ASSERT_EQUAL_INT(0, a.pollen[POLLEN_MUGWORT]);
    TEST_ASSERT_EQUAL_INT(0, a.pollen[POLLEN_RAGWEED]); // missing
}

static void test_missing_aqi_fails_and_leaves_out_untouched(void)
{
    air_t a = {.aqi = 99};
    TEST_ASSERT_FALSE(air_parse("{\"current\":{\"birch_pollen\":3}}", &a));
    TEST_ASSERT_FALSE(air_parse("{\"current\":{\"european_aqi\":null}}", &a));
    TEST_ASSERT_FALSE(air_parse("{\"error\":true,\"reason\":\"x\"}", &a));
    TEST_ASSERT_FALSE(air_parse("{\"current\":", &a)); // truncated body
    TEST_ASSERT_FALSE(air_parse("", &a));
    TEST_ASSERT_EQUAL_INT(99, a.aqi);
}

static void test_aqi_bands(void)
{
    TEST_ASSERT_EQUAL_STRING("GOOD", air_aqi_label(0));
    TEST_ASSERT_EQUAL_STRING("GOOD", air_aqi_label(20));
    TEST_ASSERT_EQUAL_STRING("FAIR", air_aqi_label(21));
    TEST_ASSERT_EQUAL_STRING("FAIR", air_aqi_label(40));
    TEST_ASSERT_EQUAL_STRING("MODERATE", air_aqi_label(60));
    TEST_ASSERT_EQUAL_STRING("POOR", air_aqi_label(80));
    TEST_ASSERT_EQUAL_STRING("VERY POOR", air_aqi_label(100));
    TEST_ASSERT_EQUAL_STRING("EXTREME", air_aqi_label(101));
}

static void test_pollen_levels_and_names(void)
{
    TEST_ASSERT_NULL(air_pollen_level(0));
    TEST_ASSERT_EQUAL_STRING("LOW", air_pollen_level(1));
    TEST_ASSERT_EQUAL_STRING("LOW", air_pollen_level(10));
    TEST_ASSERT_EQUAL_STRING("MED", air_pollen_level(11));
    TEST_ASSERT_EQUAL_STRING("MED", air_pollen_level(50));
    TEST_ASSERT_EQUAL_STRING("HIGH", air_pollen_level(51));
    TEST_ASSERT_EQUAL_STRING("MUGWORT", air_pollen_name(POLLEN_MUGWORT));
    TEST_ASSERT_EQUAL_STRING("", air_pollen_name(POLLEN_COUNT));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parses_real_response);
    RUN_TEST(test_pollen_values_and_null_or_missing_as_zero);
    RUN_TEST(test_missing_aqi_fails_and_leaves_out_untouched);
    RUN_TEST(test_aqi_bands);
    RUN_TEST(test_pollen_levels_and_names);
    return UNITY_END();
}
