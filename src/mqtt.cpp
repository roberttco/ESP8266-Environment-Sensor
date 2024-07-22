#include <Arduino.h>

#include "config.h"
#include "debug.h"
#include "mqtt.h"

#ifdef PUBLISH_MQTT

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

extern String device_unique_id;
extern String device_unique_id_sans_prefix;

#ifdef HADISCOVERY
String make_sensor_config_string(String uid_postfix,
                          String sensorName,
                          String valueTemplate,
                          String unitOfMeasurement,
                          String deviceClass,
                          bool isDiagnostic,
                          String parentUniqueId,
                          String parentFriendlyName,
                          bool includeDeviceDetails)
{
  String sensor_config_message(
    "{"
    "\"name\":\"" + sensorName + "\","
    "\"unique_id\":\"" + device_unique_id + "_" + uid_postfix + "\","
    "\"state_topic\":\"" + MQTT_TOPIC + "\","
    "\"value_template\":\"" + valueTemplate + "\",");

    if (isDiagnostic)
    {
      sensor_config_message += "\"entity_category\":\"diagnostic\",";
    }
    else
    {
      sensor_config_message +=
        "\"device_class\":\"" + deviceClass  + "\","
        "\"unit_of_measurement\":\"" + unitOfMeasurement + "\",";
    }

    sensor_config_message += "\"device\":{\"identifiers\":\"" + parentUniqueId + "\"";

    if (includeDeviceDetails)
    {
      char itoabuf[6];
      itoa(FW_VERSION, itoabuf, 10);
      sensor_config_message += ",\"name\":\"" + parentFriendlyName + " " + device_unique_id_sans_prefix + "\",\"sw_version\":\"" + itoabuf + "\"";
    }

    sensor_config_message += "}}";

  return sensor_config_message;
}

void SendHomeAssistantDiscovery()
{
  DEBUG_PRINT("Sending home assistant discovery MQTT message for ");
  DEBUG_PRINTLN(device_unique_id);

  // #### Temperature
  String sensor_config_topic = {"homeassistant/sensor/__DEVICE_UNIQUE_ID__/temperature/config"};
  sensor_config_topic.replace("__DEVICE_UNIQUE_ID__", device_unique_id);

  String sensor_config_payload = make_sensor_config_string("temperature", "Temperature", "{{ value_json.environment.tempc }}", "°C", "temperature", false, device_unique_id, "Environment Sensor",true);

  MQTT_ConnectAndSend(device_unique_id, (char *)sensor_config_topic.c_str(), sensor_config_payload, true);

  // #### Voltage
  sensor_config_topic = {"homeassistant/sensor/__DEVICE_UNIQUE_ID__/voltage/config"};
  sensor_config_topic.replace("__DEVICE_UNIQUE_ID__", device_unique_id);

  sensor_config_payload = make_sensor_config_string("voltage", "Voltage", "{{ value_json.power.vdd }}", "V", "voltage", false, device_unique_id, "Environment Sensor",false);

  MQTT_ConnectAndSend(device_unique_id, (char *)sensor_config_topic.c_str(), sensor_config_payload, true);

  // #### Pressure
  sensor_config_topic = {"homeassistant/sensor/__DEVICE_UNIQUE_ID__/pressure/config"};
  sensor_config_topic.replace("__DEVICE_UNIQUE_ID__", device_unique_id);

  sensor_config_payload = make_sensor_config_string("pressure", "Pressure", "{{ value_json.environment.pressure_hpa }}", "hPa", "pressure", false, device_unique_id, "Environment Sensor",false);

  MQTT_ConnectAndSend(device_unique_id, (char *)sensor_config_topic.c_str(), sensor_config_payload, true);

  // #### Humidity
  sensor_config_topic = {"homeassistant/sensor/__DEVICE_UNIQUE_ID__/humidity/config"};
  sensor_config_topic.replace("__DEVICE_UNIQUE_ID__", device_unique_id);

  sensor_config_payload = make_sensor_config_string("humidity", "Humidity", "{{ value_json.environment.humidity }}", "%", "humidity", false, device_unique_id, "Environment Sensor",false);

  MQTT_ConnectAndSend(device_unique_id, (char *)sensor_config_topic.c_str(), sensor_config_payload, true);

  // #### ssid
  sensor_config_topic = {"homeassistant/sensor/__DEVICE_UNIQUE_ID__/ssid/config"};
  sensor_config_topic.replace("__DEVICE_UNIQUE_ID__", device_unique_id);

  sensor_config_payload = make_sensor_config_string("ssid", "SSID", "{{ value_json.wifi.ssid }}", "", "none", true, device_unique_id, "Environment Sensor",false);

  MQTT_ConnectAndSend(device_unique_id, (char *)sensor_config_topic.c_str(), sensor_config_payload, true);

  // #### rss
  sensor_config_topic = {"homeassistant/sensor/__DEVICE_UNIQUE_ID__/rss/config"};
  sensor_config_topic.replace("__DEVICE_UNIQUE_ID__", device_unique_id);

  sensor_config_payload = make_sensor_config_string("rss", "RSS", "{{ value_json.wifi.rssi }}", "", "none", true, device_unique_id, "Environment Sensor",false);

  MQTT_ConnectAndSend(device_unique_id, (char *)sensor_config_topic.c_str(), sensor_config_payload, true);

  // #### MAC
  sensor_config_topic = {"homeassistant/sensor/__DEVICE_UNIQUE_ID__/mac/config"};
  sensor_config_topic.replace("__DEVICE_UNIQUE_ID__", device_unique_id);

  sensor_config_payload = make_sensor_config_string("mac", "MAC", "{{ value_json.wifi.mac }}", "", "none", true, device_unique_id, "Environment Sensor",false);

  MQTT_ConnectAndSend(device_unique_id, (char *)sensor_config_topic.c_str(), sensor_config_payload, true);

  // #### FW build date
  sensor_config_topic = {"homeassistant/sensor/__DEVICE_UNIQUE_ID__/builddate/config"};
  sensor_config_topic.replace("__DEVICE_UNIQUE_ID__", device_unique_id);

  sensor_config_payload = make_sensor_config_string("builddate", "BuildDate", "{{ value_json.firmware.date }}", "", "none", true, device_unique_id, "Environment Sensor",false);

  MQTT_ConnectAndSend(device_unique_id, (char *)sensor_config_topic.c_str(), sensor_config_payload, true);
}

#endif // HADISCOVERY

void MQTT_Configure()
{
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setBufferSize(512);
}

bool MQTT_ConnectAndSend(String clientId, char *topic, String msg, bool retained)
{
  const char *buf = msg.c_str();
  int bufSize = msg.length();

  DEBUG_PRINT("MQTT_ConnectAndSend message length = ");
  DEBUG_PRINTLN(bufSize);

  // Loop until we're reconnected
  int retries = 3;
  bool mqtt_connected = mqttClient.connected();

  while (mqtt_connected == false && retries > 0)
  {
    DEBUG_PRINT("Attempting to connect to MQTT broker ");
    // Attempt to connect
    if ((mqtt_connected = mqttClient.connect(clientId.c_str())) == false)
    {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(mqttClient.state());
      DEBUG_PRINTLN(" try again in 1 second");
      // Wait 1 seconds before retrying
      delay(1000);
    }

    retries -= 1;
  }

  if (mqtt_connected)
  {
    DEBUG_PRINTLN("connected");
    // Once connected, publish an announcement...
    DEBUG_PRINTLN("Sending:");
    DEBUG_PRINTLN(buf);
    mqttClient.publish(topic, buf, retained);
  }

  return mqtt_connected;
}

#endif // PUBLISH_MQTT
