#include <time.h>

#include "deps/ds18b20/ds18b20.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "settings.h"
#include "status.h"
#include "mqtt.h"
#include "SensorReadingMessage.h"

#include "cjson.h"

const int TEMP_SENSOR_GPIO = 15; //GPIO where you connected ds18b20

static const char *TAG = "SENSOR_TASK";
static const char *THERMOMETER_SENSOR_TYPE = "thermometer";

void send_temp_sensor_reading_event(float temp, time_t timestamp) {
    struct SensorReading sensorReading = {
            .unit = "Celcius",
            .value2 = 0,
            .sensor_type = THERMOMETER_SENSOR_TYPE,
            .value = temp,
            .timestamp = timestamp
    };

    publish_SensorReadingMessage(
            get_mqtt_client(), THERMOMETER_SENSOR_TYPE, settings.datacenter_id, settings.device_id, &sensorReading);
}

static void temp_sensor_task(void *pvParameters) {
    ds18b20_init(TEMP_SENSOR_GPIO);

    while (1) {
        if (is_mqtt_subscribed() && is_time_synced()) {
            time_t ts;

            time(&ts);
            float temp = ds18b20_get_temp();
            ESP_LOGI(TAG, "[%llu] Temperature: %0.1f\n", ts, temp);

            send_temp_sensor_reading_event(temp, ts);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void init_temp_sensor() {
    xTaskCreatePinnedToCore(&temp_sensor_task, "temp_sensor_task", 4096, NULL, 5, NULL, 0);
}
