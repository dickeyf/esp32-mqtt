#include <string.h>

#include "wifi.h"
#include "settings.h"
#include "nvs_flash.h"
#include "cjson.h"

#define DEFAULT_AP_WIFI_SSID      "pigeon_esp"
#define DEFAULT_AP_WIFI_PASS      "dovecote"

settings_t settings;

void nvs_init() {
  ESP_ERROR_CHECK(nvs_flash_init());
  read_settings();
}


void default_settings() {
  strcpy(settings.ap_password, DEFAULT_AP_WIFI_PASS);
  strcpy(settings.ap_ssid, DEFAULT_AP_WIFI_SSID);
  strcpy(settings.mqtt_uri, "");
  strcpy(settings.mqtt_username, "");
  strcpy(settings.mqtt_password, "");
  strcpy(settings.ssid, "");
  strcpy(settings.password, "");
  strcpy(settings.device_id, "1");
}

esp_err_t read_settings() {
  nvs_handle_t nvs_handle;
  esp_err_t err;

  err = nvs_open(SETTING_NAMESPACE, NVS_READONLY, &nvs_handle);
  if (err != ESP_OK) {
    default_settings();
    return err;
  }

  size_t required_size = sizeof(settings);
  err = nvs_get_blob(nvs_handle, "settings", &settings, &required_size);
  if (err != ESP_OK || required_size != sizeof(settings)) {
    default_settings();
    nvs_close(nvs_handle);
    return err;
  }

  return err;
}

char* get_settings(const char** error_msg) {
  esp_err_t err;

  err = read_settings();
  if (err != ESP_OK) {
    *error_msg = "Failure while reading settings from NVS.";

    return NULL;
  }

  cJSON *settingsResponse = cJSON_CreateObject();

  cJSON_AddItemToObject(settingsResponse, "wifi_ssid", cJSON_CreateString((const char*)settings.ssid));
  cJSON_AddItemToObject(settingsResponse, "wifi_password", cJSON_CreateString((const char*)settings.password));
  cJSON_AddItemToObject(settingsResponse, "ap_ssid", cJSON_CreateString((const char*)settings.ap_ssid));
  cJSON_AddItemToObject(settingsResponse, "ap_password", cJSON_CreateString((const char*)settings.ap_password));
  cJSON_AddItemToObject(settingsResponse, "mqtt_url", cJSON_CreateString((const char*)settings.mqtt_uri));
  cJSON_AddItemToObject(settingsResponse, "mqtt_username", cJSON_CreateString((const char*)settings.mqtt_username));
  cJSON_AddItemToObject(settingsResponse, "mqtt_password", cJSON_CreateString((const char*)settings.mqtt_password));
  cJSON_AddItemToObject(settingsResponse, "device_id", cJSON_CreateString((const char*)settings.device_id));

  char* response = cJSON_PrintUnformatted(settingsResponse);
  cJSON_Delete(settingsResponse);

  return response;
}

esp_err_t post_settings(const char* jsonPayload, const char** error_msg) {
  cJSON* json = cJSON_Parse(jsonPayload);

  if (!json) {
    *error_msg = "Failed to parse request as JSON.";
    return ESP_FAIL;
  }

  nvs_handle_t nvs_handle;

  if (nvs_open(SETTING_NAMESPACE, NVS_READWRITE, &nvs_handle) != ESP_OK) {
    cJSON_Delete(json);
    *error_msg = "Failure while opening NVS.";
    return ESP_FAIL;
  }

  *error_msg = NULL;
  if (!cJSON_HasObjectItem(json, "wifi_ssid")) {
    *error_msg = "The wifi_ssid field is missing from the request.";
  }
  if (!cJSON_HasObjectItem(json, "wifi_password")) {
    *error_msg = "The wifi_password field is missing from the request.";
  }
  if (!cJSON_HasObjectItem(json, "ap_ssid")) {
    *error_msg = "The ap_ssid field is missing from the request.";
  }
  if (!cJSON_HasObjectItem(json, "ap_password")) {
    *error_msg = "The ap_password field is missing from the request.";
  }
  if (!cJSON_HasObjectItem(json, "mqtt_uri")) {
    *error_msg = "The mqtt_uri field is missing from the request.";
  }
  if (!cJSON_HasObjectItem(json, "mqtt_username")) {
    *error_msg = "The mqtt_username field is missing from the request.";
  }
  if (!cJSON_HasObjectItem(json, "mqtt_password")) {
    *error_msg = "The mqtt_password field is missing from the request.";
  }
  if (!cJSON_HasObjectItem(json, "device_id")) {
      *error_msg = "The device_id field is missing from the request.";
    }
  if (*error_msg) {
    nvs_close(nvs_handle);
    cJSON_Delete(json);
    return ESP_ERR_INVALID_ARG;
  }

  strcpy((char*)settings.ssid, cJSON_GetObjectItem(json, "wifi_ssid")->valuestring);
  strcpy((char*)settings.password, cJSON_GetObjectItem(json, "wifi_password")->valuestring);
  strcpy((char*)settings.ap_ssid, cJSON_GetObjectItem(json, "ap_ssid")->valuestring);
  strcpy((char*)settings.ap_password, cJSON_GetObjectItem(json, "ap_password")->valuestring);
  strcpy((char*)settings.mqtt_uri, cJSON_GetObjectItem(json, "mqtt_uri")->valuestring);
  strcpy((char*)settings.mqtt_username, cJSON_GetObjectItem(json, "mqtt_username")->valuestring);
  strcpy((char*)settings.mqtt_password, cJSON_GetObjectItem(json, "mqtt_password")->valuestring);
  strcpy((char*)settings.device_id, cJSON_GetObjectItem(json, "device_id")->valuestring);

  if (nvs_set_blob(nvs_handle, "settings", &settings, sizeof(settings)) != ESP_OK) {
    nvs_close(nvs_handle);
    cJSON_Delete(json);
    *error_msg = "Failure while writing settings to NVS.";
    return ESP_FAIL;
  }

  nvs_close(nvs_handle);
  cJSON_Delete(json);

  on_wifi_settings_changed();

  return ESP_OK;
}

esp_err_t delete_settings(const char** error_msg) {
  nvs_handle_t nvs_handle;
  esp_err_t err;

  err = nvs_open(SETTING_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    *error_msg = "Failure while opening NVS.";
    return err;
  }

  err = nvs_erase_key(nvs_handle, "settings");
  if (err != ESP_OK) {
    *error_msg = "Failure while erasing \"settings\" key.";
    nvs_close(nvs_handle);

    return err;
  }

  nvs_close(nvs_handle);
  return ESP_OK;
}
