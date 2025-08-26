#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "app_config.h"


static const char *TAG = "wifi";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry;
#define WIFI_CONNECTED_BIT BIT0


static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
esp_wifi_connect();
} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
if (s_retry < 10) {
esp_wifi_connect(); s_retry++;
ESP_LOGW(TAG, "retry to connect to the AP");
}
xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
s_retry = 0;
xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
ESP_LOGI(TAG, "got ip");
}
}


esp_err_t wifi_setup_start(void) {
ESP_ERROR_CHECK(esp_netif_init());
ESP_ERROR_CHECK(esp_event_loop_create_default());
esp_netif_create_default_wifi_sta();


wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
ESP_ERROR_CHECK(esp_wifi_init(&cfg));
s_wifi_event_group = xEventGroupCreate();


ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));


char *ssid, *pass; app_config_get_wifi(&ssid, &pass);


wifi_config_t wifi_config = {0};
strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;


ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
ESP_ERROR_CHECK(esp_wifi_start());


EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));
if (!(bits & WIFI_CONNECTED_BIT)) return ESP_FAIL;
return ESP_OK;
}


bool wifi_is_connected(void) {
EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
return (bits & WIFI_CONNECTED_BIT);
}
