#pragma once

#include <Arduino.h>
#include "rtcdata.h"

String oneWireAddressToString(uint8_t *mac);
String macAddressToString(uint8_t *mac);
boolean ConnectToWiFi(RtcData *rtcData);
String ScanSsids();
