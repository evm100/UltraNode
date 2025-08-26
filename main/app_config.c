#include "app_config.h"
#include "sdkconfig.h"

void app_config_get_wifi(char **ssid, char **pass) {
    static char *s = CONFIG_APP_WIFI_SSID;
    static char *p = CONFIG_APP_WIFI_PASS;
    *ssid = s; *pass = p;
}

const char* app_config_get_mqtt_url(void) {
    return CONFIG_APP_MQTT_URI;
}

const char* app_config_get_mqtt_base(void) {
    return CONFIG_APP_MQTT_BASE_TOPIC;
}

int app_config_get_led_gpio(void) {
    return CONFIG_APP_LED_GPIO;
}

int app_config_get_led_count(void) {
    return CONFIG_APP_LED_COUNT;
}

float app_config_get_default_brightness(void) {
#ifdef CONFIG_APP_DEFAULT_BRIGHTNESS_PCT
    return ((float)CONFIG_APP_DEFAULT_BRIGHTNESS_PCT) / 100.0f;
#else
    return 1.0f;
#endif
}

bool app_config_ota_skip_cert_validation(void) {
    /* Guard against CONFIG_APP_OTA_SKIP_CERT_VALIDATION not being defined. */
#ifdef CONFIG_APP_OTA_SKIP_CERT_VALIDATION
    return CONFIG_APP_OTA_SKIP_CERT_VALIDATION;
#else
    return false;
#endif
}

#ifdef CONFIG_APP_USE_PIR
int app_config_get_pir_gpio(void) {
    return CONFIG_APP_PIR_GPIO;
}
#endif

#ifdef CONFIG_APP_USE_ULTRASONIC
int app_config_get_ultrasonic_trig_gpio(void) {
    return CONFIG_APP_ULTRASONIC_TRIG_GPIO;
}

int app_config_get_ultrasonic_echo_gpio(void) {
    return CONFIG_APP_ULTRASONIC_ECHO_GPIO;
}

int app_config_get_ultrasonic_near_cm(void) {
    return CONFIG_APP_ULTRASONIC_NEAR_CM;
}
#endif

float app_config_get_present_brightness(void) {
#ifdef CONFIG_APP_PRESENT_BRIGHTNESS_PCT
    return ((float)CONFIG_APP_PRESENT_BRIGHTNESS_PCT) / 100.0f;
#else
    return 0.1f;
#endif
}

float app_config_get_near_brightness(void) {
#ifdef CONFIG_APP_NEAR_BRIGHTNESS_PCT
    return ((float)CONFIG_APP_NEAR_BRIGHTNESS_PCT) / 100.0f;
#else
    return 1.0f;
#endif
}

int app_config_get_near_light_gpio1(void) {
#ifdef CONFIG_APP_NEAR_LIGHT_GPIO1
    return CONFIG_APP_NEAR_LIGHT_GPIO1;
#else
    return -1;
#endif
}

int app_config_get_near_light_gpio2(void) {
#ifdef CONFIG_APP_NEAR_LIGHT_GPIO2
    return CONFIG_APP_NEAR_LIGHT_GPIO2;
#else
    return -1;
#endif
}

