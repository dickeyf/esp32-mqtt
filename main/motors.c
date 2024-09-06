#include <esp_types.h>

#include "motors.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "MOTORS_MODULE";

#define PWM_TIMER LEDC_TIMER_1

#define MOTOR_LEFT_FORWARD_GPIO 12
#define MOTOR_LEFT_BACKWARD_GPIO 13
#define MOTOR_RIGHT_FORWARD_GPIO 14
#define MOTOR_RIGHT_BACKWARD_GPIO 15
#define MOTOR_LEFT_FORWARD_PWM_CHANNEL LEDC_CHANNEL_1
#define MOTOR_LEFT_BACKWARD_PWM_CHANNEL LEDC_CHANNEL_2
#define MOTOR_RIGHT_FORWARD_PWM_CHANNEL LEDC_CHANNEL_3
#define MOTOR_RIGHT_BACKWARD_PWM_CHANNEL LEDC_CHANNEL_4

QueueHandle_t motor_queue;

void set_motor_speed(ledc_channel_t channel, uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, channel));
}

void stop_motors() {
    set_motor_speed(MOTOR_LEFT_FORWARD_PWM_CHANNEL, 0);
    set_motor_speed(MOTOR_LEFT_BACKWARD_PWM_CHANNEL, 0);
    set_motor_speed(MOTOR_RIGHT_FORWARD_PWM_CHANNEL, 0);
    set_motor_speed(MOTOR_RIGHT_BACKWARD_PWM_CHANNEL, 0);
}


void motors_forward(uint32_t speed) {
    set_motor_speed(MOTOR_LEFT_FORWARD_PWM_CHANNEL, speed);
    set_motor_speed(MOTOR_LEFT_BACKWARD_PWM_CHANNEL, 0);
    set_motor_speed(MOTOR_RIGHT_FORWARD_PWM_CHANNEL, speed);
    set_motor_speed(MOTOR_RIGHT_BACKWARD_PWM_CHANNEL, 0);
}

void motors_backward(uint32_t speed) {
    set_motor_speed(MOTOR_LEFT_FORWARD_PWM_CHANNEL, 0);
    set_motor_speed(MOTOR_LEFT_BACKWARD_PWM_CHANNEL, speed);
    set_motor_speed(MOTOR_RIGHT_FORWARD_PWM_CHANNEL, 0);
    set_motor_speed(MOTOR_RIGHT_BACKWARD_PWM_CHANNEL, speed);
}

void motors_left(uint32_t speed) {
    set_motor_speed(MOTOR_LEFT_FORWARD_PWM_CHANNEL, 0);
    set_motor_speed(MOTOR_LEFT_BACKWARD_PWM_CHANNEL, speed);
    set_motor_speed(MOTOR_RIGHT_FORWARD_PWM_CHANNEL, speed);
    set_motor_speed(MOTOR_RIGHT_BACKWARD_PWM_CHANNEL, 0);

}

void motors_right(uint32_t speed) {
    set_motor_speed(MOTOR_LEFT_FORWARD_PWM_CHANNEL, speed);
    set_motor_speed(MOTOR_LEFT_BACKWARD_PWM_CHANNEL, 0);
    set_motor_speed(MOTOR_RIGHT_FORWARD_PWM_CHANNEL, 0);
    set_motor_speed(MOTOR_RIGHT_BACKWARD_PWM_CHANNEL, speed);

}

struct motor_command {
    int left_tract_speed;
    int right_tract_speed;
};

void motors_update_thrust(int speed, int angle) {
    struct motor_command motorCommand = {
            .left_tract_speed = 0,
            .right_tract_speed = 0
    };
    if (angle > -45 && angle < 45) {
        motorCommand.left_tract_speed = speed;
        motorCommand.right_tract_speed = speed;
    } else if (angle <= -45 && angle > -135) {
        motorCommand.left_tract_speed = -speed;
        motorCommand.right_tract_speed = speed;
    } else if (angle < -135 || angle > 135) {
        motorCommand.left_tract_speed = -speed;
        motorCommand.right_tract_speed = -speed;
    } else {
        motorCommand.left_tract_speed = speed;
        motorCommand.right_tract_speed = -speed;
    }

    xQueueSend(motor_queue, (void*)&motorCommand, 10/portTICK_PERIOD_MS);
}

