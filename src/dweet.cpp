#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "config.h"
#include "debug.h"

#ifdef PUBLISH_DWEET  // only include this code if publishing with dweet

#include <ESP8266WiFi.h>

boolean SendDweet(String msg)
{
  const char *buf = msg.c_str();

  boolean rval = false;
  if (WiFi.status() == WL_CONNECTED)
  {
    DEBUG_PRINTLN("Sending to dweet.io");
    DEBUG_PRINTLN(buf);

    WiFiClient wificlient;
    HTTPClient http;    //Declare object of class HTTPClient

    http.begin(wificlient, DWEET_URL);      //Specify request destination
    http.addHeader("Content-Type", "application/json");  //Specify content-type header

    unsigned int httpCode = http.POST(buf);   //Send the request

    DEBUG_PRINT("HTTP Code: ");
    DEBUG_PRINTLN(httpCode);   //Print HTTP return code

    // No need to end if the system is being restarted because of deep sleep
    http.end();  //Close connection

    rval = true;
  }
  else
  {
    DEBUG_PRINTLN ("Not connected to WiFi - cant send to dweet.io");
  }

  return rval;
}

#endif
