#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>

#define MYDEBUG
//#define DEBUG_ESP_HTTP_UPDATE

// Misc
#define DEVICEID_PREFIX "environment"
#define DALLAS_GPIO 5

#ifndef DEEP_SLEEP_TIME_SECONDS
#define DEEP_SLEEP_TIME_SECONDS 5
#endif

#ifndef SSID_RESCANLOOPS
#define SSID_RESCANLOOPS 0
#endif

// WiFi Settings
// IPAddress ip(192, 168, 2, 82);
// IPAddress dns(192, 168, 2, 4);
// IPAddress gw(192, 168, 2, 254);
// IPAddress subnet(255, 255, 255, 0);

#define WIFI_PASSWORD "password"
#define WIFI_SSID "ssid"

// DWEET
// sensor 3
//#define PUBLISH_DWEET

#ifndef DWEET_URL
#define DWEET_URL "http://dweet.io/dweet/quietly/for/<dweetguid>"
#endif

// MQTT
#define PUBLISH_MQTT
//#define HADISCOVERY
#define MQTT_BROKER "<broker IP>"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

// if discovering then need MQTT
#ifdef HADISCOVERY
#define PUBLISH_MQTT
#endif

#ifndef MQTT_TOPIC
#define MQTT_TOPIC "<environment data root MQTT topic>"
#endif

#ifndef SSID_TOPIC
#define SSID_TOPIC "<ssid list MQTT topic>"
#endif

/// Firmware
#ifndef FW_VERSION
#define FW_VERSION 100
#endif
#define FW_URL_BASE "http://192.168.2.5/firmware/"

