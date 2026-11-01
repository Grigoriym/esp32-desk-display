#include <string.h>
#include "unity.h"
#include "metrics_format.h"

#define NOW 1793500000L // late 2026

static screen_data_t data;
static metrics_device_t dev;
static metrics_record_t rec;
static char buf[512];

void setUp(void)
{
    memset(&data, 0, sizeof(data));
    dev = (metrics_device_t){.uptime_s = 3600, .heap_free_kb = 150};
}

void tearDown(void)
{
}

static void fill_all(void)
{
    data.indoor_ok = true;
    data.indoor = (bme280_reading_t){.temp_c = 23.456f, .humidity_pct = 45.26f, .pressure_hpa = 1013.2f};
    data.co2_ok = true;
    data.co2.co2_ppm = 812;
    data.weather_ok = true;
    data.weather = (weather_t){.temp_c = -4, .wind_kmh = 9, .uv_max = 3, .weather_code = 61};
    data.air_ok = true;
    data.air = (air_t){.aqi = 23, .pollen = {[POLLEN_BIRCH] = 120, [POLLEN_RAGWEED] = 1}};
    dev.rssi_ok = true;
    dev.rssi = -61;
}

static void test_device_only_before_any_data(void)
{
    metrics_record_make(&data, &dev, NOW, &rec);
    int n = metrics_format(&rec, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("device,device=desk uptime_s=3600,heap_free_kb=150 1793500000\n", buf);
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
}

static void test_all_measurements(void)
{
    fill_all();
    metrics_record_make(&data, &dev, NOW, &rec);
    metrics_format(&rec, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING(
        "indoor,device=desk temp_c=23.46,humidity=45.3,pressure_hpa=1013.20 1793500000\n"
        "co2,device=desk ppm=812 1793500000\n"
        "outdoor,device=desk temp_c=-4,wind_kmh=9,uv=3,weather_code=61 1793500000\n"
        "air,device=desk aqi=23,alder=0,birch=120,grass=0,mugwort=0,ragweed=1 1793500000\n"
        "device,device=desk uptime_s=3600,heap_free_kb=150,rssi=-61 1793500000\n",
        buf);
}

static void test_clock_not_set_means_no_timestamp(void)
{
    metrics_record_make(&data, &dev, 100, &rec);
    TEST_ASSERT_EQUAL_UINT32(0, rec.time_utc);
    metrics_format(&rec, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("device,device=desk uptime_s=3600,heap_free_kb=150\n", buf);
}

static void test_negative_indoor_temp_rounds_right(void)
{
    data.indoor_ok = true;
    data.indoor = (bme280_reading_t){.temp_c = -5.126f, .humidity_pct = 0.0f, .pressure_hpa = 0.0f};
    metrics_record_make(&data, &dev, 0, &rec);
    metrics_format(&rec, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "temp_c=-5.13,"));
}

// A record keeps small types: out-of-range values pin to the edge, not wrap.
static void test_out_of_range_values_clamp(void)
{
    data.co2_ok = data.weather_ok = data.air_ok = true;
    data.co2.co2_ppm = 70000;
    data.weather = (weather_t){.temp_c = -200, .wind_kmh = 300, .uv_max = -1, .weather_code = 99};
    data.air.aqi = -5;
    data.air.pollen[POLLEN_GRASS] = 100000;
    metrics_record_make(&data, &dev, 0, &rec);
    metrics_format(&rec, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "ppm=65535"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "temp_c=-128,wind_kmh=255,uv=0,weather_code=99"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "aqi=0,alder=0,birch=0,grass=65535"));
}

static void test_too_small_buffer_fails(void)
{
    fill_all();
    metrics_record_make(&data, &dev, NOW, &rec);
    char small[40];
    TEST_ASSERT_EQUAL_INT(-1, metrics_format(&rec, small, sizeof(small)));
    TEST_ASSERT_EQUAL_INT(-1, metrics_format(&rec, small, 0));
}

// METRICS_RECORD_TEXT_MAX must hold the longest possible values.
static void test_worst_case_fits_text_max(void)
{
    data.indoor_ok = data.co2_ok = data.weather_ok = data.air_ok = true;
    data.co2.co2_ppm = 65535;
    data.indoor = (bme280_reading_t){.temp_c = -327.0f, .humidity_pct = 6553.0f, .pressure_hpa = 6553.0f};
    data.weather = (weather_t){.temp_c = -128, .wind_kmh = 255, .uv_max = 255, .weather_code = 255};
    data.air.aqi = 65535;
    for (int i = 0; i < POLLEN_COUNT; i++) data.air.pollen[i] = 65535;
    dev = (metrics_device_t){.uptime_s = 2147483647, .heap_free_kb = 65535, .rssi_ok = true, .rssi = -128};
    metrics_record_make(&data, &dev, 4294967295L, &rec);
    char text[METRICS_RECORD_TEXT_MAX];
    TEST_ASSERT_GREATER_THAN_INT(0, metrics_format(&rec, text, sizeof(text)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_device_only_before_any_data);
    RUN_TEST(test_all_measurements);
    RUN_TEST(test_clock_not_set_means_no_timestamp);
    RUN_TEST(test_negative_indoor_temp_rounds_right);
    RUN_TEST(test_out_of_range_values_clamp);
    RUN_TEST(test_too_small_buffer_fails);
    RUN_TEST(test_worst_case_fits_text_max);
    return UNITY_END();
}
