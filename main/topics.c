#include "settings.h"


void getSensorReadingTopic(char * topic, int buflen, const char* sensorType) {
    snprintf(topic, buflen, "iot/sensor/%s/%s/%s/events/reading",
             settings.datacenter_id,
             settings.device_id,
             sensorType);
}

void getSensorStatusTopic(char * topic, int buflen, const char* sensorType) {
    snprintf(topic, buflen, "iot/sensor/%s/%s/status",
             settings.datacenter_id,
             settings.device_id);
}
