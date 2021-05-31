#!/bin/bash

usage () {
  echo "USAGE:"
  echo "./configure_device.sh [--host <DEVICE_HOSTNAME>] --dc <DATACENTER_ID> --ssid <WIFI SSID> --wifi-pwd <WIFI PASSPHRASE> --mqtt-uri <MQTT URL> --mqtt-user <MQTT USERNAME> --mqtt-pwd <MQTT PASSWORD> "
  echo "DEVICE_HOSTNAME: The hostname/IP Address of the ESP32 device.  By default this is 192.168.4.1 which is the case when you have joined the device's WIFI AP."
  echo "AP SSID: The SSID of the AP this device will listen onto."
  echo "AP PASSPHRASE: The WPA2 wifi passphrase for the AP this device will listen onto."
  echo "WIFI SSID: The SSID of the wifi network this device will join."
  echo "WIFI PASSPHRASE: The WPA2 wifi passphrase."
  echo "MQTT URL: The MQTT URL of the broker this device will be connecting to.  Exemple: wss://mr6ahxyib3jp2.messaging.solace.cloud:8443"
  echo "MQTT USERNAME: The MQTT USERNAME this device uses to authenticate to the broker ."
  echo "MQTT PASSWORD: The MQTT PASSWORD this device uses to authenticate to the broker."
  echo "DATACENTER_ID: The ID of the datacenter this devices connects to.  Defaults to 'default'."
  echo "DEVICE_ID: The ID of this devices.  This is by default the MQTT username specified here."
}

AP_SSID=pigeon_esp
AP_PWD=dovecote

DEVICE_HOSTNAME=192.168.4.1

while (( "$#" )); do
  case "$1" in
    --host)
      if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
        DEVICE_HOSTNAME=$2
        shift 2
      else
        echo "Error: device hostname is missing" >&2
        exit 1
      fi
      ;;
    --dev)
      if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
        DEVICE_ID=$2
        shift 2
      else
        echo "Error: device ID is missing" >&2
        exit 1
      fi
      ;;
    --dc)
      if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
        DATACENTER_ID=$2
        shift 2
      else
        echo "Error: DATACENTER_ID is missing" >&2
        exit 1
      fi
      ;;
    --ssid)
      if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
        WIFI_SSID=$2
        shift 2
      else
        echo "Error: ssid name is missing" >&2
        exit 1
      fi
      ;;
    --wifi-pwd)
      if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
        WIFI_PWD=$2
        shift 2
      else
        echo "Error: wifi password is missing" >&2
        exit 1
      fi
      ;;
    --mqtt-uri)
      if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
        MQTT_URI=$2
        shift 2
      else
        echo "Error: MQTT URI is missing" >&2
        exit 1
      fi
      ;;
    --mqtt-user)
      if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
        MQTT_USER=$2
        shift 2
      else
        echo "Error: MQTT username is missing" >&2
        exit 1
      fi
      ;;
    --mqtt-pwd)
      if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
        MQTT_PWD=$2
        shift 2
      else
        echo "Error: MQTT password is missing" >&2
        exit 1
      fi
      ;;
    -*|--*=) # unsupported flags
      echo "Error: Unsupported flag $1" >&2
      exit 1
      ;;
    *) # preserve positional arguments
      PARAMS="$PARAMS $1"
      shift
      ;;
  esac
done

if [[ -z ${WIFI_SSID} ]] || [[ -z ${WIFI_PWD} ]] || [[ -z ${MQTT_URI} ]] || [[ -z ${MQTT_USER} ]] || [[ -z ${MQTT_PWD} ]]
then
  usage
  exit 1
fi


if [[ -z ${DEVICE_ID} ]];
then
  DEVICE_ID=${MQTT_USER}
fi

if [[ -z ${DATACENTER_ID} ]];
then
  DATACENTER_ID=default
fi

AP_SSID=${MQTT_USER}
AP_PWD=${MQTT_PWD}

echo "Configuring ${DEVICE_HOSTNAME} with these parameters:"
echo "Device ID: ${DEVICE_ID}"
echo "DATACENTER_ID: ${DATACENTER_ID}"
echo "WIFI SSID to join: ${WIFI_SSID}"
echo "Device's AP SSID: ${AP_SSID}"
echo "MQTT Broker's URL: ${MQTT_URI}"


curl --location --request POST "http://${DEVICE_HOSTNAME}/settings" --header 'Content-Type: application/json' --data-raw "{\"wifi_ssid\": \"${WIFI_SSID}\", \"wifi_password\": \"${WIFI_PWD}\",\"ap_ssid\": \"${AP_SSID}\",\"ap_password\": \"${AP_PWD}\",\"mqtt_url\": \"${MQTT_URI}\",\"mqtt_username\": \"${MQTT_USER}\",\"mqtt_password\": \"${MQTT_PWD}\",\"device_id\": \"${DEVICE_ID}\",\"datacenter_id\": \"${DATACENTER_ID}\" }"

