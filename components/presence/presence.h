#ifndef PRESENCE_H
#define PRESENCE_H

#include "esp_err.h"

typedef enum {
    PRESENCE_NONE = 0,
    PRESENCE_PRESENT,
    PRESENCE_NEAR,
} presence_state_t;

typedef void (*presence_cb_t)(presence_state_t state);

esp_err_t presence_init(presence_cb_t cb);

#endif // PRESENCE_H
