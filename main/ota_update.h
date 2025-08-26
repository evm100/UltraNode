#pragma once
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void      ota_init(void);                         // starts the daily scheduler if NVS has a schedule
esp_err_t ota_request_now(const char* url);       // spawn an OTA task immediately (non-blocking)

void      ota_schedule_set(int hour, int minute, const char* url);  // persists to NVS
bool      ota_schedule_get(int *hour, int *minute, char *url, int url_sz);
void      ota_schedule_clear(void);               // disable daily scheduler (and persist)
bool      ota_is_busy(void);                      // true while an OTA is running

#ifdef __cplusplus
}
#endif
