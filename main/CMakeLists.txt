idf_component_register(SRCS "main.c" "display.c" "font.c" "wifi.c" "clock.c" "clock_time.c" "weather.c" "weather_parse.c" "bme280.c" "encoder.c" "encoder_decode.c" "bvg.c" "bvg_parse.c" "screens.c" "health.c"
                        INCLUDE_DIRS "."
                        REQUIRES esp_driver_i2c esp_driver_gpio esp_wifi esp_netif nvs_flash esp_event
                                 esp_http_client mbedtls cjson)
