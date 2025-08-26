#include "mqtt_ctrl.h"
#include "app_config.h"
#include "effects.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "mqtt";
static esp_mqtt_client_handle_t s_client;
static mqtt_callbacks_t s_cbs;

static char topic_color[128];
static char topic_effect[128];
static char topic_ota[128];
static char topic_status_online[128];
static char topic_status_info[128];

static void make_topics(void) {
    const char *base = app_config_get_mqtt_base();
    snprintf(topic_color, sizeof(topic_color), "%s/cmd/color", base);
    snprintf(topic_effect, sizeof(topic_effect), "%s/cmd/effect", base);
    snprintf(topic_ota, sizeof(topic_ota), "%s/cmd/ota", base);
    snprintf(topic_status_online, sizeof(topic_status_online), "%s/status/online", base);
    snprintf(topic_status_info, sizeof(topic_status_info), "%s/status/info", base);
}

static void publish_online(int val) {
    char buf[2] = { (char)('0' + (val ? 1 : 0)), '\0' };
    esp_mqtt_client_publish(s_client, topic_status_online, buf, 0, 1, true);
}

static const char* effect_to_name(effect_t e) {
    switch (e) {
        case EFFECT_STATIC:          return "static";
        case EFFECT_RAINBOW:         return "rainbow";
        case EFFECT_THEATER_CHASE:   return "theater";
        case EFFECT_FIRE:            return "fire";
        case EFFECT_WAVE:            return "wave";
        case EFFECT_TWINKLE:         return "twinkle";
        case EFFECT_COLOR_WIPE:      return "wipe";
        case EFFECT_STROBE:          return "strobe";
        case EFFECT_BREATHING:       return "breathing";
        case EFFECT_COMET:           return "comet";
        case EFFECT_SPARKLE:         return "sparkle";
        case EFFECT_SPACEY_GRADIENT: return "spacey";
        default:                     return "unknown";
    }
}

static effect_t effect_from_name(const char *n) {
    if (!n) return EFFECT_STATIC;
    if (!strcmp(n, "static"))   return EFFECT_STATIC;
    if (!strcmp(n, "rainbow"))  return EFFECT_RAINBOW;
    if (!strcmp(n, "theater"))  return EFFECT_THEATER_CHASE;
    if (!strcmp(n, "fire"))     return EFFECT_FIRE;
    if (!strcmp(n, "wave"))     return EFFECT_WAVE;
    if (!strcmp(n, "twinkle"))  return EFFECT_TWINKLE;
    if (!strcmp(n, "wipe"))     return EFFECT_COLOR_WIPE;
    if (!strcmp(n, "strobe"))   return EFFECT_STROBE;
    if (!strcmp(n, "breathing"))return EFFECT_BREATHING;
    if (!strcmp(n, "comet"))    return EFFECT_COMET;
    if (!strcmp(n, "sparkle"))  return EFFECT_SPARKLE;
    if (!strcmp(n, "spacey"))   return EFFECT_SPACEY_GRADIENT;
    return EFFECT_STATIC;
}

static void handle_color_json(const char *data, int len) {
    cJSON *root = cJSON_ParseWithLength(data, len);
    if (!root) return;
    cJSON *jr = cJSON_GetObjectItem(root, "r");
    cJSON *jg = cJSON_GetObjectItem(root, "g");
    cJSON *jb = cJSON_GetObjectItem(root, "b");
    int r = 0, g = 0, b = 0;
    if (cJSON_IsNumber(jr)) r = jr->valueint;
    if (cJSON_IsNumber(jg)) g = jg->valueint;
    if (cJSON_IsNumber(jb)) b = jb->valueint;
    effects_set_static((rgb_t){(uint8_t)r, (uint8_t)g, (uint8_t)b});
    cJSON_Delete(root);
}

static void handle_effect_json(const char *data, int len) {
    cJSON *root = cJSON_ParseWithLength(data, len);
    if (!root) return;
    cJSON *jn = cJSON_GetObjectItem(root, "name");
    if (cJSON_IsString(jn)) effects_set_effect(effect_from_name(jn->valuestring));
    cJSON_Delete(root);
}

static void handle_ota_json(const char *data, int len) {
    cJSON *root = cJSON_ParseWithLength(data, len);
    if (!root) return;
    cJSON *now = cJSON_GetObjectItem(root, "now");
    if (cJSON_IsString(now) && s_cbs.on_ota_now) s_cbs.on_ota_now(now->valuestring);
    cJSON *sch = cJSON_GetObjectItem(root, "schedule");
    if (cJSON_IsObject(sch) && s_cbs.on_ota_schedule_update) {
        cJSON *hour = cJSON_GetObjectItem(sch, "hour");
        cJSON *min  = cJSON_GetObjectItem(sch, "minute");
        cJSON *url  = cJSON_GetObjectItem(sch, "url");
        if (cJSON_IsNumber(hour) && cJSON_IsNumber(min) && cJSON_IsString(url)) {
            s_cbs.on_ota_schedule_update(hour->valueint, min->valueint, url->valuestring);
        }
    }
    cJSON_Delete(root);
}

static void mqtt_event_handler(void *args, esp_event_base_t base, int32_t id, void *event_data) {
    esp_mqtt_event_handle_t e = (esp_mqtt_event_handle_t)event_data;
    switch (id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            publish_online(1);
            esp_mqtt_client_subscribe(s_client, topic_color, 1);
            esp_mqtt_client_subscribe(s_client, topic_effect, 1);
            esp_mqtt_client_subscribe(s_client, topic_ota, 1);
            mqtt_ctrl_publish_info();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            publish_online(0);
            break;
        case MQTT_EVENT_DATA: {
            char topic[128] = {0};
            char msg[512] = {0};
            snprintf(topic, e->topic_len + 1, "%.*s", e->topic_len, e->topic);
            snprintf(msg, e->data_len + 1, "%.*s", e->data_len, e->data);
            if (!strcmp(topic, topic_color))      handle_color_json(msg, e->data_len);
            else if (!strcmp(topic, topic_effect)) handle_effect_json(msg, e->data_len);
            else if (!strcmp(topic, topic_ota))    handle_ota_json(msg, e->data_len);
            break;
        }
        default:
            break;
    }
}

esp_err_t mqtt_ctrl_start(const mqtt_callbacks_t *cbs) {
    if (!cbs) return ESP_ERR_INVALID_ARG;
    s_cbs = *cbs;
    make_topics();

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = app_config_get_mqtt_url(),
    };
    s_client = esp_mqtt_client_init(&cfg);
    if (!s_client) return ESP_FAIL;
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    return esp_mqtt_client_start(s_client);
}

void mqtt_ctrl_publish_info(void) {
    if (!s_client) return;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "firmware", APP_FIRMWARE_NAME "-" APP_FIRMWARE_VERSION);
    cJSON_AddNumberToObject(root, "led_count", app_config_get_led_count());
    cJSON_AddStringToObject(root, "effect", effect_to_name(effects_get_effect()));
    char *payload = cJSON_PrintUnformatted(root);
    esp_mqtt_client_publish(s_client, topic_status_info, payload, 0, 1, true);
    cJSON_free(payload);
    cJSON_Delete(root);
}

