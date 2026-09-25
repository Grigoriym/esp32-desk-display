#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "clock_time.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static struct tm utc(int year, int month, int day, int hour, int min, int sec)
{
    struct tm t = {
        .tm_year = year - 1900,
        .tm_mon = month - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min = min,
        .tm_sec = sec,
    };
    return t;
}

// --- utc_to_epoch ---

static void test_epoch_known_values(void)
{
    struct tm t = utc(1970, 1, 1, 0, 0, 0);
    TEST_ASSERT_EQUAL_INT64(0, utc_to_epoch(&t));
    t = utc(2000, 1, 1, 0, 0, 0); // DS3231 year 00
    TEST_ASSERT_EQUAL_INT64(946684800, utc_to_epoch(&t));
    t = utc(2000, 2, 29, 12, 0, 0); // leap day, century divisible by 400
    TEST_ASSERT_EQUAL_INT64(951825600, utc_to_epoch(&t));
    t = utc(2099, 12, 31, 23, 59, 59); // DS3231 year 99, its last second
    TEST_ASSERT_EQUAL_INT64(4102444799, utc_to_epoch(&t));
}

// Every day 2000-2100 at 23:59:59, checked against glibc's timegm().
static void test_epoch_matches_timegm_2000_2100(void)
{
    struct tm t = utc(2000, 1, 1, 23, 59, 59);
    for (int day = 0; day < 36890; day++) { // > 101 years
        struct tm expected_tm = t;
        time_t expected = timegm(&expected_tm); // also normalizes t (day overflow)
        TEST_ASSERT_EQUAL_INT64(expected, utc_to_epoch(&expected_tm));
        t = expected_tm;
        t.tm_mday++;
    }
    TEST_ASSERT_EQUAL_INT(2100, t.tm_year + 1900);
}

// --- BCD ---

static void test_bcd_round_trip(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x59, bin_to_bcd(59));
    TEST_ASSERT_EQUAL_UINT8(59, bcd_to_bin(0x59));
    for (uint8_t v = 0; v < 100; v++) {
        TEST_ASSERT_EQUAL_UINT8(v, bcd_to_bin(bin_to_bcd(v)));
    }
}

// --- LOCAL_TZ: the Berlin DST switch moments ---

static void use_local_tz(void)
{
    setenv("TZ", LOCAL_TZ, 1);
    tzset();
}

// Local wall-clock hour and DST flag at the given UTC moment.
static void assert_local(const struct tm *utc_tm, int hour, int min, int isdst)
{
    time_t epoch = utc_to_epoch(utc_tm);
    struct tm local;
    localtime_r(&epoch, &local);
    TEST_ASSERT_EQUAL_INT(hour, local.tm_hour);
    TEST_ASSERT_EQUAL_INT(min, local.tm_min);
    TEST_ASSERT_EQUAL_INT(isdst, local.tm_isdst);
}

static void test_dst_ends_last_sunday_of_october(void)
{
    use_local_tz();
    // 2026-10-25 03:00 CEST = 01:00 UTC: clocks go back to 02:00 CET.
    struct tm t = utc(2026, 10, 25, 0, 59, 59);
    assert_local(&t, 2, 59, 1);
    t = utc(2026, 10, 25, 1, 0, 0);
    assert_local(&t, 2, 0, 0);
}

static void test_dst_starts_last_sunday_of_march(void)
{
    use_local_tz();
    // 2027-03-28 02:00 CET = 01:00 UTC: clocks jump to 03:00 CEST.
    struct tm t = utc(2027, 3, 28, 0, 59, 59);
    assert_local(&t, 1, 59, 0);
    t = utc(2027, 3, 28, 1, 0, 0);
    assert_local(&t, 3, 0, 1);
}

static void test_winter_and_summer_offsets(void)
{
    use_local_tz();
    struct tm t = utc(2026, 1, 15, 12, 0, 0);
    assert_local(&t, 13, 0, 0); // CET = UTC+1
    t = utc(2026, 7, 15, 12, 0, 0);
    assert_local(&t, 14, 0, 1); // CEST = UTC+2
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_epoch_known_values);
    RUN_TEST(test_epoch_matches_timegm_2000_2100);
    RUN_TEST(test_bcd_round_trip);
    RUN_TEST(test_dst_ends_last_sunday_of_october);
    RUN_TEST(test_dst_starts_last_sunday_of_march);
    RUN_TEST(test_winter_and_summer_offsets);
    return UNITY_END();
}