void service_motor_queue() {
    struct motor_command motorCommand;

    if (xQueueReceive(motor_queue, (void*)&motorCommand, 1000 / portTICK_PERIOD_MS)) {
        if (motorCommand.right_tract_speed<0) {
            set_motor_speed(MOTOR_RIGHT_BACKWARD_PWM_CHANNEL, abs(motorCommand.right_tract_speed));
            set_motor_speed(MOTOR_RIGHT_FORWARD_PWM_CHANNEL, 0);
        } else {
            set_motor_speed(MOTOR_RIGHT_BACKWARD_PWM_CHANNEL, 0);
            set_motor_speed(MOTOR_RIGHT_FORWARD_PWM_CHANNEL, abs(motorCommand.right_tract_speed));
        }

        if (motorCommand.left_tract_speed<0) {
            set_motor_speed(MOTOR_LEFT_BACKWARD_PWM_CHANNEL, abs(motorCommand.left_tract_speed));
            set_motor_speed(MOTOR_LEFT_FORWARD_PWM_CHANNEL, 0);
        } else {
            set_motor_speed(MOTOR_LEFT_BACKWARD_PWM_CHANNEL, 0);
            set_motor_speed(MOTOR_LEFT_FORWARD_PWM_CHANNEL, abs(motorCommand.left_tract_speed));
        }
    } else {
        // Stop motors, must have lost signal.
        motors_forward(0);
    }
}

static void motors_task(void *pvParameters) {
    motor_queue =
            xQueueCreate( 4, sizeof(struct motor_command) );

    ESP_LOGI(TAG, "Motors task initialized.");
    while (1) {
        service_motor_queue();
    }
}

void init_motors() {
    ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_HIGH_SPEED_MODE,
            .timer_num        = PWM_TIMER,
            .duty_resolution  = LEDC_TIMER_13_BIT,
            .freq_hz          = 5000,  // Set output frequency at 5 kHz
            .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_config;
    ledc_config.channel = MOTOR_LEFT_FORWARD_PWM_CHANNEL;
    ledc_config.gpio_num = MOTOR_LEFT_FORWARD_GPIO;
    ledc_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_config.intr_type = LEDC_INTR_DISABLE;
    ledc_config.duty = 0;
    ledc_config.hpoint = 0;
    ledc_config.timer_sel = PWM_TIMER;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_config));

    ledc_config.channel = MOTOR_LEFT_BACKWARD_PWM_CHANNEL;
    ledc_config.gpio_num = MOTOR_LEFT_BACKWARD_GPIO;
    ledc_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_config.intr_type = LEDC_INTR_DISABLE;
    ledc_config.duty = 0;
    ledc_config.hpoint = 0;
    ledc_config.timer_sel = PWM_TIMER;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_config));

    ledc_config.channel = MOTOR_RIGHT_FORWARD_PWM_CHANNEL;
    ledc_config.gpio_num = MOTOR_RIGHT_FORWARD_GPIO;
    ledc_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_config.intr_type = LEDC_INTR_DISABLE;
    ledc_config.duty = 0;
    ledc_config.hpoint = 0;
    ledc_config.timer_sel = PWM_TIMER;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_config));

    ledc_config.channel = MOTOR_RIGHT_BACKWARD_PWM_CHANNEL;
    ledc_config.gpio_num = MOTOR_RIGHT_BACKWARD_GPIO;
    ledc_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_config.intr_type = LEDC_INTR_DISABLE;
    ledc_config.duty = 0;
    ledc_config.hpoint = 0;
    ledc_config.timer_sel = PWM_TIMER;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_config));

    xTaskCreatePinnedToCore(&motors_task, "motors_task", 8192, NULL, 12, NULL, 0);
}
