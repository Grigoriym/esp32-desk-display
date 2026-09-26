#include "scd41_parse.h"

uint8_t scd41_crc8(const uint8_t *word)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < 2; i++) {
        crc ^= word[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

// The n-th word of buf, or false if its CRC doesn't match.
static bool word_at(const uint8_t *buf, int n, uint16_t *out)
{
    const uint8_t *w = &buf[n * 3];
    if (scd41_crc8(w) != w[2]) return false;
    *out = (uint16_t)((w[0] << 8) | w[1]);
    return true;
}

bool scd41_parse_word(const uint8_t *buf, uint16_t *out)
{
    return word_at(buf, 0, out);
}

bool scd41_parse_data_ready(const uint8_t *buf, bool *ready)
{
    uint16_t status;
    if (!word_at(buf, 0, &status)) return false;
    // Datasheet: the lower 11 bits all 0 = no new measurement yet.
    *ready = (status & 0x07FF) != 0;
    return true;
}

bool scd41_parse_measurement(const uint8_t *buf, scd41_reading_t *out)
{
    uint16_t co2 = 0;
    uint16_t temp = 0;
    uint16_t rh = 0;
    if (!word_at(buf, 0, &co2) || !word_at(buf, 1, &temp) || !word_at(buf, 2, &rh)) return false;
    out->co2_ppm = co2;
    out->temp_c = -45.0f + 175.0f * (float)temp / 65535.0f;
    out->humidity_pct = 100.0f * (float)rh / 65535.0f;
    return true;
}
