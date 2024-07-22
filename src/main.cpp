#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DS18B20.h>

#include "config.h"
#include "debug.h"

#include "rtcdata.h"
#include "ota.h"
#include "dweet.h"
#include "mqtt.h"
#include "wifi.h"

// WIFI
const char * _wifi_password = "UbKNUJakLBLpOh";
const char * _wifi_ssid = "qsvMIMt8Fm6NV3";

// make it so the A/D converter reads VDD
ADC_MODE(ADC_VCC);

// #define CALIBRATE_VCC
#define VCC_CORRECTION -0.25
#define VCC_CUTOFF 2.00

unsigned long wificonnecttime = 0L;
int status = WL_IDLE_STATUS;

// #########################
// # DS18B20 variables
// #########################
OneWire oneWire(DALLAS_GPIO);
DS18B20 ds18b20_sensor(&oneWire);

uint8_t ds18b20address[8]; // = {40,216,131,138,6,0,0,237};
uint8_t selected;

RTCMemory<RtcData> rtcMemory;

// #########################
// # Misc variables
// #########################
float tempc;
float tempf;
double ucvdd;
double vdd;
RtcData* rtcData = NULL;

String device_mac_address;
String device_unique_id;
String device_unique_id_sans_prefix;

unsigned long starttime;
unsigned long sleepTimeSeconds = DEEP_SLEEP_TIME_SECONDS;

void setup()
{
  pinMode(4, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);

  starttime = millis();

#ifdef MYDEBUG
  Serial.begin(115200);
  while ( !Serial );
#endif

  device_unique_id_sans_prefix = WiFi.macAddress();
  device_unique_id_sans_prefix.replace(":","");
  device_mac_address = device_unique_id_sans_prefix;
  device_unique_id_sans_prefix = device_unique_id_sans_prefix.substring(6);

  device_unique_id = String(DEVICEID_PREFIX) + "_" + device_unique_id_sans_prefix;

  DEBUG_PRINT("Device unique identifiers:");
  DEBUG_PRINTLN(device_unique_id_sans_prefix);
  DEBUG_PRINTLN(device_unique_id);

  // start with WiFi turned off
  // https://www.bakke.online/index.php/2017/05/21/reducing-wifi-power-consumption-on-esp8266-part-2/
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay( 1 );


  DEBUG_PRINTLN ("\nRolling...");

  if(rtcMemory.begin()){
    DEBUG_PRINTLN("RTC memory library initialization done!");
  } else {
    DEBUG_PRINTLN("No previous RTC memory data found. The memory is reset to zeros!");
  }

  // Get the data
  rtcData = rtcMemory.getData();

#ifdef MYDEBUG
  Serial.printf("Valid: %X\nChannel: %d\nap_mac: %s\nLoops before scan: %X ",
    rtcData->flags,
    rtcData->channel,
    macAddressToString(rtcData->ap_mac).c_str(),
    rtcData->loopsBeforeScan);
#endif

  // if gpio 13 is low, then wipe the RTC data and force a rediscovery of the AP and the DS18B20 address
  int clear_a = digitalRead(13);
  int clear_b = digitalRead(14);
  if ((clear_a == 0) && (clear_b == 0))
  {
    DEBUG_PRINTLN("Clearing RTC data structure.");
    memset(rtcData,0,sizeof(RtcData));
    rtcMemory.save();

    // after clearing the RTC memory, wait 10 secoinds and reboot so there is time to
    // reconnect the sensor wire.
    delay(10000);
    ESP.restart();
  }

  ds18b20_sensor.begin();
  ds18b20_sensor.setResolution(10);

  MQTT_Configure();
}

