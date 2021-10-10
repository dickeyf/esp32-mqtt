#include <stdint.h>

#include "cjson.h"
#include "status.h"
#include "settings.h"
#include "mqtt.h"
#include "wifi.h"
#include "SensorStatusMessage.h"

uint32_t status = 0;

void build_status(struct SensorStatus* sensorStatus) {
    char ipAddress[16];
    snprintf(ipAddress, sizeof(ipAddress), IPSTR, IP2STR(&sta_ip_info.ip));

    sensorStatus->jsonObj = NULL;
    sensorStatus->wifi_ip = ipAddress;
    sensorStatus->mqtt_connected = is_mqtt_connected();
    sensorStatus->device_id = settings.device_id;
    sensorStatus->wifi_up = is_station_connected();
    sensorStatus->time_synced = is_time_synced();
    sensorStatus->mqtt_subscribed = is_mqtt_subscribed();
}

char *get_health() {
    struct SensorStatus sensorStatus;
    build_status(&sensorStatus);

    return create_SensorStatusMessage(&sensorStatus);
}

void publish_health() {
    struct SensorStatus sensorStatus;
    build_status(&sensorStatus);
    publish_SensorStatusMessage(get_mqtt_client(), settings.datacenter_id, settings.device_id, &sensorStatus);
}
