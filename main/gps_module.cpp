#include <time.h>
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "settings.h"
#include "status.h"
#include "mqtt.h"
#include "deps/tinygps/tinygps.h"

#include "cjson.h"


#define BUF_SIZE (256)
#define RD_BUF_SIZE (BUF_SIZE)

static const char *TAG = "GPS_TASK";
TinyGPSPlus gps;

char* create_gps_position_event(double lat, double lng, time_t timestamp) {
  cJSON *gps_position_event = cJSON_CreateObject();

  cJSON_AddItemToObject(gps_position_event, "sensor_type",
          cJSON_CreateString("GPS"));

  cJSON_AddItemToObject(gps_position_event, "unit",
          cJSON_CreateString("degrees"));

  cJSON_AddItemToObject(gps_position_event, "latitude",
          cJSON_CreateNumber(lat));

  cJSON_AddItemToObject(gps_position_event, "longitude",
          cJSON_CreateNumber(lng));

  cJSON_AddItemToObject(gps_position_event, "timestamp",
            cJSON_CreateNumber(timestamp));

  char* event_out = cJSON_PrintUnformatted(gps_position_event);
  cJSON_Delete(gps_position_event);
  return event_out;
}

static void service_uart_queue(uart_port_t uart_num, QueueHandle_t uart_queue) {
  uint8_t dtmp[RD_BUF_SIZE];
  uart_event_t event;

  //Waiting for UART event.
  if(xQueueReceive(uart_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
    bzero(dtmp, RD_BUF_SIZE);
    switch(event.type) {
    case UART_DATA:
        uart_read_bytes(uart_num, dtmp, event.size, portMAX_DELAY);
        for (int i=0; i<event.size; i++) {
          gps.encode(dtmp[i]);
        }
        break;
    case UART_FIFO_OVF:
        ESP_LOGI(TAG, "hw fifo overflow");
        // Directly flush the rx buffer here in order to read more data.
        uart_flush_input(uart_num);
        xQueueReset(uart_queue);
        break;
    case UART_BUFFER_FULL:
        ESP_LOGI(TAG, "ring buffer full");
        // If buffer full happened, you should consider encreasing your buffer size
        // As an example, we directly flush the rx buffer here in order to read more data.
        uart_flush_input(uart_num);
        xQueueReset(uart_queue);
        break;
    case UART_BREAK:
        ESP_LOGI(TAG, "uart rx break");
        break;
    case UART_PARITY_ERR:
        ESP_LOGI(TAG, "uart parity error");
        break;
    case UART_FRAME_ERR:
        ESP_LOGI(TAG, "uart frame error");
        break;
    //Others
    default:
        ESP_LOGI(TAG, "uart event type: %d", event.type);
        break;
    }
  }
}

static void gps_task(void *pvParameters) {
  QueueHandle_t uart_queue;

  char topic[256];

  const uart_port_t uart_num = UART_NUM_2;
  uart_config_t uart_config = {
      .baud_rate = 9600,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
      .rx_flow_ctrl_thresh = 122,
  };

  //Install UART driver, and get the queue.
  ESP_ERROR_CHECK(
      uart_driver_install(
          uart_num, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0));
  // Configure UART parameters
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_ERROR_CHECK(
      uart_set_pin(
          uart_num, 17, 16,
          UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  int satelites_valid = 0;
  double last_lat=89.9999;
  double last_lng=0;

  while (1) {
    service_uart_queue(uart_num, uart_queue);

    if (gps.satellites.isValid() != (satelites_valid!=0)) {
      satelites_valid = gps.satellites.isValid();

      if (satelites_valid) {
        ESP_LOGI(TAG, "Satellite tracking acquired - # tracked: %d",
            gps.satellites.value());
      } else {
        ESP_LOGI(TAG, "Satellite tracking lost - # tracked: %d",
            gps.satellites.value());
      }
    }

    if (gps.location.isValid() && gps.location.isUpdated()) {
      double lat = gps.location.lat();
      double lng = gps.location.lng();
      ESP_LOGI(TAG, "Valid position: %.6f %.6f - Satellites: %d",
          lat, lng, gps.satellites.value());

      double dist_meters = TinyGPSPlus::distanceBetween(
          last_lat, last_lng, lat, lng);

      if ((dist_meters > 50) && is_mqtt_subscribed() && is_time_synced()) {
        last_lat = lat;
        last_lng = lng;
        time_t ts;

        time(&ts);
        ESP_LOGI(TAG, "[%lu] GPS Event: %.6f %.6f\n", ts, lat, lng);

        char* temp_evt = create_gps_position_event(lat, lng, ts);
        snprintf(topic, sizeof(topic), "iot/sensors/%s/events/gps/position",
            settings.device_id);
        mqtt_publish(topic, temp_evt);
        free(temp_evt);
      }
    }
  }
}


extern "C" void init_gps_module() {
  xTaskCreatePinnedToCore(&gps_task, "gps_task", 8192, NULL, 12, NULL, 0);
}
