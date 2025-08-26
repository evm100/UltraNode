#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { uint8_t r, g, b; } rgb_t;

/** Initialize the LED strip driver (WS2812/WS2812B) */
void ws2812_init(int led_count, int gpio);

/** Push a full RGB buffer to the strip */
void ws2812_set_colors(size_t led_count, const rgb_t *colors);

#ifdef __cplusplus
}
#endif
