#include <string.h>
#include <stdlib.h>

#include "cjson.h"
#include "status.h"
#include "settings.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "mqtt.h"
#include "esp_netif.h"

#define INIT_AP_WIFI_CHANNEL   4
#define INIT_AP_MAX_STA_CONN   2

static const char *TAG = "rest_if";
esp_netif_ip_info_t sta_ip_info;

static const char* get_auth_mode(wifi_auth_mode_t authmode) {
  switch (authmode) {
    case WIFI_AUTH_OPEN:
      return "WIFI_AUTH_OPEN";
    case WIFI_AUTH_WEP:
      return "WIFI_AUTH_WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WIFI_AUTH_WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
      return "WIFI_AUTH_WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WIFI_AUTH_WPA_WPA2_PSK";
    default:
      break;
  }

  return "Unknown";
}

static const char* get_cipher_name(wifi_cipher_type_t cipherType) {
  switch (cipherType) {
    case WIFI_CIPHER_TYPE_NONE:
      return "WIFI_CIPHER_TYPE_NONE";
    case WIFI_CIPHER_TYPE_WEP40:
      return "WIFI_CIPHER_TYPE_WEP40";
    case WIFI_CIPHER_TYPE_WEP104:
      return "WIFI_CIPHER_TYPE_WEP104";
    case WIFI_CIPHER_TYPE_TKIP:
      return "WIFI_CIPHER_TYPE_TKIP";
    case WIFI_CIPHER_TYPE_CCMP:
      return "WIFI_CIPHER_TYPE_CCMP";
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
      return "WIFI_CIPHER_TYPE_TKIP_CCMP";
    case WIFI_CIPHER_TYPE_AES_CMAC128:
      return "WIFI_CIPHER_TYPE_AES_CMAC128";
    default:
      break;
  }

  return "WIFI_CIPHER_TYPE_UNKNOWN";
}

void initialise_wifi_ap() {
  wifi_config_t wifi_config = { .ap = { .max_connection = INIT_AP_MAX_STA_CONN,
      .channel = INIT_AP_WIFI_CHANNEL,
      .authmode = WIFI_AUTH_WPA_WPA2_PSK}};

  strcpy((char*)wifi_config.ap.ssid, settings.ap_ssid);
  wifi_config.ap.ssid_len = strlen(settings.ap_ssid);
  strcpy((char*)wifi_config.ap.password, settings.ap_password);


  if (strlen(settings.ap_password) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  if (strlen(settings.wifi_ssid) != 0) {
    strcpy((char*)wifi_config.sta.ssid, settings.wifi_ssid);
    strcpy((char*)wifi_config.sta.password, settings.wifi_password);
    wifi_config.sta.bssid_set = 0;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_LOGI(TAG, "wifi station config set.  SSID:%s password:%s",
        settings.wifi_ssid, settings.wifi_password);
  }

  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_connect();

  ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
      settings.ap_ssid, settings.ap_password, INIT_AP_WIFI_CHANNEL);
}

char* get_wifi_networks() {
  wifi_scan_config_t scan_config = {
      .ssid = NULL,
      .bssid = NULL,
      .channel = 0,
      .show_hidden = 1,
      .scan_type = WIFI_SCAN_TYPE_ACTIVE,
      .scan_time = { .active = { .min = 0, .max = 0 }, .passive = 120 } };

  esp_wifi_scan_start(&scan_config, true);

  uint16_t ap_count = 0;
  wifi_ap_record_t *ap_list = NULL;

  esp_wifi_scan_get_ap_num(&ap_count);

  cJSON *wifiResponse = cJSON_CreateObject();

  cJSON_AddItemToObject(wifiResponse, "ap_count", cJSON_CreateNumber(ap_count));

  cJSON *wifiList = cJSON_CreateArray();
  cJSON_AddItemToObject(wifiResponse, "wifi_list", wifiList);

  if (ap_count > 0) {
    ap_list = (wifi_ap_record_t*) malloc(ap_count * sizeof(wifi_ap_record_t));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));
  }

  for (uint8_t i = 0; i < ap_count; i++) {
    cJSON *wifi_ap = cJSON_CreateObject();
    cJSON_AddItemToObject(wifiList, "wifi_ap", wifi_ap);

    char *ht_mode;
    switch (ap_list[i].second) {
      case WIFI_SECOND_CHAN_NONE:
        ht_mode = "HT20";
        break;
      case WIFI_SECOND_CHAN_ABOVE:
        ht_mode = "HT40+";
        break;
      case WIFI_SECOND_CHAN_BELOW:
        ht_mode = "HT40-";
        break;
      default:
        ht_mode = "UNKNOWN";
        break;

    }

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
        ap_list[i].bssid[0], ap_list[i].bssid[1], ap_list[i].bssid[2],
        ap_list[i].bssid[3], ap_list[i].bssid[4], ap_list[i].bssid[5]);

    cJSON_AddItemToObject(wifi_ap, "authmode",
        cJSON_CreateString(get_auth_mode(ap_list[i].authmode)));
    cJSON_AddItemToObject(wifi_ap, "ssid", cJSON_CreateString((char*)ap_list[i].ssid));
    cJSON_AddItemToObject(wifi_ap, "bssid", cJSON_CreateString(macStr));
    cJSON_AddItemToObject(wifi_ap, "rssi", cJSON_CreateNumber(ap_list[i].rssi));
    cJSON_AddItemToObject(wifi_ap, "channel",
        cJSON_CreateNumber(ap_list[i].primary));
    cJSON_AddItemToObject(wifi_ap, "ht_mode", cJSON_CreateString(ht_mode));
    cJSON_AddItemToObject(wifi_ap, "pairwise_cipher",
        cJSON_CreateString(get_cipher_name(ap_list[i].pairwise_cipher)));
    cJSON_AddItemToObject(wifi_ap, "group_cipher",
        cJSON_CreateString(get_cipher_name(ap_list[i].group_cipher)));
    cJSON_AddItemToObject(wifi_ap, "11b",
        cJSON_CreateString(ap_list[i].phy_11b ? "true" : "false"));
    cJSON_AddItemToObject(wifi_ap, "11g",
        cJSON_CreateString(ap_list[i].phy_11g ? "true" : "false"));
    cJSON_AddItemToObject(wifi_ap, "11n",
        cJSON_CreateString(ap_list[i].phy_11n ? "true" : "false"));
    cJSON_AddItemToObject(wifi_ap, "lr",
        cJSON_CreateString(ap_list[i].phy_lr ? "true" : "false"));
    cJSON_AddItemToObject(wifi_ap, "wps",
        cJSON_CreateString(ap_list[i].wps ? "true" : "false"));
    cJSON *countryRestrictions = cJSON_CreateObject();
    cJSON_AddItemToObject(wifi_ap, "country_restrictions", countryRestrictions);
    ap_list[i].country.cc[2] = 0;
    cJSON_AddItemToObject(countryRestrictions, "country_code",
        cJSON_CreateString(ap_list[i].country.cc));
    cJSON_AddItemToObject(countryRestrictions, "start_channel",
        cJSON_CreateNumber(ap_list[i].country.schan));
    cJSON_AddItemToObject(countryRestrictions, "num_of_channel",
        cJSON_CreateNumber(ap_list[i].country.nchan));
    cJSON_AddItemToObject(countryRestrictions, "max_tx_pwr",
        cJSON_CreateNumber(ap_list[i].country.max_tx_power));
    cJSON_AddItemToObject(countryRestrictions, "policy",
        cJSON_CreateString(
            ap_list[i].country.policy == WIFI_COUNTRY_POLICY_AUTO ?
                "AUTO" : "MANUAL"));
  }
  if (ap_list != NULL) {
    free(ap_list);
  }

  char* response = cJSON_PrintUnformatted(wifiResponse);

  cJSON_Delete(wifiResponse);

  return response;
}

