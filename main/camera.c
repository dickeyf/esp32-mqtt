#include <esp_types.h>

#include "camera.h"
#include "esp_camera.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "settings.h"
#include "status.h"
#include "mqtt.h"


static const char *TAG = "CAMERA_MODULE";

//WROVER-KIT PIN Map
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1 //software reset will be performed
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_FLASH_PIN    4

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

static camera_config_t camera_config = {
        .pin_pwdn  = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sscb_sda = CAM_PIN_SIOD,
        .pin_sscb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        .xclk_freq_hz = 20000000,//EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
        .frame_size = FRAMESIZE_VGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

        .jpeg_quality = 12, //0-63 lower number means higher quality
        .fb_count = 2, //if more than one, i2s runs in continuous mode. Use only with JPEG
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY//CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

void camera_flash(uint32_t turnOn) {
    gpio_set_level(CAM_FLASH_PIN, turnOn);
}

static void camera_stream_task(void *pvParameters) {
  char topic[256];
  camera_fb_t * fb = NULL;
  uint8_t * _jpg_buf;
  size_t _jpg_buf_len;
  snprintf(topic, sizeof(topic), "iot/sensor/%s/%s/camera/frames",
           settings.datacenter_id, settings.device_id);

  char* payload_buf = malloc(1024*100);
  time_t* ts = (time_t*)payload_buf;

  while (1) {
    if (is_mqtt_subscribed() && is_time_synced()) {
      fb = esp_camera_fb_get();
      time(ts);
      if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
      }
      if(fb->format != PIXFORMAT_JPEG){
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        if(!jpeg_converted){
          ESP_LOGE(TAG, "JPEG compression failed");
          esp_camera_fb_return(fb);
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          continue;
        }
        ESP_LOGI(TAG, "MJPG: jpeg_converted");
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
      memcpy(payload_buf+sizeof(time_t), _jpg_buf, fb->len);

      esp_mqtt_client_publish(get_mqtt_client(), topic, payload_buf, _jpg_buf_len+sizeof(time_t), 0, 0);

      if(fb->format != PIXFORMAT_JPEG){
        free(_jpg_buf);
      }
      esp_camera_fb_return(fb);
    } else {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}

void init_camera() {
    //power up the camera if PWDN pin is defined
    if(CAM_PIN_PWDN != -1){
        gpio_reset_pin(CAM_FLASH_PIN);
        gpio_set_direction(CAM_FLASH_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(CAM_FLASH_PIN, 0);

        gpio_reset_pin(CAM_PIN_PWDN);
        gpio_set_direction(CAM_PIN_PWDN, GPIO_MODE_OUTPUT);
        gpio_set_level(CAM_PIN_PWDN, 0);
    }

    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
    }

  xTaskCreatePinnedToCore(&camera_stream_task, "camera_stream_task", 4096, NULL, 5, NULL, 1);
}

