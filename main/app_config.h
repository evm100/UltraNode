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

#ifdef CONFIG_APP_USE_PIR
int app_config_get_pir_gpio(void);
#endif
#ifdef CONFIG_APP_USE_ULTRASONIC
int app_config_get_ultrasonic_trig_gpio(void);
int app_config_get_ultrasonic_echo_gpio(void);
int app_config_get_ultrasonic_near_cm(void);
#endif

float app_config_get_present_brightness(void); // returns 0.0..1.0
float app_config_get_near_brightness(void);    // returns 0.0..1.0
int app_config_get_near_light_gpio1(void);
int app_config_get_near_light_gpio2(void);

