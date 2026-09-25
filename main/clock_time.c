#include "clock_time.h"

// Days-from-civil, proleptic Gregorian.
time_t utc_to_epoch(const struct tm *t)
{
    int y = t->tm_year + 1900;
    int m = t->tm_mon + 1;
    y -= m <= 2;
    int era = (y >= 0 ? y : y - 399) / 400;
    int yoe = y - era * 400;
    int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + t->tm_mday - 1;
    int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    long days = (long)era * 146097 + doe - 719468;
    return (time_t)days * 86400 + (time_t)t->tm_hour * 3600 + (time_t)t->tm_min * 60 + t->tm_sec;
}

uint8_t bcd_to_bin(uint8_t v)
{
    return (v >> 4) * 10 + (v & 0x0F);
}

uint8_t bin_to_bcd(uint8_t v)
{
    return ((v / 10) << 4) | (v % 10);
}
