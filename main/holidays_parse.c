#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "holidays_parse.h"
#include "cJSON.h"

// "New Year's Day" -> "NEW YEARS DAY": the font has no '.' or '\'', so those
// are dropped rather than drawn as gaps.
static void display_name(const char *src, char *dst, size_t len)
{
    size_t n = 0;
    for (; *src && n + 1 < len; src++) {
        unsigned char c = (unsigned char)*src;
        if (isalnum(c) || (c == ' ' && n > 0 && dst[n - 1] != ' ')) dst[n++] = (char)toupper(c);
    }
    while (n > 0 && dst[n - 1] == ' ') n--;
    dst[n] = '\0';
}

// "2026-10-03" -> 20261003, or 0 if it isn't a date in that form.
static int parse_ymd(const char *s)
{
    static const char FORMAT[] = "dddd-dd-dd";
    int ymd = 0;
    for (int i = 0; FORMAT[i]; i++) {
        if (FORMAT[i] == '-') {
            if (s[i] != '-') return 0;
        } else {
            if (!isdigit((unsigned char)s[i])) return 0;
            ymd = ymd * 10 + (s[i] - '0');
        }
    }
    return ymd;
}

// Nationwide ("global": true) or listing region among its "counties".
static bool applies_to(const cJSON *item, const char *region)
{
    if (cJSON_IsTrue(cJSON_GetObjectItem(item, "global"))) return true;
    const cJSON *c;
    cJSON_ArrayForEach(c, cJSON_GetObjectItem(item, "counties"))
    {
        if (cJSON_IsString(c) && strcmp(c->valuestring, region) == 0) return true;
    }
    return false;
}

bool holidays_parse(const char *json, int year, const char *region, holidays_t *out)
{
    cJSON *root = cJSON_Parse(json);
    if (!cJSON_IsArray(root)) {
        cJSON_Delete(root);
        return false;
    }

    holidays_t h = {.year = year};
    const cJSON *item;
    cJSON_ArrayForEach(item, root)
    {
        const cJSON *date = cJSON_GetObjectItem(item, "date");
        const cJSON *name = cJSON_GetObjectItem(item, "name");
        if (!cJSON_IsString(date) || !cJSON_IsString(name) || !applies_to(item, region)) continue;
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference): cJSON_IsString(NULL) is false (cJSON.c is another TU)
        int ymd = parse_ymd(date->valuestring);
        if (ymd == 0) continue;
        if (h.count == HOLIDAYS_MAX) break;
        holiday_t *hd = &h.day[h.count++];
        hd->ymd = ymd;
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference): as above
        display_name(name->valuestring, hd->name, sizeof(hd->name));
    }
    *out = h;
    cJSON_Delete(root);
    return true;
}

int holidays_next(const holidays_t *h, int today_ymd)
{
    for (int i = 0; i < h->count; i++) {
        if (h->day[i].ymd >= today_ymd) return i;
    }
    return -1;
}

int holidays_wanted_year(const holidays_t *h, int today_ymd)
{
    int this_year = today_ymd / 10000;
    if (h->year < this_year) return this_year;
    if (h->year == this_year && holidays_next(h, today_ymd) < 0) return this_year + 1;
    return 0;
}
