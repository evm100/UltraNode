#include "presence.h"
#include "app_config.h"
#include "effects.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

static const char *TAG = "presence";

static bool s_present = false;
static bool s_near = false;

static void apply_state(void) {
    float pct = 0.0f;
    if (s_near) pct = app_config_get_near_brightness();
    else if (s_present) pct = app_config_get_present_brightness();

    uint8_t level = (uint8_t)(255.0f * pct);
    rgb_t c = {level, level, level};
    effects_set_static(c);

    int g1 = app_config_get_near_light_gpio1();
    int g2 = app_config_get_near_light_gpio2();
    if (g1 >= 0) gpio_set_level(g1, s_near);
    if (g2 >= 0) gpio_set_level(g2, s_near);
}

#ifdef CONFIG_APP_USE_PIR
static void pir_task(void *arg) {
    int gpio = app_config_get_pir_gpio();
    gpio_set_direction(gpio, GPIO_MODE_INPUT);
    for (;;) {
        bool st = gpio_get_level(gpio);
        if (st != s_present) {
            s_present = st;
            ESP_LOGI(TAG, "PIR %s", s_present ? "present" : "clear");
            apply_state();
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
#endif

#ifdef CONFIG_APP_USE_ULTRASONIC
static void ultrasonic_task(void *arg) {
    int trig = app_config_get_ultrasonic_trig_gpio();
    int echo = app_config_get_ultrasonic_echo_gpio();
    int near_cm = app_config_get_ultrasonic_near_cm();

    gpio_set_direction(trig, GPIO_MODE_OUTPUT);
    gpio_set_direction(echo, GPIO_MODE_INPUT);
    gpio_set_level(trig, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    for (;;) {
        gpio_set_level(trig, 1);
        esp_rom_delay_us(10);
        gpio_set_level(trig, 0);

        uint32_t start = esp_timer_get_time();
        while (!gpio_get_level(echo) && (esp_timer_get_time() - start) < 30000) {}
        start = esp_timer_get_time();
        while (gpio_get_level(echo) && (esp_timer_get_time() - start) < 30000) {}
        uint32_t duration = esp_timer_get_time() - start;
        float dist_cm = duration / 58.0f;

        bool n = dist_cm > 0 && dist_cm < near_cm;
        if (n != s_near) {
            s_near = n;
            ESP_LOGI(TAG, "Ultrasonic %s %.1fcm", s_near ? "near" : "far", dist_cm);
            apply_state();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
#endif

void presence_init(void) {
    int g1 = app_config_get_near_light_gpio1();
    int g2 = app_config_get_near_light_gpio2();
    if (g1 >= 0) {
        gpio_set_direction(g1, GPIO_MODE_OUTPUT);
        gpio_set_level(g1, 0);
    }
    if (g2 >= 0) {
        gpio_set_direction(g2, GPIO_MODE_OUTPUT);
        gpio_set_level(g2, 0);
    }
#ifdef CONFIG_APP_USE_PIR
    xTaskCreate(pir_task, "pir_task", 2048, NULL, 5, NULL);
#endif
#ifdef CONFIG_APP_USE_ULTRASONIC
    xTaskCreate(ultrasonic_task, "ultra_task", 4096, NULL, 5, NULL);
#endif
}

