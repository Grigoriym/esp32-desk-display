#include <stdio.h>
#include "metrics_format.h"

#define TAGS ",device=desk "

// Appends to buf at used; on overflow clears ok and writes nothing more.
#define PUT(...)                                                \
    do {                                                        \
        if (!ok) break;                                         \
        int n_ = snprintf(buf + used, len - used, __VA_ARGS__); \
        if (n_ < 0 || (size_t)n_ >= len - used) ok = false;     \
        else used += (size_t)n_;                                \
    } while (0)

int metrics_format(const screen_data_t *d, const metrics_device_t *dev, char *buf, size_t len)
{
    if (len == 0) return -1;
    size_t used = 0;
    bool ok = true;
    buf[0] = '\0';
    if (d->indoor_ok) {
        PUT("indoor" TAGS "temp_c=%.2f,humidity=%.1f,pressure_hpa=%.2f\n", (double)d->indoor.temp_c,
            (double)d->indoor.humidity_pct, (double)d->indoor.pressure_hpa);
    }
    if (d->weather_ok) {
        PUT("outdoor" TAGS "temp_c=%d,wind_kmh=%d,uv=%d,weather_code=%d\n", d->weather.temp_c,
            d->weather.wind_kmh, d->weather.uv_max, d->weather.weather_code);
    }
    if (d->air_ok) {
        const int *p = d->air.pollen;
        PUT("air" TAGS "aqi=%d,alder=%d,birch=%d,grass=%d,mugwort=%d,ragweed=%d\n", d->air.aqi,
            p[POLLEN_ALDER], p[POLLEN_BIRCH], p[POLLEN_GRASS], p[POLLEN_MUGWORT], p[POLLEN_RAGWEED]);
    }
    PUT("device" TAGS "uptime_s=%ld,heap_free_kb=%d", dev->uptime_s, dev->heap_free_kb);
    if (dev->rssi_ok) PUT(",rssi=%d", dev->rssi);
    PUT("\n");
    return ok ? (int)used : -1;
}
