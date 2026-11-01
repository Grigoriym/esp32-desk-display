#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "metrics_format.h"

// Starts the upload task for the dashboard (server/). ESP_ERR_NOT_SUPPORTED
// when metrics_secrets.h leaves METRICS_HOST empty: uploads stay off.
esp_err_t metrics_start(void);

// Queues one minute's readings for upload and returns straight away; the task
// POSTs them, so a slow or down server never blocks the caller. What can't be
// uploaded goes to the backlog partition on flash and is sent, oldest first,
// once the server answers again.
void metrics_submit(const metrics_record_t *record);
