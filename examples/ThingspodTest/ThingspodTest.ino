#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <Thingspod.h>

#define CURRENT_FIRMWARE_TITLE "TEST"
#define CURRENT_FIRMWARE_VERSION "1.0.0"

#define PROVISIONING_DEVICE_NAME "ProvisionedDevice"
#define PROVISIONING_DEVICE_KEY "glvrlhqffkongkik8w5h"
#define PROVISIONING_SECRET_KEY "ltzbcv2voso4fkbmu4pl"

#define WIFI_SSID "Ronika"
#define WIFI_PASSWORD "RonikaCoLtdRouter2022Q3"

const char *TOKEN = "OTADevice";
#define THINGSPOD_SERVER "platform.r9k.ir"
#define THINGSPOD_MQTT_PORT 30883

#define SERIAL_DEBUG_BAUD 115200

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Thingspod thingspod(wifiClient, &mqttClient);

const std::array<const char *, 2U> attrKey{"attr1", "attr2"};

class Logger;

int status = WL_IDLE_STATUS;

void setup()
{
  Serial.begin(SERIAL_DEBUG_BAUD);
  Serial.println();
  initWifi();
  mqttClient.setServer(THINGSPOD_SERVER, 30883);
  mqttClient.setCallback(mqttCallback);
}

void initWifi()
{
  Serial.println("\nConnecting to AP ...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {

    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to AP");
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  thingspod.onMessage(topic, payload, length);
}

const void OTAUpdate(const bool &success)
{
  if (success)
  {
    Serial.println("Done, Reboot now");
#if defined(ESP8266)
    ESP.restart();
#elif defined(ESP32)
    esp_restart();
#endif
  }
  else
  {
    Serial.println("No new firmware");
  }
}

RPCCallback rpc("test", rpcCallbacl);
RPCResponse rpcCallbacl(const RPCData &data)
{
  Logger::log("rpc callback");
  String output;
  serializeJson(data, output);
  Logger::log(output.c_str());
  return RPCResponse();
}

SharedAttributeCallback sharedAttrRequest(attrKey.cbegin(), attrKey.cend(), sharedAttrCallback);
void sharedAttrCallback(const SharedAttributeData &data)
{
  Logger::log("sharedAttrCallBack");
  String output;
  serializeJson(data, output);
  Logger::log(output.c_str());
}

ProvisionCallback provision(provisionCallback);
void provisionCallback(const ProvisionData &data)
{
  Logger::log("provisioning data callback");
  String output;
  serializeJson(data, output);
  Logger::log(output.c_str());
  TOKEN = data[ACCESS_TOKEN_CREDENTIAL_KEY].as<const char *>();
  thingspod.disconnect();
}

void reconnectWifi()
{
  status = WiFi.status();

  if (status != WL_CONNECTED)
  {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }

    Serial.println("\nConnected to AP");
  }
}

void loop()
{
  delay(600);

  if (WiFi.status() != WL_CONNECTED)
  {
    reconnectWifi();
  }

  if (!thingspod.connected())
  {
    // Connect to the Thingspod
    Serial.print("Connecting to: ");
    Serial.print(THINGSPOD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);

    if (TOKEN == nullptr)
    {
      if (!thingspod.connect(THINGSPOD_SERVER, THINGSPOD_MQTT_PORT))
      {
        Serial.println("Failed to connect");
      }
      Serial.println("Connect for provisioning...");

      thingspod.provisionSubscribe(provision);
      thingspod.sendProvisionRequest(PROVISIONING_DEVICE_NAME, PROVISIONING_DEVICE_KEY, PROVISIONING_SECRET_KEY);
      return;
    }

    if (!thingspod.connect(THINGSPOD_SERVER, THINGSPOD_MQTT_PORT, TOKEN))
    {
      Serial.println("Failed to connect");
      return;
    }
    thingspod.RPCSubscribe(rpc);
    thingspod.sharedAttributesSubscribe(sharedAttrRequest);
    thingspod.sendTelemetryInt("dataKey", 10);
    thingspod.sendAttributeInt("attr", 20);
    thingspod.startFirmwareUpdate(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, OTAUpdate);
  }

  if (thingspod.connected())
  {
    thingspod.mqttClientLoop();
  }
}