void loop()
{
  // get the battery voltage
  ucvdd = ESP.getVcc() / 1000.0;
  vdd = ucvdd + VCC_CORRECTION;

#ifdef MYDEBUG
  Serial.printf ("Uncorrected VDD %f mV. Corrected to %f mV.\n",ucvdd,vdd);
#endif

#ifdef CALIBRATE_VCC
  DEBUG_PRINTLN("Take VCC reading now - you have 5 seconds.");
  //delay here to make it easier to read VCC with a volt meter to calibrate the VCC correction value
  delay(5000);
#endif

  ds18b20_sensor.requestTemperatures();

  //  wait until sensor is ready
  while (!ds18b20_sensor.isConversionComplete())
  {
    delay(1);
  }

  tempc = ds18b20_sensor.getTempC();
  DEBUG_PRINTLN("Got temperature");

  tempf = tempc * 1.8 + 32;

  // Now send out the measurements

  int connectRetries = 1;  // retry once with a wifi wakeup in the middle
  while (connectRetries > 0)
  {
    if (ConnectToWiFi(rtcData))
    {
      // Write current connection info back to RTC
      rtcData->channel = WiFi.channel();
      rtcData->flags |= APVALID;
      memcpy( rtcData->ap_mac, WiFi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)

      wificonnecttime = millis() - starttime;
      break;
    }
    else
    {
      // wait 10 second between retries.
      delay(10000);
    }

    connectRetries--;
  }

  if (connectRetries == 0)
  {
    DEBUG_PRINTLN("Connect retries exhausted.  Clearing RTC data and sleeping for 5 minutes.");

    // clear RTC data in case there is something amiss with the info - get a fresh start.
    memset(rtcData,0,sizeof(RtcData));
    sleepTimeSeconds = 300;
  }
  else
  {
    // get RSSI value including retries.
    DEBUG_PRINT("Getting RSSI ... ");

    connectRetries = 5;
    long rssi = WiFi.RSSI();
    while ((rssi > 0) && (connectRetries > 0))
    {
      connectRetries--;
      delay(50);
    }

    if (connectRetries <= 0)
    {
      rssi = 0;
    }

    DEBUG_PRINTLN(rssi);

#if defined PUBLISH_MQTT || defined PUBLISH_DWEET

#ifdef HADISCOVERY
    if ((rtcData->flags & HADSENT) == 0)
    {
      rtcData->flags |= HADSENT;
      SendHomeAssistantDiscovery();
    }
#endif

    if (WiFi.status() == WL_CONNECTED)
    {
      char buf[1024];
      snprintf(buf, sizeof(buf), "{ \"wifi\": { \"ssid\":\"%s\",\"connecttime\":\"%lu\", \"rssi\":\"%ld\",\"mac\":\"%s\" },"
                                  "\"power\" : { \"ucvdd\": \"%.2f\", \"vdd\": \"%.2f\", \"sleepTimeSeconds\" : \"%lu\", \"loopsBeforeSsidScan\" : \"%i\"},"
                                  "\"environment\" : { \"tempf\":\"%f\", \"tempc\":\"%f\", \"humidity\":\"%f\", \"pressure_hpa\":\"%f\", \"dewpoint\" : \"%f\" },"
                                  "\"firmware\" : { \"version\":\"%d\",\"date\":\"%s %s\" }}",

              WIFI_SSID,
              wificonnecttime,
              rssi,
              device_mac_address.c_str(),

              ucvdd,
              vdd,
              sleepTimeSeconds,
              rtcData->loopsBeforeScan,

              tempf,
              tempc,
              0.0,
              0.0,
              0.0,

              FW_VERSION,
              __DATE__,
              __TIME__);

      String result = String(buf);
#ifdef PUBLISH_DWEET
      SendDweet(result);
#endif //PUBLISH_DWEET

#ifdef PUBLISH_MQTT
      MQTT_ConnectAndSend(device_unique_id, MQTT_TOPIC, result, false);

      if (SSID_RESCANLOOPS > 0)
      {
        if (rtcData->loopsBeforeScan <= 0)
        {
          String idjson = ScanSsids();
          rtcData->loopsBeforeScan = SSID_RESCANLOOPS;

          MQTT_ConnectAndSend(device_unique_id, SSID_TOPIC, idjson, false);
        }
        else
        {
          rtcData->loopsBeforeScan = rtcData->loopsBeforeScan - 1;
        }
      }

#endif //PUBLISH_MQTT
#endif //PUBLISH_DWEET || PUBLISH_MQTT
      checkForUpdates(device_mac_address);
    }
  }

  rtcMemory.save();

#ifdef MYDEBUG
  Serial.printf ("Total runtime: %ld mS\n",millis() - starttime);
#endif

  DEBUG_PRINTLN("\n=====\nTime to sleep...");
  WiFi.disconnect( true );
  delay( 1 );

  if (vdd < VCC_CUTOFF)
  {
    // if Vdd < 3 volts then go to sleep for 1 hour hoping for some sun
    DEBUG_PRINTLN ("Sleeping for 20 minutes because Vdd is too low - hoping for some sun.");
    sleepTimeSeconds = 1200;
  }
  else
  {
    sleepTimeSeconds = DEEP_SLEEP_TIME_SECONDS;
  }

  // WAKE_RF_DISABLED to keep the WiFi radio disabled when we wake up
#ifdef MYDEBUG
  Serial.printf("Sleeping for %lu seconds.\n",sleepTimeSeconds);
#endif

  ESP.deepSleep(sleepTimeSeconds * 1e6, WAKE_RF_DISABLED );
}
