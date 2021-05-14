# IoT MQTT Base ESP32

This repository contains a boilerplate application that provides a MQTT client that connects over Wifi.

The Wifi and MQTT client can be configured via a REST configuration interface.

Initially when no configuration existing in the ESP32 NVS storage a default WIFI AP is started and the REST
interface will be available on it on http://192.168.4.1

### Configure the project

```
idf.py menuconfig
```

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

