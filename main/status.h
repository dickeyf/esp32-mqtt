#include <stdint.h>

extern uint32_t status;

#include "esp_bit_defs.h"

#define station_connected() status |= BIT0
#define ip_acquired() status |= BIT1
#define mqtt_connected() status |= BIT2
#define mqtt_subscribed() status |= BIT3

#define station_disconnected() status &= !BIT0
#define ip_lost() status &= !BIT1
#define mqtt_disconnected() status &= !BIT2
#define mqtt_unsubscribed() status &= !BIT3

#define is_station_connected() (status & BIT0)
#define is_ip_acquired() (status & BIT1)
#define is_mqtt_connected() (status & BIT2)
#define is_mqtt_subscribed() (status & BIT3)

char* get_health();
