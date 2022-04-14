#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "esp_http_server.h"
#include "settings.h"
#include "camera.h"
#include "status.h"
#include "wifi.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "motors.h"

static const char *TAG = "WEB_SERVER";

static const char *WEB_MOUNT_POINT = "/www";
#define SCRATCH_BUFSIZE (10240)
static char scratch[SCRATCH_BUFSIZE];
#define FILE_PATH_MAX (128)


static esp_err_t rest_settings_delete_handler(httpd_req_t *req) {
  const char* error_msg;
  esp_err_t err = delete_settings(&error_msg);

  if (err != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, error_msg);
    return ESP_OK;
  }

  httpd_resp_sendstr(req, "");

  return ESP_OK;
}

static esp_err_t rest_settings_get_handler(httpd_req_t *req) {
  const char* errorMsg;
  char* response = get_settings(&errorMsg);

  if (response == NULL) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, errorMsg);

    return ESP_OK;
  }

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_sendstr(req, response);

  free(response);

  return ESP_OK;
}

static esp_err_t motors_handler(httpd_req_t *req) {
    int bytes_read = httpd_req_recv(req, scratch, SCRATCH_BUFSIZE);

    if (bytes_read > 0) {
        scratch[bytes_read] = 0;

        if (memcmp(scratch, "forward ", 8)==0) {
            int32_t speed = atoi(&scratch[8]);
            ESP_LOGI(TAG, "Got forward command, speed = %d", speed);
            if (speed >= 0) {
                motors_forward(speed);
            } else {
                motors_backward(abs(speed));
            }
        } else if (memcmp(scratch, "backward ", 9)==0) {
            int32_t speed = atoi(&scratch[9]);
            ESP_LOGI(TAG, "Got backward command, speed = %d", speed);
            if (speed >= 0) {
                motors_backward(speed);
            } else {
                motors_forward(abs(speed));
            }
        } else if (memcmp(scratch, "left ", 5)==0) {
            int32_t speed = atoi(&scratch[5]);
            ESP_LOGI(TAG, "Got left command, speed = %d", speed);
            if (speed >= 0) {
                motors_left(speed);
            } else {
                motors_right(abs(speed));
            }
        } else if (memcmp(scratch, "right ", 6)==0) {
            int32_t speed = atoi(&scratch[6]);
            ESP_LOGI(TAG, "Got right command, speed = %d", speed);
            if (speed >= 0) {
                motors_right(speed);
            } else {
                motors_left(abs(speed));
            }
        }

        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_status(req, "202 Accepted");
        httpd_resp_sendstr(req, "");
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "Failed to parse request as JSON.");
    }

    return ESP_OK;
}

static esp_err_t rest_settings_save_handler(httpd_req_t *req) {
  int bytes_read = httpd_req_recv(req, scratch, SCRATCH_BUFSIZE);

  if (bytes_read > 0) {
    scratch[bytes_read] = 0;

    const char* error_msg;
    if (post_settings(scratch, &error_msg) != ESP_OK) {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, error_msg);
      return ESP_OK;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_status(req, "202 Accepted");
    httpd_resp_sendstr(req, "");
  } else {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
        "Failed to parse request as JSON.");
  }

  return ESP_OK;
}

static esp_err_t rest_common_option_handler(httpd_req_t *req) {
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Max-Age", "600");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "*");
  httpd_resp_set_status(req, "204 No Content");
  httpd_resp_sendstr(req, "");

  return ESP_OK;
}

static esp_err_t rest_wifi_networks_get_handler(httpd_req_t *req) {
  char* response = get_wifi_networks();

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_sendstr(req, response);

  free(response);

  return ESP_OK;
}

