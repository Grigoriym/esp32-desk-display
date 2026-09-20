#pragma once

#include "esp_err.h"

// Brings up WiFi station mode and blocks until connected, retrying on
// disconnect. Logs the assigned IP once connected.
esp_err_t wifi_connect(void);
