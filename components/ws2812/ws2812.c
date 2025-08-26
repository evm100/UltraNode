#include "ws2812.h"
#include "led_strip.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "WS2812_LEDSTRIP";

static led_strip_handle_t s_strip = NULL;
static int s_count = 0;

void ws2812_init(int led_count, int gpio)
{
    s_count = led_count;

    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio,
        .max_leds = led_count,
        .flags.invert_out = false,
        // Some led_strip versions expose .led_model (others don’t)
        // If your version has it, we set it to WS2812 (GRB order).
        // If not, it’s fine; WS2812 is default or selected by factory.
        #ifdef LED_MODEL_WS2812
        .led_model = LED_MODEL_WS2812,
        #endif
        // Note: older versions DO NOT have .led_pixel_format
        // Newer versions might; we skip it for broad compatibility.
    };

    // RMT backend configuration (safe defaults across versions)
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
        .mem_block_symbols = 0,            // auto
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (err != ESP_OK || !s_strip) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device failed: %d", err);
        return;
    }

    // Clear once
    for (int i = 0; i < s_count; ++i) {
        led_strip_set_pixel(s_strip, i, 0, 0, 0);
    }
    led_strip_refresh(s_strip);
    ESP_LOGI(TAG, "WS2812: %d LEDs on GPIO %d (led_strip)", led_count, gpio);
}

void ws2812_set_colors(size_t led_count, const rgb_t *colors)
{
    if (!s_strip || !colors || (int)led_count > s_count) return;

    // API expects RGB; driver handles GRB order internally based on model
    for (int i = 0; i < (int)led_count; ++i) {
        led_strip_set_pixel(s_strip, i, colors[i].r, colors[i].g, colors[i].b);
    }
    led_strip_refresh(s_strip);
}
