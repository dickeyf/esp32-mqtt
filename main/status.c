#include <stdint.h>

#include "cjson.h"
#include "status.h"
#include "settings.h"
#include "wifi.h"
#include "SensorStatusSchema.h"

uint32_t status = 0;

char *get_health() {
    char ipAddress[16];
    snprintf(ipAddress, sizeof(ipAddress), IPSTR, IP2STR(&sta_ip_info.ip));

    struct SensorStatus sensorStatus = {
        .jsonObj = NULL,
        .wifi_ip = ipAddress,
        .mqtt_connected = is_mqtt_connected(),
        .device_id = settings.device_id,
        .wifi_up = is_station_connected(),
        .time_synced = is_time_synced(),
        .mqtt_subscribed = is_mqtt_subscribed()
    };

    cJSON *healthResponse = create_SensorStatusSchema(&sensorStatus);
    char *response = cJSON_PrintUnformatted(healthResponse);
    cJSON_Delete(healthResponse);

    return response;
}
