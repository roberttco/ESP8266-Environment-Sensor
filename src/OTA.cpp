
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include "config.h"
#include "debug.h"
#include "rtcdata.h"

extern RTCMemory<RtcData> rtcMemory;

#ifdef MYDEBUG

void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

#endif

void checkForUpdates(String mac)
{
  String fwURL = String( FW_URL_BASE );
  fwURL.concat( mac );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( "/version.txt" );

  DEBUG_PRINTLN( "Checking for firmware updates." );
  DEBUG_PRINT( "MAC address: " );
  DEBUG_PRINTLN( mac );
  DEBUG_PRINT( "Firmware version URL: " );
  DEBUG_PRINTLN( fwVersionURL );

  WiFiClient wifiClient;
  HTTPClient httpClient;

  httpClient.begin(wifiClient, fwVersionURL);

  int httpCode = httpClient.GET();
  if( httpCode == 200 )
  {
    String newFWVersion = httpClient.getString();

    DEBUG_PRINT( "Current firmware version: " );
    DEBUG_PRINTLN( FW_VERSION );
    DEBUG_PRINT( "Available firmware version: " );
    DEBUG_PRINTLN( newFWVersion );

    int newVersion = newFWVersion.toInt();

    if( newVersion != FW_VERSION )
    {
      String fwImageURL = fwURL + "/" + String(newVersion) + ".bin";
      DEBUG_PRINT( "Preparing to update from: " );
      DEBUG_PRINTLN(fwImageURL.c_str());

#ifdef MYDEBUG
      ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
      ESPhttpUpdate.onStart(update_started);
      ESPhttpUpdate.onEnd(update_finished);
      ESPhttpUpdate.onProgress(update_progress);
      ESPhttpUpdate.onError(update_error);
#endif

      t_httpUpdate_return ret = ESPhttpUpdate.update(wifiClient, fwImageURL);

      if (ret == HTTP_UPDATE_OK)
      {
        DEBUG_PRINTLN("Clearing RTC data structure.");

        // Get the data
        RtcData* rtcdata = rtcMemory.getData();
        rtcdata->flags = 0;
        rtcdata->channel = 0;
        memset(rtcdata->ap_mac,0,6);
        rtcdata->loopsBeforeScan = 0;
        rtcMemory.save();

        // after clearing the RTC memory, wait 10 seconds and reboot so there is time to
        // reconnect the sensor wire.
        delay(10000);
      }

#ifdef MYDEBUG
      switch(ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;

        case HTTP_UPDATE_OK:
          Serial.println("HTTP_UPDATE_OK");
          break;
      }
#endif
    }
    else
    {
      DEBUG_PRINTLN( "Already on latest version" );
    }
  }
  else
  {
    DEBUG_PRINT( "Firmware version check failed, got HTTP response code " );
    DEBUG_PRINTLN( httpCode );
  }

  httpClient.end();
}
