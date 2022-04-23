#include "servo.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#define PWM_TIMER LEDC_TIMER_1

#define SERVO_A_PWM_GPIO 13
#define SERVO_A_PWM_CHANNEL LEDC_CHANNEL_1

/**
 * NOTES:
 * Min pulse width 500 us = smallest angle
 * Max pulse width 2500 us = widest angle
 *
 * PWM freq: 50Hz, period: 20000us
 * Min pulse width: 1/40th of period
 * Max pulse width: 5/40th of period
 *
 * @param angle
 */

void set_servo_angle(double angle) {
  int duty = ((1.0/40.0) + (1.0/10.0)*angle)*(1 << LEDC_TIMER_13_BIT);

  ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, SERVO_A_PWM_CHANNEL, duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, SERVO_A_PWM_CHANNEL));
}

void init_servos() {

  ledc_timer_config_t ledc_timer = {
          .speed_mode       = LEDC_HIGH_SPEED_MODE,
          .timer_num        = PWM_TIMER,
          .duty_resolution  = LEDC_TIMER_13_BIT,
          .freq_hz          = 50,  // Set output frequency at 50 Hz
          .clk_cfg          = LEDC_AUTO_CLK
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));


  ledc_channel_config_t ledc_config;
  ledc_config.channel = SERVO_A_PWM_CHANNEL;
  ledc_config.gpio_num = SERVO_A_PWM_GPIO;
  ledc_config.speed_mode = LEDC_HIGH_SPEED_MODE;
  ledc_config.intr_type = LEDC_INTR_DISABLE;
  ledc_config.duty = (1.0/40.0)*(1 >> ledc_timer.duty_resolution);
  ledc_config.hpoint = 0;
  ledc_config.timer_sel = PWM_TIMER;
  ledc_channel_config(&ledc_config);
}
