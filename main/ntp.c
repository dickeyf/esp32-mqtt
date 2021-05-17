#include "status.h"
#include "esp_sntp.h"
#include "esp_log.h"


static const char *TAG = "TIME";

void time_sync_notification_cb(struct timeval *tv)
{
  if (!is_time_synced()) {
    ESP_LOGI(TAG, "Time synchrronized");
    time_synced();
  }
}

void time_init() {
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
  sntp_init();
}
