#include "esp_sntp.h"
#include "esp_log.h"
#include <time.h>


static const char *TAG_TS = "time_sync";
static bool s_synced = false;


static void time_sync_notification_cb(struct timeval *tv) {
s_synced = true;
ESP_LOGI(TAG_TS, "Time synchronized");
}


void time_sync_start(void) {
sntp_setoperatingmode(SNTP_OPMODE_POLL);
sntp_setservername(0, "pool.ntp.org");
sntp_set_time_sync_notification_cb(time_sync_notification_cb);
sntp_init();
}


int64_t time_sync_now_epoch(void) {
if (!s_synced) return 0;
time_t now; time(&now); return (int64_t)now;
}
