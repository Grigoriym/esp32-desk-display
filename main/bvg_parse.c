#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "bvg_parse.h"
#include "cJSON.h"

void bvg_display_name(const char *src, char *dst, size_t dst_size)
{
    size_t n = 0;
    for (const unsigned char *p = (const unsigned char *)src; *p && n + 2 < dst_size; p++) {
        if (*p == '(') break;
        const char *sub = NULL;
        char one[2] = {0};
        if (*p == 0xC3 && p[1]) { // UTF-8 umlauts / sharp s
            switch (p[1]) {
                case 0xA4:
                case 0x84: sub = "AE"; break;
                case 0xB6:
                case 0x96: sub = "OE"; break;
                case 0xBC:
                case 0x9C: sub = "UE"; break;
                case 0x9F: sub = "SS"; break;
                default: break; // other 2-byte chars: dropped
            }
            p++;
        } else if (isalnum(*p) || *p == ' ' || *p == '-' || *p == '/') {
            one[0] = (char)toupper(*p);
            sub = one;
        }
        for (; sub && *sub && n + 1 < dst_size; sub++) dst[n++] = *sub;
    }
    while (n > 0 && dst[n - 1] == ' ') n--; // left over from " (Berlin)"
    dst[n] = '\0';
}

// Returns the end of the JSON object starting at '{' (just past its '}'), or
// NULL if the text ends first (truncated response). Skips strings, so braces
// inside note texts don't count.
static const char *object_end(const char *p)
{
    int depth = 0;
    bool in_string = false;
    for (; *p; p++) {
        if (in_string) {
            if (*p == '\\' && p[1]) p++;
            else if (*p == '"') in_string = false;
        } else if (*p == '"') {
            in_string = true;
        } else if (*p == '{') {
            depth++;
        } else if (*p == '}' && --depth == 0) {
            return p + 1;
        }
    }
    return NULL;
}

// Fills *dep from one parsed "Departure" object; false for a cancelled or
// malformed one.
static bool parse_departure(const cJSON *d, bvg_departure_t *dep)
{
    if (cJSON_IsTrue(cJSON_GetObjectItem(d, "cancelled"))) return false;
    cJSON *line = cJSON_GetObjectItem(d, "name");
    cJSON *dir = cJSON_GetObjectItem(d, "direction");
    // "rtTime" is the real time incl. delay; missing without live data.
    cJSON *time = cJSON_GetObjectItem(d, "rtTime");
    if (!cJSON_IsString(time)) time = cJSON_GetObjectItem(d, "time");
    if (!cJSON_IsString(line) || !cJSON_IsString(dir) || !cJSON_IsString(time)) return false;

    // "22:41:00", already local time.
    // NOLINTNEXTLINE(cert-err34-c): match count is checked, fields are 2 digits
    if (sscanf(time->valuestring, "%d:%d", &dep->hour, &dep->minute) != 2) return false;
    bvg_display_name(line->valuestring, dep->line, sizeof(dep->line));
    bvg_display_name(dir->valuestring, dep->direction, sizeof(dep->direction));
    return true;
}

bool bvg_parse(const char *json, bvg_departures_t *out)
{
    // Error answers carry "errorCode"; a board with no trains has no
    // "Departure" key at all, only the server info.
    if (strstr(json, "\"errorCode\"")) return false;
    const char *p = strstr(json, "\"Departure\"");
    if (!p) {
        if (!strstr(json, "\"serverVersion\"")) return false;
        *out = (bvg_departures_t){0};
        return true;
    }
    p = strchr(p, '[');
    if (!p) return false;

    // One departure object at a time: the whole response (~4 KB of JSON per
    // train, mostly notes and product info) would need ~100 KB of heap as a
    // cJSON tree, a single departure ~15 KB.
    bvg_departures_t result = {0};
    for (p++;;) {
        p += strspn(p, " \t\r\n,");
        if (*p == ']') break;
        if (*p != '{') return false;
        const char *end = object_end(p);
        if (!end) return false; // truncated
        if (result.count < BVG_MAX_DEPARTURES) {
            cJSON *d = cJSON_ParseWithLength(p, (size_t)(end - p));
            if (!d) return false;
            if (parse_departure(d, &result.dep[result.count])) result.count++;
            cJSON_Delete(d);
        }
        p = end;
    }

    *out = result;
    return true;
}
