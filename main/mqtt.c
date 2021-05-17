#include "mqtt_client.h"
#include "esp_log.h"
#include "settings.h"
#include "status.h"


static const char *TAG = "MQTT_CLIENT";
extern const uint8_t mqtt_eclipse_org_pem_start[] asm("_binary_trust_store_cer_start");
extern const uint8_t mqtt_eclipse_org_pem_end[] asm("_binary_trust_store_cer_pem_end");


esp_mqtt_client_handle_t mqtt_client;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
  char topic[256];

  esp_mqtt_client_handle_t client = event->client;
  int msg_id;
  // your_context_t *context = event->context;
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
      snprintf(topic, sizeof(topic), "iot/sensors/%s/#", settings.device_id);
      msg_id = esp_mqtt_client_subscribe(client, topic, 0);
      ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
      mqtt_connected();
      break;
    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
      //esp_mqtt_client_reconnect(client);
      mqtt_disconnected();
      break;
    case MQTT_EVENT_SUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
      mqtt_subscribed();
      char* data_p = get_health();
      snprintf(topic, sizeof(topic), "iot/sensors/%s/events/heartbeat", settings.device_id);
      msg_id = esp_mqtt_client_publish(client, topic,
          data_p, 0, 0, 0);
      free(data_p);
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
      mqtt_unsubscribed();
      break;
    case MQTT_EVENT_PUBLISHED:
      ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG, "MQTT_EVENT_DATA");
      printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
      printf("DATA=%.*s\r\n", event->data_len, event->data);
      break;
    case MQTT_EVENT_ERROR:
      ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
      if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
        ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x",
            event->error_handle->esp_tls_last_esp_err);
        ESP_LOGI(TAG, "Last tls stack error number: 0x%x",
            event->error_handle->esp_tls_stack_err);
      } else if (event->error_handle->error_type
          == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
        ESP_LOGI(TAG, "Connection refused error: 0x%x",
            event->error_handle->connect_return_code);
      } else {
        ESP_LOGW(TAG, "Unknown error type: 0x%x",
            event->error_handle->error_type);
      }
      break;
    default:
      ESP_LOGI(TAG, "Other event id:%d", event->event_id);
      break;
  }
  return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
    int32_t event_id, void *event_data) {
  ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
      event_id);
  mqtt_event_handler_cb(event_data);
}

int mqtt_publish(char* topic, const char* payload) {
    return esp_mqtt_client_publish(mqtt_client, topic,
        payload, 0, 0, 0);
}

void mqtt_start(void) {
  const esp_mqtt_client_config_t mqtt_cfg = { .uri =
      settings.mqtt_uri, .username = settings.mqtt_username, .password =
      settings.mqtt_password, .cert_pem = (const char*) mqtt_eclipse_org_pem_start, };

  ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
  if (mqtt_client!=NULL) {
    esp_mqtt_client_destroy(mqtt_client);
  }
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler,
      mqtt_client);
  esp_mqtt_client_start(mqtt_client);
}
