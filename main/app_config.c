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

