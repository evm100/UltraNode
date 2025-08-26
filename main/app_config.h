#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "sdkconfig.h"

#define APP_API_BEARER CONFIG_APP_API_BEARER
#define APP_FIRMWARE_NAME "ESP32-ULTRALIGHTS-NODE"
#define APP_FIRMWARE_VERSION "1.0.0"


void app_config_get_wifi(char **ssid, char **pass);
const char* app_config_get_mqtt_url(void);
const char* app_config_get_mqtt_base(void);
int app_config_get_led_gpio(void);
int app_config_get_led_count(void);
float app_config_get_default_brightness(void); // returns 0.0..1.0
bool app_config_ota_skip_cert_validation(void);

