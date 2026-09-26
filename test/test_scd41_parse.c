#include "unity.h"
#include "scd41_parse.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_crc_datasheet_example(void)
{
    // Sensirion's own example: CRC of 0xBEEF is 0x92.
    const uint8_t word[] = {0xBE, 0xEF};
    TEST_ASSERT_EQUAL_HEX8(0x92, scd41_crc8(word));
}

static void test_measurement_datasheet_example(void)
{
    // SCD4x datasheet read_measurement example: 500 ppm, 25 C, 37 %RH.
    const uint8_t buf[] = {0x01, 0xF4, 0x33, 0x66, 0x67, 0xA2, 0x5E, 0xB9, 0x3C};
    scd41_reading_t r;
    TEST_ASSERT_TRUE(scd41_parse_measurement(buf, &r));
    TEST_ASSERT_EQUAL_INT(500, r.co2_ppm);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, r.temp_c);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 37.0f, r.humidity_pct);
}

static void test_measurement_bad_crc_fails(void)
{
    // Last word's CRC off by one: the whole reading is rejected.
    const uint8_t buf[] = {0x01, 0xF4, 0x33, 0x66, 0x67, 0xA2, 0x5E, 0xB9, 0x3D};
    scd41_reading_t r = {.co2_ppm = -1};
    TEST_ASSERT_FALSE(scd41_parse_measurement(buf, &r));
}

static void test_data_ready(void)
{
    uint8_t buf[3] = {0x80, 0x00, 0};
    bool ready = true;
    buf[2] = scd41_crc8(buf);
    TEST_ASSERT_TRUE(scd41_parse_data_ready(buf, &ready));
    TEST_ASSERT_FALSE(ready); // only bits above the lower 11 set

    buf[0] = 0x80;
    buf[1] = 0x06;
    buf[2] = scd41_crc8(buf);
    TEST_ASSERT_TRUE(scd41_parse_data_ready(buf, &ready));
    TEST_ASSERT_TRUE(ready);

    buf[2] ^= 1;
    TEST_ASSERT_FALSE(scd41_parse_data_ready(buf, &ready));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc_datasheet_example);
    RUN_TEST(test_measurement_datasheet_example);
    RUN_TEST(test_measurement_bad_crc_fails);
    RUN_TEST(test_data_ready);
    return UNITY_END();
}
