# IoT MQTT Base ESP32

This repository contains a boilerplate application that provides a MQTT client that connects over Wifi.

The Wifi and MQTT client can be configured via a REST configuration interface.

Initially when no configuration exists in the ESP32 NVS storage a default WIFI AP is started.  This AP SSID is
pigeon_esp and the password to join the Wifi is "dovecote" (See src/settings.c).

After you connect to the `pigeon_esp` WIFI AP, the REST interface will be available over it at http://192.168.4.1

Via this endpoint you will be able to configure the device (See the "Configuring a newly flashed device" section below)

### Configure the project

```
idf.py menuconfig
```

### Build and Flash

Build the project:
```
idf.py build
```
 
And then flash it to the board, 

```
idf.py -p PORT flash 
```

You can additionally run the monitor tool to view logs from UART 0:

```
idf.py -p PORT monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

### Configuring a newly flashed device

First, you must connect to the `pigeon_esp` WIFI AP of the newly flashed device.
The REST interface will be available over it at http://192.168.4.1

This cURL command can then be used to configure this device:
```
curl --location --request POST 'http://192.168.4.1/settings' \
--header 'Content-Type: application/json' \
--data-raw '{
    "wifi_ssid": "<YOUR Wifi SSID>",
    "wifi_password": "<YOUR Wifi password>",
    "ap_ssid": "<Your Device's AP Name>",
    "ap_password": "<Your Device's AP password>",
    "mqtt_uri": "<Your MQTT Broker's URL>",
    "mqtt_username": "<Your Device's MQTT Username>",
    "mqtt_password": "<Your Device's MQTT password>",
    "device_id": "<Your Device's ID>"
}'
```

Note that the ap_ssid and ap_password parameters will change the AP's settings from their defaults of 
pigeon_esp / dovecote.  This is important as the device will still act as an AP as a failsafe way to re-configure the
device.  This AP is always functional so the password should be secure.

It is also possible to use the `configure_device.sh` script to do the same.

Note that in either case the REST request will fail with a connection reset by peer, as the device won't wait before
re-configuring.