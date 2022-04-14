#include <time.h>

#include "driver/gpio.h"
#include "soc/gpio_reg.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "mqtt.h"
#include "settings.h"
#include "status.h"
#include "SensorReadingMessage.h"
#include "rom/ets_sys.h"

const int DIST_SENSOR_PING_GPIO = 15; //GPIO where you connected trigger pin
const int DIST_SENSOR_PONG_GPIO = 36; //GPIO where you connected echo pin

static uint64_t lastRisingEdge = -1;
static const char *TAG = "DISTANCE_TASK";
portMUX_TYPE distance_sensor_mux = portMUX_INITIALIZER_UNLOCKED;
static const char *DISTANCE_SENSOR_TYPE = "distance";

void send_distance_event(int distanceCM, time_t timestamp) {
    struct SensorReading sensorReading = {
            .jsonObj = NULL,
            .unit = "centimeter",
            .value2 = 0,
            .sensor_type = DISTANCE_SENSOR_TYPE,
            .value = distanceCM,
            .timestamp = timestamp
    };

    publish_SensorReadingMessage(
            get_mqtt_client(), DISTANCE_SENSOR_TYPE, settings.datacenter_id, settings.device_id, &sensorReading);
}


static void distPongIrqCallback( void *arg ) {
    QueueHandle_t dist_sensor_queue = (QueueHandle_t)arg;

    uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);   //read status to get interrupt status for GPIO0-31
    uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);//read status1 to get interrupt status for GPIO32-39
    //Clear
    SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);    //Clear intr for gpio0-gpio31
    SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h); //Clear intr for gpio32-39

    if(gpio_intr_status_h & BIT(DIST_SENSOR_PONG_GPIO - 32)) {
        uint64_t irq_ts = esp_timer_get_time();
        int gpio_level = gpio_get_level(DIST_SENSOR_PONG_GPIO);
        if (gpio_level != 0) {
            // Get time when the ECHO signal starts
            lastRisingEdge = irq_ts;
        } else {
            if (lastRisingEdge != -1) {
                //Compute who long the ECHO signal have been raised.
                uint32_t timeraised = (uint32_t)(irq_ts - lastRisingEdge);

                xQueueSendToBackFromISR(dist_sensor_queue, &timeraised, NULL);

                lastRisingEdge = -1;
            } else {
                ets_printf("Ignore falling edge as no rising edge reading was available");
            }
        }
    }
}


static void send_ping() {
    portENTER_CRITICAL(&distance_sensor_mux);
    gpio_set_level(DIST_SENSOR_PING_GPIO,1);
    esp_rom_delay_us(10);
    gpio_set_level(DIST_SENSOR_PING_GPIO,0);
    portEXIT_CRITICAL(&distance_sensor_mux);
}

static void distance_sensor_task(void *pvParameters) {
    QueueHandle_t dist_sensor_queue = xQueueCreate( 10, 4);
    gpio_reset_pin(DIST_SENSOR_PING_GPIO);
    gpio_set_direction(DIST_SENSOR_PING_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(DIST_SENSOR_PONG_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DIST_SENSOR_PONG_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(DIST_SENSOR_PONG_GPIO, GPIO_INTR_ANYEDGE);

    ESP_LOGI(TAG, "Enabling interrupt for pin %d.", DIST_SENSOR_PONG_GPIO);
    if (gpio_intr_enable(DIST_SENSOR_PONG_GPIO)!=ESP_OK) {
        ESP_LOGE(TAG, "Failed enabling interrupt for pin.");
    }

    if (gpio_isr_register(distPongIrqCallback, (void *)dist_sensor_queue, 0, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "Failed registering interrupts for GPIO.");
    }


    while (1) {
        send_ping();
        int echoDuration;
        int distanceCM = -1;
        if(xQueueReceive(dist_sensor_queue, (void * )&echoDuration, (TickType_t)(1000 / portTICK_PERIOD_MS))) {
            distanceCM = echoDuration/58;

            ESP_LOGI(TAG, "Measured a distance of %d centimeters.", distanceCM);
        } else {
            ESP_LOGI(TAG, "Didn't receive an echo to the ping.");
        }

        if (is_mqtt_subscribed() && is_time_synced() && distanceCM != -1) {
            time_t ts;
            time(&ts);

            send_distance_event(distanceCM, ts);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void init_distance_sensor() {
    xTaskCreatePinnedToCore(&distance_sensor_task, "distance_sensor_task", 4096, NULL, 5, NULL, 0);
}