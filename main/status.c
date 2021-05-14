#include <stdint.h>

#include "cjson.h"
#include "status.h"

uint32_t status = 0;

char* get_health() {
  cJSON *healthResponse = cJSON_CreateObject();

  cJSON_AddBoolToObject(healthResponse, "station_up", is_station_connected());
  cJSON_AddBoolToObject(healthResponse, "station_ip", is_ip_acquired());
  cJSON_AddBoolToObject(healthResponse, "mqtt_connected", is_mqtt_connected());
  cJSON_AddBoolToObject(healthResponse, "mqtt_subscribed", is_mqtt_subscribed());
  char* response = cJSON_PrintUnformatted(healthResponse);
  cJSON_Delete(healthResponse);

  return response;
}
