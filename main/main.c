/* Esptouch example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "settings.h"
#include "wifi.h"
#include "web.h"
#include "temp_sensor.h"
#include "ntp.h"
#include "trigger_sensor.h"
#include "analog_sensor.h"
#include "gps_module.h"

static const char *TAG = "main";
#define ENABLE_TEMPERATURE_SENSOR 1
#define ENABLE_TILT_SENSOR 0
#define ENABLE_MOTION_SENSOR 0
#define ENABLE_NOISE_SENSOR 0
#define ENABLE_GPS_MODULE 1

void app_main(void) {
  ESP_LOGI(TAG, "main function start.");

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  nvs_init();

  wifi_init();

  time_init();

  ESP_ERROR_CHECK(init_web());

  if (ENABLE_TEMPERATURE_SENSOR) {
    init_temp_sensor();
  }
  if (ENABLE_TILT_SENSOR) {
    init_trigger_sensor("tilt");
  }
  if (ENABLE_MOTION_SENSOR) {
    init_trigger_sensor("motion");
  }
  if (ENABLE_NOISE_SENSOR) {
    init_analog_sensor("noise");
  }
  if (ENABLE_GPS_MODULE) {
    init_gps_module();
  }
}

