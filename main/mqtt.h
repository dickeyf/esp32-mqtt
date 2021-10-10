#ifdef __cplusplus
extern "C" {
#endif
#include "mqtt_client.h"

void mqtt_start(void);
int mqtt_publish(char* topic, const char* payload);
esp_mqtt_client_handle_t get_mqtt_client();

#ifdef __cplusplus
}
#endif
