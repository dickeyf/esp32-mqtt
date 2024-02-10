/* ESP32 async API code-gen example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdlib.h>
#include "esp_event.h"
#include "esp_log.h"
#include "settings.h"
#include "wifi.h"
#include "web.h"
#include "temp_sensor.h"
#include "trigger_sensor.h"
#include "analog_sensor.h"
#include "gps_module.h"
#include "distance_sensor.h"
#include "camera.h"
#include "motors.h"
#include "servo.h"

static const char *TAG = "main";
#define ENABLE_TEMPERATURE_SENSOR 0
#define ENABLE_TILT_SENSOR 0
#define ENABLE_MOTION_SENSOR 0
#define ENABLE_HALL_SENSOR 0
#define ENABLE_DISTANCE_MODULE 0
#define ENABLE_NOISE_SENSOR 0
#define ENABLE_GPS_MODULE 0
#define ENABLE_ESP32_CAM 1
#define ENABLE_MOTORS_DRIVER 0
#define ENABLE_SERVOS_DRIVER 0

void app_main(void) {
    ESP_LOGI(TAG, "main function start.");

    //Init motor as soon as possible to avoid wheels spinning at boot time (I wonder if I can flash a config to set the
    //pin)
    if (ENABLE_MOTORS_DRIVER) {
      init_motors();
    }

    if (ENABLE_SERVOS_DRIVER) {
      init_servos();
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    nvs_init();

    wifi_init();

    ESP_ERROR_CHECK(init_web());

    if (ENABLE_TEMPERATURE_SENSOR) {
        init_temp_sensor();
    }
    if (ENABLE_TILT_SENSOR) {
        init_trigger_sensor("tilt");
    }
    if (ENABLE_HALL_SENSOR) {
        init_trigger_sensor("hall");
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
    if (ENABLE_DISTANCE_MODULE) {
        init_distance_sensor();
    }
    if (ENABLE_ESP32_CAM) {
        init_camera();
    }
}

