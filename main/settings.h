#include <esp_err.h>

#define SETTING_NAMESPACE "setting"

typedef struct {
  char wifi_ssid[32];
  char wifi_password[64];
  char ap_ssid[32];
  char ap_password[64];
  char mqtt_url[256];
  char mqtt_username[32];
  char mqtt_password[32];
  char device_id[32];
  char datacenter_id[32];
} settings_t;

extern settings_t settings;

void nvs_init();
char* get_settings(const char** error_msg);
esp_err_t delete_settings(const char** error_msg);
esp_err_t post_settings(const char* jsonPayload, const char** error_msg);
esp_err_t read_settings();
