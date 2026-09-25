#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "alerts_parse.h"
#include "cJSON.h"

// Timestamps look like "2026-09-25T22:00:00+02:00"; the first 16 characters
// ("YYYY-MM-DDTHH:MM") compare as strings in the same timezone.
#define STAMP_LEN 16

static const char *string_field(const cJSON *obj, const char *name)
{
    const cJSON *v = cJSON_GetObjectItem(obj, name);
    return cJSON_IsString(v) ? v->valuestring : "";
}

static alert_severity_t severity_of(const char *s)
{
    if (strcmp(s, "extreme") == 0) return ALERT_EXTREME;
    if (strcmp(s, "severe") == 0) return ALERT_SEVERE;
    if (strcmp(s, "moderate") == 0) return ALERT_MODERATE;
    return ALERT_MINOR;
}

// "HH:MM" when onset is on now's day, else "DD/MM".
static void format_onset(const char *onset, const char *now_local, char *out, size_t len)
{
    if (strlen(onset) < STAMP_LEN) {
        out[0] = '\0';
    } else if (strncmp(onset, now_local, 10) == 0) {
        snprintf(out, len, "%.5s", onset + 11);
    } else {
        snprintf(out, len, "%.2s/%.2s", onset + 8, onset + 5);
    }
}

bool alerts_parse(const char *json, const char *now_local, alerts_t *out)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;
    const cJSON *list = cJSON_GetObjectItem(root, "alerts");
    if (!cJSON_IsArray(list)) {
        cJSON_Delete(root);
        return false;
    }

    alerts_t a = {0};
    const cJSON *item;
    cJSON_ArrayForEach(item, list)
    {
        if (strcmp(string_field(item, "status"), "actual") != 0) continue;
        if (strcmp(string_field(item, "response_type"), "allclear") == 0) continue;

        const char *onset = string_field(item, "onset");
        bool started = strncmp(onset, now_local, STAMP_LEN) <= 0;
        alert_severity_t severity = severity_of(string_field(item, "severity"));
        a.count++;
        if (a.count > 1 && (a.started > started || (a.started == started && a.severity >= severity)))
            continue;

        a.started = started;
        a.severity = severity;
        snprintf(a.event, sizeof(a.event), "%s", string_field(item, "event_en"));
        for (char *c = a.event; *c; c++) *c = (char)toupper((unsigned char)*c);
        format_onset(onset, now_local, a.onset, sizeof(a.onset));
    }
    *out = a;
    cJSON_Delete(root);
    return true;
}
