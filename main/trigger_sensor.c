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

char* create_trigger_sensor_reading_event(
    int triggered, time_t timestamp, const char* sensor_type) {
  cJSON *sensor_reading_event = cJSON_CreateObject();

  cJSON_AddItemToObject(sensor_reading_event, "sensor_type",
          cJSON_CreateString(sensor_type));

  cJSON_AddItemToObject(sensor_reading_event, "unit",
          cJSON_CreateString("boolean"));

  cJSON_AddItemToObject(sensor_reading_event, "reading",
          cJSON_CreateBool(triggered));

  cJSON_AddItemToObject(sensor_reading_event, "timestamp",
            cJSON_CreateNumber(timestamp));

  char* event_out = cJSON_PrintUnformatted(sensor_reading_event);
  cJSON_Delete(sensor_reading_event);
  return event_out;
}

static void tilt_sensor_task(void *pvParameters) {
  const char* sensor_type = (const char*) pvParameters;

  char topic[256];
  gpio_pad_select_gpio(TILT_SENSOR_GPIO);
  gpio_set_direction(TILT_SENSOR_GPIO, GPIO_MODE_INPUT);
  int triggered = -1;

  while (1) {
    vTaskDelay(250 / portTICK_RATE_MS);

    if (is_mqtt_subscribed() && is_time_synced()) {
      int curLevel = gpio_get_level(TILT_SENSOR_GPIO);
      if(curLevel!=triggered) {
        triggered = curLevel;

        time_t ts;
        time(&ts);
        ESP_LOGI(TAG, "[%lu] %s sensor triggered: %s\n", ts, sensor_type, triggered ? "true":"false");
        char* temp_evt = create_trigger_sensor_reading_event(
              triggered, ts, sensor_type);
        snprintf(topic, sizeof(topic), "iot/sensors/%s/events/%s/reading",
            settings.device_id, sensor_type);
        mqtt_publish(topic, temp_evt);
        free(temp_evt);
      }
    }
  }
}

void init_trigger_sensor(const char* sensor_type) {
  xTaskCreatePinnedToCore(&tilt_sensor_task, "trigger_sensor_task", 4096, sensor_type, 5, NULL, 0);
}
