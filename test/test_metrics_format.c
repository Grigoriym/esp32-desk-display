#include <string.h>
#include "unity.h"
#include "metrics_format.h"

static screen_data_t data;
static metrics_device_t dev;
static char buf[512];

void setUp(void)
{
    memset(&data, 0, sizeof(data));
    dev = (metrics_device_t){.uptime_s = 3600, .heap_free_kb = 150};
}

void tearDown(void)
{
}

static void test_device_only_before_any_data(void)
{
    int n = metrics_format(&data, &dev, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("device,device=desk uptime_s=3600,heap_free_kb=150\n", buf);
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
}

static void test_all_measurements(void)
{
    data.indoor_ok = true;
    data.indoor = (bme280_reading_t){.temp_c = 23.456f, .humidity_pct = 45.25f, .pressure_hpa = 1013.2f};
    data.weather_ok = true;
    data.weather = (weather_t){.temp_c = -4, .wind_kmh = 9, .uv_max = 3, .weather_code = 61};
    data.air_ok = true;
    data.air = (air_t){.aqi = 23, .pollen = {[POLLEN_BIRCH] = 120, [POLLEN_RAGWEED] = 1}};
    dev.rssi_ok = true;
    dev.rssi = -61;
    metrics_format(&data, &dev, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("indoor,device=desk temp_c=23.46,humidity=45.2,pressure_hpa=1013.20\n"
                             "outdoor,device=desk temp_c=-4,wind_kmh=9,uv=3,weather_code=61\n"
                             "air,device=desk aqi=23,alder=0,birch=120,grass=0,mugwort=0,ragweed=1\n"
                             "device,device=desk uptime_s=3600,heap_free_kb=150,rssi=-61\n",
                             buf);
}

static void test_too_small_buffer_fails(void)
{
    data.indoor_ok = true;
    char small[40];
    TEST_ASSERT_EQUAL_INT(-1, metrics_format(&data, &dev, small, sizeof(small)));
    TEST_ASSERT_EQUAL_INT(-1, metrics_format(&data, &dev, small, 0));
}

// The firmware's 512-byte batch must hold the longest possible values.
static void test_worst_case_fits_512(void)
{
    data.indoor_ok = data.weather_ok = data.air_ok = true;
    data.indoor = (bme280_reading_t){.temp_c = -40.0f, .humidity_pct = 100.0f, .pressure_hpa = 1100.0f};
    data.weather = (weather_t){
        .temp_c = -2147483647, .wind_kmh = -2147483647, .uv_max = -2147483647, .weather_code = -2147483647};
    data.air.aqi = -2147483647;
    for (int i = 0; i < POLLEN_COUNT; i++) data.air.pollen[i] = -2147483647;
    dev = (metrics_device_t){
        .uptime_s = -2147483647, .heap_free_kb = -2147483647, .rssi_ok = true, .rssi = -128};
    TEST_ASSERT_GREATER_THAN_INT(0, metrics_format(&data, &dev, buf, sizeof(buf)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_device_only_before_any_data);
    RUN_TEST(test_all_measurements);
    RUN_TEST(test_too_small_buffer_fails);
    RUN_TEST(test_worst_case_fits_512);
    return UNITY_END();
}
