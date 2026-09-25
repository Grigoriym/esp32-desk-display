#pragma once

#include "esp_err.h"

// GETs url (HTTPS via the certificate bundle, 10 s timeout) into buf,
// NUL-terminated and cut at size - 1 bytes; ESP_OK only on HTTP 200.
esp_err_t http_get(const char *url, char *buf, int size);
