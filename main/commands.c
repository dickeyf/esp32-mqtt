#include "esp_log.h"
#include "SensorCommandSchema.h"
#include "motors.h"
#include "servo.h"
#include <string.h>

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

    if (strcmp(sensorCommand->commandName, "thrust")==0) {
        int speed = sensorCommand->commandParam2 * 4000.0;
        int angle = sensorCommand->commandParam3;
        motors_update_thrust(speed, angle);
    } else if (strcmp(sensorCommand->commandName, "servo")==0) {
      double angle = sensorCommand->commandParam2;
      set_servo_angle(angle);
    }

    free_SensorCommandSchema(sensorCommand);
}