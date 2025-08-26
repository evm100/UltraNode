#pragma once
#include <stdbool.h>
#include <esp_err.h>


typedef void (*ota_request_cb_t)(const char* url);
typedef void (*ota_schedule_cb_t)(int hour, int minute, const char* url);

typedef struct {
    ota_request_cb_t  on_ota_now;
    ota_schedule_cb_t on_ota_schedule_update;
} mqtt_callbacks_t;


esp_err_t mqtt_ctrl_start(const mqtt_callbacks_t *cbs);
void mqtt_ctrl_publish_info(void);
