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

static void test_word_datasheet_examples(void)
{
    // SCD4x datasheet get_automatic_self_calibration_* examples.
    const uint8_t disabled[] = {0x00, 0x00, 0x81};
    const uint8_t target_420[] = {0x01, 0xA4, 0x4D};
    const uint8_t initial_76h[] = {0x00, 0x4C, 0xC1};
    const uint8_t standard_156h[] = {0x00, 0x9C, 0xC5};
    uint16_t v = 0xFFFF;
    TEST_ASSERT_TRUE(scd41_parse_word(disabled, &v));
    TEST_ASSERT_EQUAL_UINT16(0, v);
    TEST_ASSERT_TRUE(scd41_parse_word(target_420, &v));
    TEST_ASSERT_EQUAL_UINT16(420, v);
    TEST_ASSERT_TRUE(scd41_parse_word(initial_76h, &v));
    TEST_ASSERT_EQUAL_UINT16(76, v);
    TEST_ASSERT_TRUE(scd41_parse_word(standard_156h, &v));
    TEST_ASSERT_EQUAL_UINT16(156, v);

    const uint8_t bad[] = {0x00, 0x9C, 0xC4};
    TEST_ASSERT_FALSE(scd41_parse_word(bad, &v));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc_datasheet_example);
    RUN_TEST(test_measurement_datasheet_example);
    RUN_TEST(test_measurement_bad_crc_fails);
    RUN_TEST(test_data_ready);
    RUN_TEST(test_word_datasheet_examples);
    return UNITY_END();
}
