#ifndef PROVISIONING_H
#define PROVISIONING_H

#include "Base.h"
#include "Claiming.h"
#include "Attribute.h"

constexpr char *PROVISION_RESPONSE_TOPIC PROGMEM = "/provision/response";
constexpr char *PROVISION_ACCESS_TOKEN PROGMEM = "provision";

#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_AVR_MEGA)

constexpr char *PROVISION_REQUEST_TOPIC PROGMEM = "/provision/request";

constexpr char *PROVISION_STATUS_KEY PROGMEM = "status";
constexpr char *PROVISION_CREDENTIAL_TYPE_KEY PROGMEM = "credentialsType";
constexpr char *STATUS_SUCCESS PROGMEM = "SUCCESS";
constexpr char *X509_CREDENTIAL_TYPE PROGMEM = "X509_CERTIFICATE";

constexpr char *ACCESS_TOKEN_CREDENTIAL_TYPE PROGMEM = "ACCESS_TOKEN";
constexpr char *ACCESS_TOKEN_CREDENTIAL_KEY PROGMEM = "credentialsValue";

constexpr char *MQTT_BASIC_TOKEN_CREDENTIAL_TYPE PROGMEM = "MQTT_BASIC";

constexpr char *PROVISION_DEVICE_KEY PROGMEM = "provisionDeviceKey";
constexpr char *PROVISION_DEVICE_SECRET_KEY PROGMEM = "provisionDeviceSecret";

#endif

using ProvisionData = const JsonObjectConst;

class ProvisionCallback
{
  template <size_t PayloadSize, size_t MaxFieldsElement, typename Logger>
  friend class ProvisioningTemplate;

public:
  using processFn = std::function<void(const ProvisionData &data)>;

  inline ProvisionCallback()
      : callbackFunction(nullptr) {}

  inline ProvisionCallback(processFn cb)
      : callbackFunction(cb) {}

private:
  processFn callbackFunction;
};

template <
    size_t PayloadSize,
    size_t MaxFieldsElement,
    typename Logger = Logger>
class ProvisioningTemplate : public Base
{

public:
  inline ProvisioningTemplate(PubSubClient *mqttClient, bool *enableQos) : Base(mqttClient, enableQos)
  {
  }

  inline bool isProvisionResponseTopic(const char *const topic)
  {
    return strncmp_P(topic, PROVISION_RESPONSE_TOPIC, strlen(PROVISION_RESPONSE_TOPIC)) == 0;
  }

#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_AVR_MEGA)

  inline const bool sendProvisionRequest(const char *deviceName, const char *provisionDeviceKey, const char *provisionDeviceSecret)
  {
    StaticJsonDocument<JSON_OBJECT_SIZE(3)> requestBuffer;
    JsonObject requestObject = requestBuffer.to<JsonObject>();

    String deviceNameKey = DEVICE_NAME_KEY;
    String provisionKey = PROVISION_DEVICE_KEY;
    String secretKey = PROVISION_DEVICE_SECRET_KEY;
    requestObject[deviceNameKey.c_str()] = deviceName;
    requestObject[provisionKey.c_str()] = provisionDeviceKey;
    requestObject[secretKey.c_str()] = provisionDeviceSecret;

    uint8_t objectSize = JSON_STRING_SIZE(measureJson(requestBuffer));
    char requestPayload[objectSize];
    serializeJson(requestObject, requestPayload, objectSize);

    Logger::log(PROVISION_REQUEST);
    Logger::log(requestPayload);
    return (*mqttClient).publish(PROVISION_REQUEST_TOPIC, requestPayload, (*mqttQoS));
  }

  inline const bool provisionSubscribe(const ProvisionCallback callback)
  {
    if (!(*mqttClient).subscribe(PROVISION_RESPONSE_TOPIC, (*mqttQoS) ? 1 : 0))
    {
      return false;
    }
    this->provisionCallback = callback;
    return true;
  }

  inline const bool unsubscribeFromProvisioning()
  {
    if (!(*mqttClient).unsubscribe(PROVISION_RESPONSE_TOPIC))
    {
      return false;
    }
    return true;
  }

  inline void processProvisioningResponseMessage(char *topic, uint8_t *payload, uint32_t length)
  {
    Logger::log(PROVISION_RESPONSE);

    StaticJsonDocument<JSON_OBJECT_SIZE(MaxFieldsElement)> jsonBuffer;
    DeserializationError payloadDeserializationError = deserializeJson(jsonBuffer, payload, length);
    if (payloadDeserializationError)
    {
      Logger::log(UNABLE_TO_DE_SERIALIZE_PROVISION_RESPONSE);
      return;
    }

    const JsonObject &data = jsonBuffer.template as<JsonObject>();

    Logger::log(RECEIVED_PROVISION_RESPONSE);

    if (strncmp_P(data[PROVISION_STATUS_KEY], STATUS_SUCCESS, strlen(STATUS_SUCCESS)) == 0 && strncmp_P(data[PROVISION_CREDENTIAL_TYPE_KEY], X509_CREDENTIAL_TYPE, strlen(X509_CREDENTIAL_TYPE)) == 0)
    {
      Logger::log(X509_NOT_SUPPORTED);
      return;
    }

    if (this->provisionCallback.callbackFunction)
    {
      this->provisionCallback.callbackFunction(data);
    }
  }
#endif

private:
  ProvisionCallback provisionCallback;
};

#endif // THINGSPOD_LOGGER_H