#include "presence.h"
#include "app_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

static const char *TAG = "presence";
static presence_cb_t s_cb = NULL;
static presence_state_t s_state = PRESENCE_NONE;

#if CONFIG_APP_ENABLE_ULTRASONIC
static int measure_distance_cm(void)
{
    gpio_set_level(CONFIG_APP_US_TRIG_GPIO, 0);
    esp_rom_delay_us(2);
    gpio_set_level(CONFIG_APP_US_TRIG_GPIO, 1);
    esp_rom_delay_us(10);
    gpio_set_level(CONFIG_APP_US_TRIG_GPIO, 0);

    int64_t start = esp_timer_get_time();
    while (gpio_get_level(CONFIG_APP_US_ECHO_GPIO) == 0) {
        if (esp_timer_get_time() - start > 30000) return -1; // timeout 30ms
    }
    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(CONFIG_APP_US_ECHO_GPIO) == 1) {
        if (esp_timer_get_time() - echo_start > 30000) return -1;
    }
    int64_t echo_end = esp_timer_get_time();
    int64_t pulse_width = echo_end - echo_start; // microseconds
    int distance = (int)(pulse_width * 0.034f / 2.0f); // in cm
    return distance;
}
#endif

static presence_state_t determine_state(void)
{
#if CONFIG_APP_ENABLE_ULTRASONIC
    int dist = measure_distance_cm();
    if (dist > 0 && dist <= CONFIG_APP_US_NEAR_CM) {
        return PRESENCE_NEAR;
    }
#endif
#if CONFIG_APP_ENABLE_PIR
    if (gpio_get_level(CONFIG_APP_PIR_GPIO)) {
        return PRESENCE_PRESENT;
    }
#endif
    return PRESENCE_NONE;
}

static void presence_task(void *arg)
{
    (void)arg;
    for (;;) {
        presence_state_t new_state = determine_state();
        if (new_state != s_state) {
            s_state = new_state;
            if (s_cb) s_cb(s_state);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

esp_err_t presence_start(presence_cb_t cb)
{
#if !(CONFIG_APP_ENABLE_PIR || CONFIG_APP_ENABLE_ULTRASONIC)
    return ESP_ERR_NOT_SUPPORTED;
#else
    s_cb = cb;
#if CONFIG_APP_ENABLE_PIR
    gpio_config_t pir_conf = {
        .pin_bit_mask = 1ULL << CONFIG_APP_PIR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&pir_conf);
#endif
#if CONFIG_APP_ENABLE_ULTRASONIC
    gpio_config_t trig = {
        .pin_bit_mask = 1ULL << CONFIG_APP_US_TRIG_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&trig);
    gpio_config_t echo = {
        .pin_bit_mask = 1ULL << CONFIG_APP_US_ECHO_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&echo);
#endif
    xTaskCreate(presence_task, "presence", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Presence monitoring started");
    return ESP_OK;
#endif
}
