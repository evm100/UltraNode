#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdint.h>
#include <stdbool.h>
#include "ws2812.h"   // defines rgb_t {uint8_t r,g,b;}

typedef enum {
  EFFECT_STATIC=0,
  EFFECT_RAINBOW,
  EFFECT_THEATER_CHASE,
  EFFECT_FIRE,
  EFFECT_WAVE,
  EFFECT_TWINKLE,
  EFFECT_COLOR_WIPE,
  EFFECT_STROBE,
  EFFECT_BREATHING,
  EFFECT_COMET,
  EFFECT_SPARKLE,
  EFFECT_SPACEY_GRADIENT,
  EFFECT_MAX
} effect_t;

// Lifecycle
void effects_init(int led_count, int gpio_num);

// State setters
// 1) Set the static color AND switch to static effect
void effects_set_static(rgb_t color);

// 2) Change only the stored static color (keep current effect)
void effects_set_static_color(rgb_t color);

// 3) Change the current running effect (stored static color is kept)
void effects_set_effect(effect_t effect);

// Effect-specific config
void effects_configure_spacey(rgb_t primary, rgb_t secondary, rgb_t tertiary);

// Per-frame update (~30ms cadence)
void effects_update(void);

// ------- New getters to support persistence / UI readback -------
rgb_t   effects_get_static(void);   // last stored static color
effect_t effects_get_effect(void);  // current effect

#endif /* EFFECTS_H */
