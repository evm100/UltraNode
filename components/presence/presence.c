#include "presence.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

static const char *TAG = "presence";

static presence_cb_t s_cb;
static presence_state_t s_state = PRESENCE_NONE;

#if CONFIG_APP_ENABLE_PIR
static const gpio_num_t PIR_GPIO = CONFIG_APP_PIR_GPIO;
#endif

#if CONFIG_APP_ENABLE_ULTRASONIC
static const gpio_num_t TRIG_GPIO = CONFIG_APP_ULTRASONIC_TRIG_GPIO;
static const gpio_num_t ECHO_GPIO = CONFIG_APP_ULTRASONIC_ECHO_GPIO;
static const int NEAR_CM = CONFIG_APP_ULTRASONIC_NEAR_CM;
#endif

static void presence_task(void *arg) {
    (void)arg;
    while (1) {
        presence_state_t new_state = PRESENCE_NONE;
#if CONFIG_APP_ENABLE_ULTRASONIC
        gpio_set_level(TRIG_GPIO, 0);
        ets_delay_us(2);
        gpio_set_level(TRIG_GPIO, 1);
        ets_delay_us(10);
        gpio_set_level(TRIG_GPIO, 0);
        int64_t t0 = esp_timer_get_time();
        int64_t timeout = t0 + 40000; // 40ms timeout
        while (gpio_get_level(ECHO_GPIO) == 0 && esp_timer_get_time() < timeout) {}
        t0 = esp_timer_get_time();
        while (gpio_get_level(ECHO_GPIO) == 1 && esp_timer_get_time() < timeout) {}
        int64_t t1 = esp_timer_get_time();
        float dist = (t1 - t0) / 58.0f;
        if (dist > 0 && dist <= NEAR_CM) {
            new_state = PRESENCE_NEAR;
        }
#endif
#if CONFIG_APP_ENABLE_PIR
        if (new_state != PRESENCE_NEAR && gpio_get_level(PIR_GPIO)) {
            new_state = PRESENCE_PRESENT;
        }
#endif
        if (new_state != s_state) {
            s_state = new_state;
            if (s_cb) s_cb(s_state);
            ESP_LOGI(TAG, "state %d", s_state);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

esp_err_t presence_init(presence_cb_t cb) {
    s_cb = cb;
#if CONFIG_APP_ENABLE_PIR
    gpio_config_t pir_conf = {
        .pin_bit_mask = 1ULL<<PIR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&pir_conf);
#endif
#if CONFIG_APP_ENABLE_ULTRASONIC
    gpio_config_t trig_conf = {
        .pin_bit_mask = 1ULL<<TRIG_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&trig_conf);
    gpio_set_level(TRIG_GPIO, 0);

    gpio_config_t echo_conf = {
        .pin_bit_mask = 1ULL<<ECHO_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&echo_conf);
#endif
#if CONFIG_APP_ENABLE_PIR || CONFIG_APP_ENABLE_ULTRASONIC
    xTaskCreate(presence_task, "presence", 4096, NULL, 5, NULL);
#endif
    return ESP_OK;
}