void on_wifi_settings_changed() {
  esp_wifi_disconnect();
  initialise_wifi_ap();
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
      wifi_event_ap_staconnected_t *event =
          (wifi_event_ap_staconnected_t*) event_data;
      ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac),
          event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
      wifi_event_ap_stadisconnected_t *event =
          (wifi_event_ap_stadisconnected_t*) event_data;
      ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac),
          event->aid);
    } else if (event_id == WIFI_EVENT_STA_START) {
      ESP_LOGI(TAG, "WIFI STATION START");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
      ESP_LOGI(TAG, "WIFI STATION DISCONNECTED");
      station_disconnected();
      if (strlen(settings.wifi_ssid)!=0) {
        esp_wifi_connect();
      }
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
      ESP_LOGI(TAG, "WIFI STATION CONNECTED");
      station_connected();
    }
  } else if (event_base == IP_EVENT) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
      sta_ip_info = event->ip_info;
      ESP_LOGI(TAG, "WIFI Station got IP:" IPSTR, IP2STR(&event->ip_info.ip));
      if (strlen(settings.mqtt_url)!=0) {
        mqtt_start();
      }
      ip_acquired();
    } else if (event_id == IP_EVENT_STA_LOST_IP) {
      ESP_LOGI(TAG, "WIFI Station lost IP.");
      ip_lost();
    }
  }
}

void wifi_create_ap() {
  esp_netif_init();
  esp_netif_t* wifi_ap_netif = esp_netif_create_default_wifi_ap();
  ESP_ERROR_CHECK(esp_netif_dhcps_stop(wifi_ap_netif));

  //Configure the AP's netif to default IP.  But most importantly disable the router solicitation address to stop
  //from making the ESP's a default gateway for clients that joins.  Also disable DNS advertisement.
  esp_netif_ip_info_t info;
  memset(&info, 0, sizeof(info));
  esp_netif_set_ip4_addr(&info.ip, 192, 168, 4, 1);
  esp_netif_set_ip4_addr(&info.netmask, 255, 255, 255, 0);
  esp_netif_set_ip4_addr(&info.gw, 192, 168, 4, 1);
  uint8_t disable_val = 0;
  esp_netif_dhcps_option(
          wifi_ap_netif,
          ESP_NETIF_OP_SET,
          ESP_NETIF_ROUTER_SOLICITATION_ADDRESS,
          &disable_val,
          sizeof(disable_val));
  esp_netif_dhcps_option(
          wifi_ap_netif,
          ESP_NETIF_OP_SET,
          ESP_NETIF_DOMAIN_NAME_SERVER,
          &disable_val,
          sizeof(disable_val));
  ESP_ERROR_CHECK(esp_netif_set_ip_info(wifi_ap_netif, &info));
  ESP_ERROR_CHECK(esp_netif_dhcps_start(wifi_ap_netif));
  esp_wifi_set_default_wifi_ap_handlers();
}

void wifi_init() {
  wifi_create_ap();
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(
      esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
  ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL) );

  initialise_wifi_ap();
}
