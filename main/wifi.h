#include "esp_netif.h"

extern esp_netif_ip_info_t sta_ip_info;

char* get_wifi_networks();
void initialise_wifi_ap();
void on_wifi_settings_changed();
void wifi_init();
