#include <time.h>
#include <math.h>
#include <driver/adc.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "settings.h"
#include "status.h"
#include "mqtt.h"
#include "SensorReadingMessage.h"

#include "cjson.h"

static const char *TAG = "ADC_SENSOR_TASK";

void send_analog_sensor_reading_event(
        int reading_value, time_t timestamp, const char *sensor_type, const char *unit) {
    struct SensorReading sensorReading = {
            .jsonObj = NULL,
            .unit = unit,
            .value2 = 0,
            .sensor_type = sensor_type,
            .value = reading_value,
            .timestamp = timestamp
    };

    publish_SensorReadingMessage(
            get_mqtt_client(), sensor_type, settings.datacenter_id, settings.device_id, &sensorReading);
}

static void tilt_sensor_task(void *pvParameters) {
    const char *sensor_type = (const char *) pvParameters;

    while (1) {
        vTaskDelay(1);

        if (is_mqtt_subscribed() && is_time_synced()) {
            time_t ts;
            time(&ts);
            int reading = adc1_get_raw(ADC1_CHANNEL_0);
            ESP_LOGI(TAG, "[%lu] ADC %s: %d / 4096\n", ts, sensor_type, reading);

            send_analog_sensor_reading_event(
                  reading, ts, sensor_type, "adc-4k-levels");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void init_analog_sensor(const char *sensor_type) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    xTaskCreatePinnedToCore(&tilt_sensor_task, "adc_sensor_task", 8192, (void *const) sensor_type, 5, NULL, 0);
}
