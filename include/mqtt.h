#pragma once

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

void MQTT_Configure();
bool MQTT_ConnectAndSend(String clientId, char * topic, String msg, bool retained);
void SendHomeAssistantDiscovery();
