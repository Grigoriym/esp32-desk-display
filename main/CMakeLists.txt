idf_component_register(SRCS "main.c" "display.c" "wifi.c" "clock.c" "weather.c"
                        INCLUDE_DIRS "."
                        REQUIRES esp_driver_i2c esp_wifi esp_netif nvs_flash esp_event
                                 esp_http_client mbedtls cjson)
