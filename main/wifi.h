#pragma once

#include <stdbool.h>
#include "esp_err.h"

// Brings up WiFi station mode and waits up to timeout_ms for a connection.
// Returns ESP_ERR_TIMEOUT if none by then; it keeps retrying in the
// background either way (also after a later disconnect). Logs the IP once
// connected.
esp_err_t wifi_connect(int timeout_ms);

// True while the station is connected and has an IP.
bool wifi_is_connected(void);

// Signal strength of the current connection in dBm (about -30 great, -80
// poor). False while not connected.
bool wifi_rssi(int *out);