static esp_err_t rest_health_get_handler(httpd_req_t *req) {
  char* response = get_health();

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_sendstr(req, response);

  free(response);

  return ESP_OK;
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req) {
  char filepath[FILE_PATH_MAX];

  strlcpy(filepath, WEB_MOUNT_POINT, sizeof(filepath));
  if (req->uri[strlen(req->uri) - 1] == '/') {
    strlcat(filepath, "/index.html", sizeof(filepath));
  } else {
    strlcat(filepath, req->uri, sizeof(filepath));
  }
  int fd = open(filepath, O_RDONLY, 0);
  if (fd == -1) {
    ESP_LOGE(TAG, "Failed to open file : %s", filepath);
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
        "Failed to read existing file");
    return ESP_OK;
  }

  httpd_resp_set_type(req, "text/html");

  ssize_t read_bytes;
  do {
    /* Read file in chunks into the scratch buffer */
    read_bytes = read(fd, scratch, SCRATCH_BUFSIZE);
    if (read_bytes == -1) {
      ESP_LOGE(TAG, "Failed to read file : %s", filepath);
    } else if (read_bytes > 0) {
      /* Send the buffer contents as HTTP response chunk */
      if (httpd_resp_send_chunk(req, scratch, read_bytes) != ESP_OK) {
        close(fd);
        ESP_LOGE(TAG, "File sending failed!");
        /* Abort sending file */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
            "Failed to send file");
        return ESP_OK;
      }
    }
  } while (read_bytes > 0);
  /* Close file after sending complete */
  close(fd);
  ESP_LOGI(TAG, "File sending complete");
  /* Respond with an empty chunk to signal HTTP response completion */
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

esp_err_t init_fs(void) {
  esp_vfs_spiffs_conf_t conf = {
      .base_path = WEB_MOUNT_POINT,
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false };
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
        esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
  return ESP_OK;
}

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

esp_err_t streaming_camera_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    camera_flash(0);

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if(!jpeg_converted){
                ESP_LOGE(TAG, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
            ESP_LOGI(TAG, "MJPG: jpeg_converted");
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
                 (uint32_t)(_jpg_buf_len/1024),
                 (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}


esp_err_t init_web() {
  esp_err_t err = init_fs();
  if (err != ESP_OK) {
    return err;
  }

  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 16;
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "Starting HTTP Server");
  httpd_start(&server, &config);

  httpd_uri_t wifi_list_get_uri = {
      .uri = "/wifi/networks",
      .method = HTTP_GET,
      .handler = rest_wifi_networks_get_handler,
      .user_ctx = NULL };
  httpd_register_uri_handler(server, &wifi_list_get_uri);

  httpd_uri_t settings_save_uri = {
      .uri = "/settings",
      .method = HTTP_POST,
      .handler = rest_settings_save_handler,
      .user_ctx = NULL };
  httpd_register_uri_handler(server, &settings_save_uri);

  httpd_uri_t settings_get_uri = {
      .uri = "/settings",
      .method = HTTP_GET,
      .handler = rest_settings_get_handler,
      .user_ctx = NULL };
  httpd_register_uri_handler(server, &settings_get_uri);

  httpd_uri_t settings_delete_uri = {
        .uri = "/settings",
        .method = HTTP_DELETE,
        .handler = rest_settings_delete_handler,
        .user_ctx = NULL };
    httpd_register_uri_handler(server, &settings_delete_uri);

  httpd_uri_t camera_uri = {
        .uri = "/camera",
        .method = HTTP_GET,
        .handler = streaming_camera_handler,
        .user_ctx = NULL };
  httpd_register_uri_handler(server, &camera_uri);

    httpd_uri_t motors_uri = {
            .uri = "/motors",
            .method = HTTP_POST,
            .handler = motors_handler,
            .user_ctx = NULL };
    httpd_register_uri_handler(server, &motors_uri);


    httpd_uri_t health_get_uri = {
        .uri = "/health",
        .method = HTTP_GET,
        .handler = rest_health_get_handler,
        .user_ctx = NULL };
    httpd_register_uri_handler(server, &health_get_uri);

  httpd_uri_t common_get_uri = { .uri = "/*", .method = HTTP_GET, .handler =
      rest_common_get_handler, .user_ctx = NULL };
  httpd_register_uri_handler(server, &common_get_uri);

  httpd_uri_t common_option_uri = { .uri = "/*", .method = HTTP_OPTIONS, .handler =
      rest_common_option_handler, .user_ctx = NULL };
  httpd_register_uri_handler(server, &common_option_uri);

  return ESP_OK;
}
