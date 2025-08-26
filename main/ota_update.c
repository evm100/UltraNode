#include "ota_update.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#include "time_sync.h"  // your helper that returns epoch seconds

static const char *TAG = "ota";

static nvs_handle_t s_nvs;
static int  s_hour = -1, s_minute = -1;
static char s_url[256];
static TaskHandle_t s_sched_task = NULL;
static volatile bool s_ota_busy = false;

// -------- internals --------

static esp_err_t _http_client_init(esp_http_client_handle_t client) {
    // Attach Bearer if configured
#ifdef APP_API_BEARER
    if (APP_API_BEARER && APP_API_BEARER[0]) {
        char auth[256];
        snprintf(auth, sizeof(auth), "Bearer %s", APP_API_BEARER);
        esp_http_client_set_header(client, "Authorization", auth);
    }
#endif
    esp_http_client_set_header(client, "User-Agent", "esp32-ultralights/1");
#if CONFIG_APP_OTA_SKIP_CERT_VALIDATION
    // DEV ONLY: you had this before
    esp_http_client_set_transport_keep_alive(client, true);
#endif
    return ESP_OK;
}

static void do_ota_from_url(const char* url) {
    if (!url || !url[0]) return;

    s_ota_busy = true;
    ESP_LOGI(TAG, "Starting OTA: %s", url);

    esp_http_client_config_t http = {
        .url               = url,
        .timeout_ms        = 60000,
        .keep_alive_enable = false,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
#if CONFIG_APP_OTA_SKIP_CERT_VALIDATION
    // DEV ONLY: disables server cert validation. Do not ship with this!
    http.crt_bundle_attach = NULL;
#endif

    esp_https_ota_config_t cfg = {
        .http_config          = &http,
        .http_client_init_cb  = _http_client_init,
        .partial_http_download = false,
        .max_http_request_size = 4096,
    };

    esp_err_t ret = esp_https_ota(&cfg);
    s_ota_busy = false;

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA success, restarting...");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
    }
}

typedef struct { char url[256]; } ota_now_args_t;
static void ota_now_task(void *arg) {
    ota_now_args_t *a = (ota_now_args_t*)arg;
    do_ota_from_url(a->url);
    vPortFree(a);
    vTaskDelete(NULL);
}

// -------- public API --------
bool ota_is_busy(void) { return s_ota_busy; }

esp_err_t ota_request_now(const char* url) {
    if (!url || !url[0]) return ESP_ERR_INVALID_ARG;
    if (s_ota_busy) {
        ESP_LOGW(TAG, "OTA already in progress; ignoring immediate request");
        return ESP_ERR_INVALID_STATE;
    }
    ota_now_args_t *a = (ota_now_args_t*)pvPortMalloc(sizeof(*a));
    if (!a) return ESP_ERR_NO_MEM;
    strlcpy(a->url, url, sizeof(a->url));
    BaseType_t ok = xTaskCreatePinnedToCore(ota_now_task, "ota_now", 8192, a, 5, NULL, tskNO_AFFINITY);
    if (ok != pdPASS) { vPortFree(a); return ESP_FAIL; }
    return ESP_OK;
}

void ota_schedule_set(int hour, int minute, const char* url) {
    s_hour = hour;
    s_minute = minute;
    if (url) strlcpy(s_url, url, sizeof(s_url)); else s_url[0]='\0';

    nvs_set_i32(s_nvs, "hour",   s_hour);
    nvs_set_i32(s_nvs, "minute", s_minute);
    nvs_set_str(s_nvs, "url",    s_url);
    nvs_commit(s_nvs);

    ESP_LOGI(TAG, "Scheduled OTA set to %02d:%02d -> %s", s_hour, s_minute, s_url);
}

bool ota_schedule_get(int *hour, int *minute, char *url, int url_sz) {
    if (s_hour < 0 || s_minute < 0 || s_url[0] == '\0') return false;
    if (hour)   *hour = s_hour;
    if (minute) *minute = s_minute;
    if (url && url_sz > 0) strlcpy(url, s_url, url_sz);
    return true;
}

void ota_schedule_clear(void) {
    s_hour = s_minute = -1;
    s_url[0] = '\0';
    nvs_erase_key(s_nvs, "hour");
    nvs_erase_key(s_nvs, "minute");
    nvs_erase_key(s_nvs, "url");
    nvs_commit(s_nvs);
    ESP_LOGI(TAG, "Scheduled OTA cleared");
}

// -------- scheduler --------
static void scheduler_task(void *arg) {
    (void)arg;
    int last_day = -1;
    for (;;) {
        int64_t now = time_sync_now_epoch(); // returns epoch seconds or <=0 if unsynced
        if (now > 0 && s_hour >= 0 && s_minute >= 0 && s_url[0]) {
            time_t tnow = (time_t)now;
            struct tm tm_now;
            localtime_r(&tnow, &tm_now);
            if (tm_now.tm_hour == s_hour && tm_now.tm_min == s_minute && tm_now.tm_mday != last_day) {
                if (!s_ota_busy) {
                    last_day = tm_now.tm_mday; // fire once per day
                    ESP_LOGI(TAG, "Scheduled time reached, triggering OTA");
                    // spawn the OTA in its own task
                    ota_request_now(s_url);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(30 * 1000)); // every 30s
    }
}

void ota_init(void) {
    ESP_ERROR_CHECK(nvs_open("ota", NVS_READWRITE, &s_nvs));

    int32_t h=-1, m=-1; size_t len = sizeof(s_url);
    s_url[0]='\0';
    nvs_get_i32(s_nvs, "hour",   &h);
    nvs_get_i32(s_nvs, "minute", &m);
    nvs_get_str(s_nvs, "url",    s_url, &len);
    if (h >= 0 && m >= 0 && s_url[0]) {
        s_hour = h; s_minute = m;
        ESP_LOGI(TAG, "Loaded scheduled OTA: %02d:%02d -> %s", s_hour, s_minute, s_url);
    } else {
        ESP_LOGI(TAG, "No scheduled OTA found in NVS");
    }

    xTaskCreatePinnedToCore(scheduler_task, "ota_sched", 4096, NULL, 4, &s_sched_task, tskNO_AFFINITY);
}
