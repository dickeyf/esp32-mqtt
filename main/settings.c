#include <string.h>

#include "wifi.h"
#include "settings.h"
#include "nvs_flash.h"
#include "cjson.h"

#include "SensorSettingsSchema.h"

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
    strcpy(settings.mqtt_url, "");
    strcpy(settings.mqtt_username, "");
    strcpy(settings.mqtt_password, "");
    strcpy(settings.wifi_ssid, "");
    strcpy(settings.wifi_password, "");
    strcpy(settings.device_id, "1");
    strcpy(settings.datacenter_id, "default");
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

char *get_settings(const char **error_msg) {
    esp_err_t err;

    err = read_settings();
    if (err != ESP_OK) {
        *error_msg = "Failure while reading settings from NVS.";

        return NULL;
    }

    struct SensorSettings sensorSettings = {
            .jsonObj = NULL,
            .ap_ssid = settings.ap_ssid,
            .wifi_ssid = settings.wifi_ssid,
            .ap_password = settings.ap_password,
            .device_id = settings.device_id,
            .mqtt_url = settings.mqtt_url,
            .wifi_password = settings.wifi_password,
            .mqtt_username = settings.mqtt_username,
            .mqtt_password = settings.mqtt_password,
            .datacenter_id = settings.datacenter_id
    };
    cJSON *settingsResponse = create_SensorSettingsSchema(&sensorSettings);

    char *response = cJSON_PrintUnformatted(settingsResponse);
    cJSON_Delete(settingsResponse);

    return response;
}

esp_err_t post_settings(const char *jsonPayload, const char **error_msg) {
    struct SensorSettings *sensorSettings;

    esp_err_t result =
        read_SensorSettingsSchema(jsonPayload, &sensorSettings, error_msg);
    if (result != ESP_OK) {
        return result;
    }

    nvs_handle_t nvs_handle;
    if (nvs_open(SETTING_NAMESPACE, NVS_READWRITE, &nvs_handle) != ESP_OK) {
        free_SensorSettingsSchema(sensorSettings);
        *error_msg = "Failure while opening NVS.";
        return ESP_FAIL;
    }

    strcpy((char *) settings.wifi_ssid, sensorSettings->wifi_ssid);
    strcpy((char *) settings.wifi_password, sensorSettings->wifi_password);
    strcpy((char *) settings.ap_ssid, sensorSettings->ap_ssid);
    strcpy((char *) settings.ap_password, sensorSettings->ap_password);
    strcpy((char *) settings.mqtt_url, sensorSettings->mqtt_url);
    strcpy((char *) settings.mqtt_username, sensorSettings->mqtt_username);
    strcpy((char *) settings.mqtt_password, sensorSettings->mqtt_password);
    strcpy((char *) settings.device_id, sensorSettings->device_id);
    strcpy((char *) settings.datacenter_id, sensorSettings->datacenter_id);

    if (nvs_set_blob(nvs_handle, "settings", &settings, sizeof(settings)) != ESP_OK) {
        free_SensorSettingsSchema(sensorSettings);
        nvs_close(nvs_handle);
        *error_msg = "Failure while writing settings to NVS.";
        return ESP_FAIL;
    }

    nvs_close(nvs_handle);
    free_SensorSettingsSchema(sensorSettings);

    on_wifi_settings_changed();

    return ESP_OK;
}

esp_err_t delete_settings(const char **error_msg) {
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
