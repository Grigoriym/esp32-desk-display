#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "web.h"
#include "web_api.h"
#include "encoder.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "mdns.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "web";

#define MDNS_HOSTNAME   "desk" // -> http://desk.local
#define MDNS_INSTANCE   "Desk display"
#define POLL_ACTIVE_SEC 120 // see web_recently_polled()

// web_index.html, embedded by main/CMakeLists.txt; EMBED_TXTFILES
// NUL-terminates it, so it's a plain C string.
extern const char INDEX_HTML[] asm("_binary_web_index_html_start");

static SemaphoreHandle_t s_lock; // guards the three below
static screen_data_t s_data;
static screen_t s_screen;
static bool s_panel_on;
static volatile TickType_t s_last_poll;
static volatile bool s_polled;

void web_publish(const screen_data_t *data, screen_t screen, bool panel_on)
{
    if (!s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_data = *data;
    s_screen = screen;
    s_panel_on = panel_on;
    xSemaphoreGive(s_lock);
}

bool web_recently_polled(void)
{
    return s_polled && xTaskGetTickCount() - s_last_poll < pdMS_TO_TICKS(POLL_ACTIVE_SEC * 1000);
}

static esp_err_t index_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_sendstr(req, INDEX_HTML);
}

static esp_err_t status_get(httpd_req_t *req)
{
    s_last_poll = xTaskGetTickCount();
    s_polled = true;

    web_view_t view = {0};
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    strftime(view.time, sizeof(view.time), "%H:%M", &t);
    strftime(view.date, sizeof(view.date), "%Y-%m-%d", &t);
    view.now_min = t.tm_hour * 60 + t.tm_min;

    char *buf = malloc(WEB_STATUS_MAX); // heap: the server task's stack is small
    if (!buf) return httpd_resp_send_500(req);
    xSemaphoreTake(s_lock, portMAX_DELAY);
    view.screen = s_screen;
    view.panel_on = s_panel_on;
    int n = web_status_json(&s_data, &view, buf, WEB_STATUS_MAX);
    xSemaphoreGive(s_lock);

    esp_err_t err;
    if (n < 0) {
        ESP_LOGW(TAG, "status JSON too big");
        err = httpd_resp_send_500(req);
    } else {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        err = httpd_resp_send(req, buf, n);
    }
    free(buf);
    return err;
}

// POST handlers: the value comes from ?<key>=..., parsed by req->user_ctx
// (web_panel_command / web_screen_command), and goes to the main loop
// through the encoder queue.
typedef bool (*command_parser_t)(const char *value, input_event_t *out);
typedef struct {
    const char *key;
    command_parser_t parse;
} command_route_t;

static esp_err_t command_post(httpd_req_t *req)
{
    const command_route_t *route = req->user_ctx;
    char query[48];
    char value[16];
    input_event_t ev;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK
        || httpd_query_key_value(query, route->key, value, sizeof(value)) != ESP_OK
        || !route->parse(value, &ev)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad or missing value, see docs/API.md");
    }
    if (!encoder_post(ev)) return httpd_resp_send_500(req);
    ESP_LOGI(TAG, "%s %s=%s", req->uri, route->key, value);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

static const command_route_t PANEL_ROUTE = {"set", web_panel_command};
static const command_route_t SCREEN_ROUTE = {"go", web_screen_command};

static esp_err_t start_mdns(void)
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK) return err;
    mdns_hostname_set(MDNS_HOSTNAME);
    mdns_instance_name_set(MDNS_INSTANCE);
    // For NSD discovery (Android app, ROADMAP task 12).
    return mdns_service_add(MDNS_INSTANCE, "_http", "_tcp", 80, NULL, 0);
}

esp_err_t web_start(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    esp_err_t err = start_mdns();
    if (err != ESP_OK) ESP_LOGW(TAG, "mDNS not used (use the IP instead): %s", esp_err_to_name(err));

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true; // a phone that drops off doesn't hold a socket forever
    httpd_handle_t server;
    err = httpd_start(&server, &config);
    if (err != ESP_OK) return err;

    const httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = index_get},
        {.uri = "/api/status", .method = HTTP_GET, .handler = status_get},
        {.uri = "/api/panel", .method = HTTP_POST, .handler = command_post, .user_ctx = (void *)&PANEL_ROUTE},
        {.uri = "/api/screen",
         .method = HTTP_POST,
         .handler = command_post,
         .user_ctx = (void *)&SCREEN_ROUTE},
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        httpd_register_uri_handler(server, &routes[i]);
    }
    ESP_LOGI(TAG, "serving on http://" MDNS_HOSTNAME ".local/");
    return ESP_OK;
}
