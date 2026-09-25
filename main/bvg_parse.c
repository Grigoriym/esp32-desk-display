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

bool bvg_parse(const char *json, bvg_departures_t *out)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;
    cJSON *deps = cJSON_GetObjectItem(root, "departures");
    if (!cJSON_IsArray(deps)) {
        cJSON_Delete(root);
        return false;
    }

    bvg_departures_t result = {0};
    cJSON *d;
    cJSON_ArrayForEach(d, deps)
    {
        if (result.count >= BVG_MAX_DEPARTURES) break;
        // "when" is the real time incl. delay, null when cancelled.
        cJSON *when = cJSON_GetObjectItem(d, "when");
        cJSON *line = cJSON_GetObjectItem(cJSON_GetObjectItem(d, "line"), "name");
        cJSON *dir = cJSON_GetObjectItem(d, "direction");
        if (!cJSON_IsString(when) || !cJSON_IsString(line) || !cJSON_IsString(dir)) continue;

        // "2026-09-24T22:41:00+02:00" -- already local time, keep HH:MM.
        bvg_departure_t *dep = &result.dep[result.count];
        if (sscanf(when->valuestring, "%*d-%*d-%*dT%d:%d", &dep->hour, &dep->minute) != 2) continue;
        bvg_display_name(line->valuestring, dep->line, sizeof(dep->line));
        bvg_display_name(dir->valuestring, dep->direction, sizeof(dep->direction));
        result.count++;
    }

    cJSON_Delete(root);
    *out = result;
    return true;
}
