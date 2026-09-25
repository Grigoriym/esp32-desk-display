#include <math.h>
#include <stddef.h>
#include "air_parse.h"
#include "cJSON.h"

// JSON field and display name per pollen_t.
static const struct {
    const char *field;
    const char *name;
} POLLEN[POLLEN_COUNT] = {
    [POLLEN_ALDER] = {"alder_pollen", "ALDER"},       [POLLEN_BIRCH] = {"birch_pollen", "BIRCH"},
    [POLLEN_GRASS] = {"grass_pollen", "GRASS"},       [POLLEN_MUGWORT] = {"mugwort_pollen", "MUGWORT"},
    [POLLEN_RAGWEED] = {"ragweed_pollen", "RAGWEED"},
};

bool air_parse(const char *json, air_t *out)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;

    cJSON *current = cJSON_GetObjectItem(root, "current");
    cJSON *aqi = current ? cJSON_GetObjectItem(current, "european_aqi") : NULL;
    if (!cJSON_IsNumber(aqi)) {
        cJSON_Delete(root);
        return false;
    }

    air_t a;
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference): cJSON_IsNumber(NULL) is false (cJSON.c is another TU)
    a.aqi = (int)lround(aqi->valuedouble);
    for (int i = 0; i < POLLEN_COUNT; i++) {
        cJSON *v = cJSON_GetObjectItem(current, POLLEN[i].field);
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference): as above
        a.pollen[i] = cJSON_IsNumber(v) ? (int)lround(v->valuedouble) : 0;
    }
    *out = a;
    cJSON_Delete(root);
    return true;
}

const char *air_aqi_label(int aqi)
{
    if (aqi <= 20) return "GOOD";
    if (aqi <= 40) return "FAIR";
    if (aqi <= 60) return "MODERATE";
    if (aqi <= 80) return "POOR";
    if (aqi <= 100) return "VERY POOR";
    return "EXTREME";
}

const char *air_pollen_name(pollen_t p)
{
    return (p >= 0 && p < POLLEN_COUNT) ? POLLEN[p].name : "";
}

const char *air_pollen_level(int grains)
{
    if (grains < 1) return NULL;
    if (grains <= 10) return "LOW";
    if (grains <= 50) return "MED";
    return "HIGH";
}
