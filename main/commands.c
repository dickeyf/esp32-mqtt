#include "esp_log.h"
#include "SensorCommandSchema.h"

static const char *TAG = "COMMANDS";

void handleCommand(const char* jsonTextData) {
    struct SensorCommand* sensorCommand;
    const char* error_msg;
    if (read_SensorCommandSchema(jsonTextData, &sensorCommand, &error_msg) != ESP_OK) {
        ESP_LOGW(TAG, "Error parsing command: %s", error_msg);
        return;
    }

    ESP_LOGI(TAG, "Received command %s.  Param1: %s, Param2: %.2f, Param3: %d, Param4: %s",
             sensorCommand->commandName,
             sensorCommand->commandParam1,
             sensorCommand->commandParam2,
             sensorCommand->commandParam3,
             sensorCommand->commandParam4?"true":"false");

    free_SensorCommandSchema(sensorCommand);
}