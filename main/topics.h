#ifdef __cplusplus
extern "C" {
#endif

void getSensorReadingTopic(char * topic, int buflen, const char* sensorType);
void getSensorStatusTopic(char * topic, int buflen, const char* sensorType);

#ifdef __cplusplus
}
#endif