#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "esp_http_server.h"
#include "settings.h"
#include "status.h"
#include "wifi.h"
#include "esp_spiffs.h"
#include "esp_log.h"


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

esp_err_t init_web() {
  esp_err_t err = init_fs();
  if (err != ESP_OK) {
    return err;
  }

  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
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
