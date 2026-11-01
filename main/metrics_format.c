#include <stdio.h>
#include "metrics_format.h"
#include "backlog_ring.h"

#define TAGS ",device=desk "

#define HAS_INDOOR  0x01u
#define HAS_CO2     0x02u
#define HAS_WEATHER 0x04u
#define HAS_AIR     0x08u
#define HAS_RSSI    0x10u

_Static_assert(sizeof(metrics_record_t) == BACKLOG_PAYLOAD_SIZE, "a record fills one backlog slot's payload");

static long clamp(long value, long low, long high)
{
    if (value < low) return low;
    return value > high ? high : value;
}

// value * scale, rounded to the nearest whole number.
static long scaled(float value, float scale)
{
    float v = value * scale;
    return (long)(v >= 0.0f ? v + 0.5f : v - 0.5f);
}

void metrics_record_make(const screen_data_t *d, const metrics_device_t *dev, long time_utc,
                         metrics_record_t *out)
{
    *out = (metrics_record_t){
        .time_utc = time_utc >= METRICS_MIN_VALID_TIME ? (uint32_t)time_utc : 0,
        .uptime_s = (uint32_t)clamp(dev->uptime_s, 0, INT32_MAX),
        .heap_free_kb = (uint16_t)clamp(dev->heap_free_kb, 0, UINT16_MAX),
    };
    if (d->indoor_ok) {
        out->flags |= HAS_INDOOR;
        out->temp_c100 = (int16_t)clamp(scaled(d->indoor.temp_c, 100.0f), INT16_MIN, INT16_MAX);
        out->humidity_x10 = (uint16_t)clamp(scaled(d->indoor.humidity_pct, 10.0f), 0, UINT16_MAX);
        out->pressure_x10 = (uint16_t)clamp(scaled(d->indoor.pressure_hpa, 10.0f), 0, UINT16_MAX);
    }
    if (d->co2_ok) {
        out->flags |= HAS_CO2;
        out->co2_ppm = (uint16_t)clamp(d->co2.co2_ppm, 0, UINT16_MAX);
    }
    if (d->weather_ok) {
        out->flags |= HAS_WEATHER;
        out->out_temp_c = (int8_t)clamp(d->weather.temp_c, INT8_MIN, INT8_MAX);
        out->wind_kmh = (uint8_t)clamp(d->weather.wind_kmh, 0, UINT8_MAX);
        out->uv = (uint8_t)clamp(d->weather.uv_max, 0, UINT8_MAX);
        out->weather_code = (uint8_t)clamp(d->weather.weather_code, 0, UINT8_MAX);
    }
    if (d->air_ok) {
        out->flags |= HAS_AIR;
        out->aqi = (uint16_t)clamp(d->air.aqi, 0, UINT16_MAX);
        for (int i = 0; i < POLLEN_COUNT; i++)
            out->pollen[i] = (uint16_t)clamp(d->air.pollen[i], 0, UINT16_MAX);
    }
    if (dev->rssi_ok) {
        out->flags |= HAS_RSSI;
        out->rssi = (int8_t)clamp(dev->rssi, INT8_MIN, INT8_MAX);
    }
}

// Appends to buf at used; on overflow clears ok and writes nothing more.
#define PUT(...)                                                \
    do {                                                        \
        if (!ok) break;                                         \
        int n_ = snprintf(buf + used, len - used, __VA_ARGS__); \
        if (n_ < 0 || (size_t)n_ >= len - used) ok = false;     \
        else used += (size_t)n_;                                \
    } while (0)

int metrics_format(const metrics_record_t *r, char *buf, size_t len)
{
    if (len == 0) return -1;
    size_t used = 0;
    bool ok = true;
    buf[0] = '\0';
    char stamp[16] = "";
    if (r->time_utc != 0) snprintf(stamp, sizeof(stamp), " %lu", (unsigned long)r->time_utc);
    if (r->flags & HAS_INDOOR) {
        PUT("indoor" TAGS "temp_c=%.2f,humidity=%.1f,pressure_hpa=%.2f%s\n", (double)r->temp_c100 / 100.0,
            (double)r->humidity_x10 / 10.0, (double)r->pressure_x10 / 10.0, stamp);
    }
    if (r->flags & HAS_CO2) PUT("co2" TAGS "ppm=%d%s\n", r->co2_ppm, stamp);
    if (r->flags & HAS_WEATHER) {
        PUT("outdoor" TAGS "temp_c=%d,wind_kmh=%d,uv=%d,weather_code=%d%s\n", r->out_temp_c, r->wind_kmh,
            r->uv, r->weather_code, stamp);
    }
    if (r->flags & HAS_AIR) {
        PUT("air" TAGS "aqi=%d,alder=%d,birch=%d,grass=%d,mugwort=%d,ragweed=%d%s\n", r->aqi,
            r->pollen[POLLEN_ALDER], r->pollen[POLLEN_BIRCH], r->pollen[POLLEN_GRASS],
            r->pollen[POLLEN_MUGWORT], r->pollen[POLLEN_RAGWEED], stamp);
    }
    PUT("device" TAGS "uptime_s=%lu,heap_free_kb=%d", (unsigned long)r->uptime_s, r->heap_free_kb);
    if (r->flags & HAS_RSSI) PUT(",rssi=%d", r->rssi);
    PUT("%s\n", stamp);
    return ok ? (int)used : -1;
}
