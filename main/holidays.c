#include <stdio.h>
#include <stdlib.h>
#include "holidays.h"
#include "http.h"

// Nager.Date: free, no key. One year of Germany's holidays is ~3.6 KB (all
// states' ones included), so the buffer is heap-allocated for the fetch only.
#define HOLIDAYS_URL      "https://date.nager.at/api/v3/PublicHolidays/%d/DE"
#define HOLIDAYS_REGION   "DE-BE"
#define HOLIDAYS_BUF_SIZE (8 * 1024)

esp_err_t holidays_fetch(int year, holidays_t *out)
{
    char url[64];
    snprintf(url, sizeof(url), HOLIDAYS_URL, year);
    char *buf = malloc(HOLIDAYS_BUF_SIZE);
    if (!buf) return ESP_ERR_NO_MEM;
    esp_err_t err = http_get(url, buf, HOLIDAYS_BUF_SIZE);
    if (err == ESP_OK && !holidays_parse(buf, year, HOLIDAYS_REGION, out)) err = ESP_FAIL;
    free(buf);
    return err;
}
