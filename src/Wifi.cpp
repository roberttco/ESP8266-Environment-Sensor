#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "config.h"
#include "debug.h"

#include "rtcdata.h"
#include "mqtt.h"
#include "wifi.h"

#ifdef MYDEBUG

const char *StatusToString(int status)
{
  switch (status)
  {
  case WL_CONNECTED:
    return "Connected";
  case WL_NO_SHIELD:
    return "No network hardware";
  case WL_IDLE_STATUS:
    return "Waiting for connection";
  case WL_NO_SSID_AVAIL:
    return "No ssid available";
  case WL_SCAN_COMPLETED:
    return "WiFi scan completed";
  case WL_CONNECT_FAILED:
    return "Connection failed";
  case WL_CONNECTION_LOST:
    return "Connection lost";
  case WL_DISCONNECTED:
    return "Disconnected";
  default:
    return "Unknown WiFi status";
  }
}

#endif

String oneWireAddressToString(uint8_t *mac)
{
  char result[33];

  snprintf(result, sizeof(result), "%d %d %d %d %d %d %d %d",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]);

  return String(result);
}

String macAddressToString(uint8_t *mac)
{
  char result[14];

  snprintf(result, sizeof(result), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return String(result);
}

boolean ConnectToWiFi(RtcData *rtcData)
{
  boolean rval = false;
  int loops = 0;
  int retries = 0;
  int wifiStatus = WL_DISCONNECTED;

  String _wifi_password = String(WIFI_PASSWORD);
  String _wifi_ssid = String(WIFI_SSID);

  for (loops = 0; loops < 3; loops++)
  {
    retries = 0;

    // the value will be zero if the data isn't valid
    boolean apvalid = ((rtcData != NULL) && ((rtcData->flags & APVALID) == APVALID));

    Serial.printf("loops = %d, valid = %0x, apvalid = %d\n", loops, rtcData->flags, (apvalid ? 1 : 0));

    if ((loops == 0) && (apvalid == true))
    {
      Serial.printf("loops = %d, valid = %0x\n", loops, rtcData->flags);
      // The RTC data was good, make a quick connection
      DEBUG_PRINTLN("Connecting to AP using stored AP channel and MAC");
      WiFi.begin(_wifi_ssid, _wifi_password, rtcData->channel, rtcData->ap_mac, true);
    }
    else
    {
      // The RTC data was not valid, so make a regular connection
      DEBUG_PRINTLN("Connecting to AP by discovering AP channel and MAC");
      WiFi.begin(_wifi_ssid, _wifi_password);
    }

    delay(50);

    wifiStatus = WiFi.status();
    while (wifiStatus != WL_CONNECTED)
    {
      retries++;
      if (retries == 300)
      {
        DEBUG_PRINTLN("No connection after 300 steps, power cycling the WiFi radio.");
        WiFi.disconnect();
        delay(10);
        WiFi.forceSleepBegin();
        delay(10);
        WiFi.forceSleepWake();
        delay(10);
        WiFi.begin(_wifi_ssid, _wifi_password);
      }

      if (retries >= 600)
      {
        WiFi.disconnect(true);
        delay(1);
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin();
        delay(10);
        DEBUG_PRINTLN("No connection after 600 steps. WiFi connection failed.");
        break;
      }
      else
      {
        delay(50);
        wifiStatus = WiFi.status();
      }
    }

    if (loops == 3)
    {
      DEBUG_PRINTLN("That was 3 loops, still no connection so let's go to deep sleep for 2 minutes");
      Serial.flush();
      ESP.deepSleep(120000000, WAKE_RF_DISABLED);
    }
    else
    {
      rval = (wifiStatus == WL_CONNECTED);
    }

    return rval;
  }

  if (wifiStatus == WL_CONNECTED)
  {
    rval = true;

    DEBUG_PRINT("Connected to ");
    DEBUG_PRINTLN(WIFI_SSID);
    DEBUG_PRINT("Assigned IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());
  }

  return rval;
}

String ScanSsids()
{
  String msg = "{";

  DEBUG_PRINTLN("Starting WiFi SSID Scan");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();

  DEBUG_PRINTLN("scan done");

  if (n == 0)
  {
    DEBUG_PRINTLN("no networks found");
    msg.concat("\"ap\":[]}");
  }
  else
  {
    msg.concat("\"ap\":[");

    DEBUG_PRINT(n);
    DEBUG_PRINTLN(" networks found");

    for (int i = 0; i < min(20, n); ++i)
    {
      // Print SSID and RSSI for THE FIRST 20 networks found
      char apbuf[64];
      snprintf(apbuf, 63, "{\"n\":\"%s\",\"p\":\"%i\"},",
               WiFi.SSID(i).substring(0,31).c_str(),
               WiFi.RSSI(i));

      msg.concat(apbuf);
    }

    // remove the last comma befor adding the closing brackets.
    msg.substring(0,msg.length()-2).concat("]");
  }

  msg.concat("}");

  return msg;
}
