#include <time.h>
#include <math.h>
#include <driver/adc.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "settings.h"
#include "status.h"
#include "mqtt.h"

#include "cjson.h"

static const char *TAG = "ADC_SENSOR_TASK";

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
  int lastSecond[100];
  int curSample=0;
  int sampleNum=0;
  float runningAvg=0;

  while (1) {
    vTaskDelay(1);

    if (is_mqtt_subscribed() && is_time_synced()) {
      int cur = adc1_get_raw(ADC1_CHANNEL_0);
      if (sampleNum<100) {
        lastSecond[curSample] = cur;
        sampleNum++;
        runningAvg = (runningAvg * (sampleNum-1) + lastSecond[curSample]) /
                     (float)sampleNum;
      } else {
        float drop = lastSecond[curSample];
        runningAvg = runningAvg + (cur - drop) / 100.0;

        time_t ts;
        time(&ts);
        if (abs(runningAvg - cur) > 200) {
          ESP_LOGI(TAG, "[%lu] %s avg: %.2f  diff: %.2f\n", ts, sensor_type, runningAvg, runningAvg - cur);
        }
      }

      curSample = (curSample +1) % 100;

        //char* temp_evt = create_trigger_sensor_reading_event(
        //      triggered, ts, sensor_type);
        //snprintf(topic, sizeof(topic), "iot/sensors/%s/events/%s/reading",
        //    settings.device_id, sensor_type);
        //mqtt_publish(topic, temp_evt);
        //free(temp_evt);

    }
  }
}

void init_analog_sensor(const char* sensor_type) {
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);
  xTaskCreatePinnedToCore(&tilt_sensor_task, "trigger_sensor_task", 8192, sensor_type, 5, NULL, 0);
}
