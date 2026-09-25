#pragma once

#include <stdbool.h>
#include "esp_err.h"

// Starts the upload task for the dashboard (server/). ESP_ERR_NOT_SUPPORTED
// when metrics_secrets.h leaves METRICS_HOST empty: uploads stay off.
esp_err_t metrics_start(void);

// Queues one batch of line protocol (see metrics_format.h) for upload and
// returns straight away; the task POSTs it, so a slow or down server never
// blocks the caller. A batch still pending is replaced by the newer one.
void metrics_submit(const char *lines);
