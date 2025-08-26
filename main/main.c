#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "app_config.h"
#include "wifi_setup.h"
#include "mqtt_ctrl.h"
#include "ota_update.h"
#include "time_sync.h"
#include "effects.h"
#include "ws2812.h"
#include "presence.h"

static const char *TAG = "APP";

#define NVS_NS_STATE "state"

static void state_save(effect_t eff, rgb_t color) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS_STATE, NVS_READWRITE, &h) != ESP_OK) return;
    uint8_t e = (uint8_t)eff;
    nvs_set_u8(h, "eff", e);
    nvs_set_blob(h, "color", &color, sizeof(color));
    nvs_commit(h);
    nvs_close(h);
}

static bool state_load(effect_t *eff, rgb_t *color) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS_STATE, NVS_READONLY, &h) != ESP_OK) return false;
    uint8_t e; size_t len = sizeof(*color);
    esp_err_t ok1 = nvs_get_u8(h, "eff", &e);
    esp_err_t ok2 = nvs_get_blob(h, "color", color, &len);
    nvs_close(h);
    if (ok1 == ESP_OK && ok2 == ESP_OK && len == sizeof(*color)) { *eff = (effect_t)e; return true; }
    return false;
}

static void nvs_init_robust(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void effect_task(void *arg) {
    (void)arg;
    for (;;) {
        effects_update();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

static void mqtt_ota_now(const char* url) {
    ota_request_now(url);
}

static void mqtt_ota_schedule(int hour, int minute, const char* url) {
    ota_schedule_set(hour, minute, url);
}

static void presence_state_changed(presence_state_t state) {
    int total = app_config_get_led_count();
    int extra = app_config_near_extra_leds();
    if (extra > total) extra = 0;
    int base = total - extra;

    int pct = 0;
    switch (state) {
        case PRESENCE_NEAR:
            pct = app_config_near_brightness_pct();
            break;
        case PRESENCE_PRESENT:
            pct = app_config_present_brightness_pct();
            break;
        default:
            pct = 0;
            break;
    }

    rgb_t high = {255,255,255};
    high.r = high.g = high.b = (uint8_t)(pct * 255 / 100);
    rgb_t off = {0,0,0};

    rgb_t *buf = calloc(total, sizeof(rgb_t));
    if (!buf) return;

    for (int i = 0; i < base; ++i) buf[i] = high;
    if (state == PRESENCE_NEAR) {
        for (int i = base; i < total; ++i) buf[i] = high;
    } else {
        for (int i = base; i < total; ++i) buf[i] = off;
    }

    ws2812_set_colors(total, buf);
    free(buf);
}

void app_main(void) {
    nvs_init_robust();

    ESP_ERROR_CHECK(wifi_setup_start());
    time_sync_start();
    ota_init();

    int led_gpio = app_config_get_led_gpio();
    int led_count = app_config_get_led_count();
    ws2812_init(led_count, led_gpio);
    effects_init(led_count, led_gpio);

    effect_t eff; rgb_t col;
    if (state_load(&eff, &col)) {
        effects_set_static_color(col);
        if (eff == EFFECT_STATIC) effects_set_static(col);
        else effects_set_effect(eff);
    } else {
        effects_set_static((rgb_t){0,0,0});
        state_save(EFFECT_STATIC, (rgb_t){0,0,0});
    }

    xTaskCreate(effect_task, "effect", 4096, NULL, 5, NULL);

    mqtt_callbacks_t cbs = {
        .on_ota_now = mqtt_ota_now,
        .on_ota_schedule_update = mqtt_ota_schedule,
    };
    ESP_ERROR_CHECK(mqtt_ctrl_start(&cbs));
    mqtt_ctrl_publish_info();

#if CONFIG_APP_ENABLE_PIR || CONFIG_APP_ENABLE_ULTRASONIC
    ESP_ERROR_CHECK(presence_start(presence_state_changed));
#endif

    ESP_LOGI(TAG, "Setup complete");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

