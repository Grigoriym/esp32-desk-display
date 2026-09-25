#include "encoder.h"
#include "encoder_decode.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "encoder";

#define ENC_CLK_GPIO    GPIO_NUM_25 // channel A
#define ENC_DT_GPIO     GPIO_NUM_26 // channel B
#define ENC_SW_GPIO     GPIO_NUM_27 // push button, active low
#define BTN_POLL_MS     10          // = 1 tick at CONFIG_FREERTOS_HZ=100
#define BTN_DEBOUNCE_MS 30

static QueueHandle_t s_events;
static enc_rotation_t s_rotation;

static uint8_t IRAM_ATTR pin_state(void)
{
    return (gpio_get_level(ENC_CLK_GPIO) << 1) | gpio_get_level(ENC_DT_GPIO);
}

// Rotation is decoded in the pin-change interrupt: polling at the 10ms tick
// would miss steps on a quick spin (bring-up saw ~30ms per detent).
static void IRAM_ATTR encoder_isr(void *arg)
{
    int click = enc_rotation_update(&s_rotation, pin_state());
    if (click == 0) return;
    encoder_event_t ev = (click > 0) ? ENCODER_EV_CW : ENCODER_EV_CCW;
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(s_events, &ev, &woken);
    portYIELD_FROM_ISR(woken);
}

static void button_task(void *arg)
{
    enc_button_t button;
    enc_button_init(&button);
    for (;;) {
        if (enc_button_update(&button, gpio_get_level(ENC_SW_GPIO), BTN_POLL_MS, BTN_DEBOUNCE_MS)) {
            encoder_event_t ev = ENCODER_EV_PRESS;
            xQueueSend(s_events, &ev, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(BTN_POLL_MS));
    }
}

esp_err_t encoder_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ENC_CLK_GPIO) | (1ULL << ENC_DT_GPIO) | (1ULL << ENC_SW_GPIO),
        .mode = GPIO_MODE_INPUT,
        // The KY-040 has 10k pull-ups on CLK/DT, but SW's is often left
        // unpopulated -- the internal ones cover both cases.
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) return err;

    s_events = xQueueCreate(16, sizeof(encoder_event_t));
    if (!s_events) return ESP_ERR_NO_MEM;

    enc_rotation_init(&s_rotation, pin_state());
    gpio_set_intr_type(ENC_CLK_GPIO, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(ENC_DT_GPIO, GPIO_INTR_ANYEDGE);
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err; // INVALID_STATE = already installed
    gpio_isr_handler_add(ENC_CLK_GPIO, encoder_isr, NULL);
    gpio_isr_handler_add(ENC_DT_GPIO, encoder_isr, NULL);

    xTaskCreate(button_task, "enc_button", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "ready (CLK=%d DT=%d SW=%d)", ENC_CLK_GPIO, ENC_DT_GPIO, ENC_SW_GPIO);
    return ESP_OK;
}

bool encoder_wait_event(encoder_event_t *ev, TickType_t timeout)
{
    if (!s_events) {
        vTaskDelay(timeout);
        return false;
    }
    return xQueueReceive(s_events, ev, timeout) == pdTRUE;
}
