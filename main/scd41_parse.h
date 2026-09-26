#pragma once

// SCD41 (Sensirion CO2 sensor) response decoding: pure, no ESP-IDF, so the
// host tests can check it. The I2C side is in scd41.c.

#include <stdbool.h>
#include <stdint.h>

// Every 16-bit word the sensor sends is 2 bytes MSB first plus this CRC-8
// (polynomial 0x31, init 0xFF) over those 2 bytes.
uint8_t scd41_crc8(const uint8_t *word);

typedef struct {
    int co2_ppm;
    float temp_c;
    float humidity_pct;
} scd41_reading_t;

// buf = the 3 bytes of the get_data_ready_status answer. False on a bad CRC.
bool scd41_parse_data_ready(const uint8_t *buf, bool *ready);

// buf = the 9 bytes of the read_measurement answer (CO2, temp, humidity).
// False on a bad CRC.
bool scd41_parse_measurement(const uint8_t *buf, scd41_reading_t *out);
