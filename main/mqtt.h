#ifdef __cplusplus
extern "C" {
#endif

void mqtt_start(void);
int mqtt_publish(char* topic, const char* payload);

#ifdef __cplusplus
}
#endif
