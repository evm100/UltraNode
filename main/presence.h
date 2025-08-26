#pragma once
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PRESENCE_NONE = 0,
    PRESENCE_PRESENT,
    PRESENCE_NEAR
} presence_state_t;

typedef void (*presence_cb_t)(presence_state_t state);

esp_err_t presence_start(presence_cb_t cb);

#ifdef __cplusplus
}
#endif
