#include <time.h>

#include "esp_log.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "settings.h"
#include "status.h"
#include "mqtt.h"

#include "cjson.h"


const int TILT_SENSOR_GPIO = 36; //GPIO where you connected tilt_sensor

static const char *TAG = "SENSOR_TASK";

char* create_tilt_sensor_reading_event(int tilted, time_t timestamp) {
  cJSON *sensor_reading_event = cJSON_CreateObject();

  cJSON_AddItemToObject(sensor_reading_event, "sensor_type",
          cJSON_CreateString("tilt"));

  cJSON_AddItemToObject(sensor_reading_event, "unit",
          cJSON_CreateString("boolean"));

  cJSON_AddItemToObject(sensor_reading_event, "reading",
          cJSON_CreateBool(tilted));

  cJSON_AddItemToObject(sensor_reading_event, "timestamp",
            cJSON_CreateNumber(timestamp));

  char* event_out = cJSON_PrintUnformatted(sensor_reading_event);
  cJSON_Delete(sensor_reading_event);
  return event_out;
}

static void tilt_sensor_task(void *pvParameters) {
  char topic[256];
  gpio_pad_select_gpio(TILT_SENSOR_GPIO);
  gpio_set_direction(TILT_SENSOR_GPIO, GPIO_MODE_INPUT);
  int tilted = -1;

  while (1) {
    vTaskDelay(250 / portTICK_RATE_MS);

    if (is_mqtt_subscribed() && is_time_synced()) {
      int curLevel = gpio_get_level(TILT_SENSOR_GPIO);
      if(curLevel!=tilted) {
        tilted = curLevel;

        time_t ts;
        time(&ts);
        ESP_LOGI(TAG, "[%lu] Tilted: %s\n", ts, tilted ? "true":"false");
        char* temp_evt = create_tilt_sensor_reading_event(tilted, ts);
        snprintf(topic, sizeof(topic), "iot/sensors/%s/events/tilt/reading",
            settings.device_id);
        mqtt_publish(topic, temp_evt);
        free(temp_evt);
      }
    }
  }
}

void init_tilt_sensor() {
  xTaskCreatePinnedToCore(&tilt_sensor_task, "tilt_sensor_task", 4096, NULL, 5, NULL, 0);
}
