#pragma once

#include "holidays_parse.h"
#include "esp_err.h"

// Fetches the given year's public holidays that apply in Berlin from
// Nager.Date. *out is only written on success.
esp_err_t holidays_fetch(int year, holidays_t *out);
