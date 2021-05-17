#include <time.h>

#include "deps/ds18b20/ds18b20.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "settings.h"
#include "status.h"
#include "mqtt.h"

#include "cjson.h"


const int TEMP_SENSOR_GPIO = 15; //GPIO where you connected ds18b20

static const char *TAG = "SENSOR_TASK";

char* create_temp_sensor_reading_event(float temp, time_t timestamp) {
  cJSON *sensor_reading_event = cJSON_CreateObject();

  cJSON_AddItemToObject(sensor_reading_event, "sensor_type",
          cJSON_CreateString("temperature"));

  cJSON_AddItemToObject(sensor_reading_event, "unit",
          cJSON_CreateString("celcius"));

  cJSON_AddItemToObject(sensor_reading_event, "reading",
          cJSON_CreateNumber((double)temp));

  cJSON_AddItemToObject(sensor_reading_event, "timestamp",
            cJSON_CreateNumber(timestamp));

  char* event_out = cJSON_PrintUnformatted(sensor_reading_event);
  cJSON_Delete(sensor_reading_event);
  return event_out;
}

static void temp_sensor_task(void *pvParameters) {
  char topic[256];
  ds18b20_init(TEMP_SENSOR_GPIO);

  while (1) {
    time_t ts;

    time(&ts);
    float temp = ds18b20_get_temp();
    ESP_LOGI(TAG, "[%lu] Temperature: %0.1f\n", ts, temp);

    if (is_mqtt_subscribed() && is_time_synced()) {
      char* temp_evt = create_temp_sensor_reading_event(temp, ts);
      snprintf(topic, sizeof(topic), "iot/sensors/%s/events/temperature/reading",
          settings.device_id);
      mqtt_publish(topic, temp_evt);
      free(temp_evt);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void init_temp_sensor() {
  xTaskCreatePinnedToCore(&temp_sensor_task, "temp_sensor_task", 4096, NULL, 5, NULL, 0);
}
