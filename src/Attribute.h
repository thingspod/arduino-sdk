#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "Base.h"
#include "Telemetry.h"

constexpr char *ATTRIBUTE_TOPIC  PROGMEM = "v1/devices/me/attributes";
constexpr char *ATTRIBUTE_RESPONSE_TOPIC  PROGMEM = "v1/devices/me/attributes/response";

constexpr char *ATTRIBUTE_REQUEST_TOPIC  PROGMEM = "v1/devices/me/attributes/request/%u";
constexpr char *ATTRIBUTE_RESPONSE_SUBSCRIBE_TOPIC  PROGMEM = "v1/devices/me/attributes/response/+";

constexpr char *SHARED_KEYS  PROGMEM = "sharedKeys";
constexpr char *SHARED_KEY  PROGMEM = "shared";

using Attribute = Telemetry;
using SharedAttributeData = const JsonObjectConst;

class SharedAttributeCallback
{
  template <size_t PayloadSize, size_t MaxFieldsElement, typename Logger>
  friend class AttributeTemplate;

public:
  using processFn = const std::function<void(const SharedAttributeData &data)>;

  inline SharedAttributeCallback()
      : attributes(), callbackFunction(nullptr) {}

  template <class InputIterator>
  inline SharedAttributeCallback(const InputIterator &first_itr, const InputIterator &last_itr, processFn cb)
      : attributes(first_itr, last_itr), callbackFunction(cb) {}

  inline SharedAttributeCallback(processFn cb)
      : attributes(), callbackFunction(cb) {}

private:
  const std::vector<const char *> attributes;
  processFn callbackFunction;
};

class SharedAttributeRequestCallback
{
  template <size_t PayloadSize, size_t MaxFieldsElement, typename Logger>
  friend class AttributeTemplate;

public:
  using processFn = std::function<void(const SharedAttributeData &data)>;

  inline SharedAttributeRequestCallback()
      : requestId(0U), callbackFunction(nullptr) {}

  inline SharedAttributeRequestCallback(processFn cb)
      : requestId(0U), callbackFunction(cb) {}

  inline SharedAttributeRequestCallback& setCallback(processFn cb){
    this->callbackFunction = cb;
    return *this;
  }
private:
  uint32_t requestId;         // Id the request was called with
  processFn callbackFunction; // Callback to call
};

template <
    size_t PayloadSize,
    size_t MaxFieldsElement,
    typename Logger = Logger>
class AttributeTemplate : public Base
{

public:
  inline AttributeTemplate(PubSubClient *mqttClient, bool *enableQos) : Base(mqttClient, enableQos)
  {
    this->requestId = 0;
    this->reserveCallbakSize();
  }

  inline void reserveCallbakSize(void)
  {
    this->sharedAttributeUpdateCallbacks.reserve(MaxFieldsElement);
    this->sharedAttributeRequestCallbacks.reserve(MaxFieldsElement);
  }

  inline bool isAttributeResponseMessage(const char *const topic)
  {
    return strncmp_P(topic, ATTRIBUTE_RESPONSE_TOPIC, strlen(ATTRIBUTE_RESPONSE_TOPIC)) == 0;
  }

  inline bool isAttributeMessage(const char *const topic)
  {
    return strncmp_P(topic, ATTRIBUTE_TOPIC, strlen(ATTRIBUTE_TOPIC)) == 0;
  }

  inline void processSharedAttributeUpdateMessage(char *topic, uint8_t *payload, uint32_t length)
  {
    StaticJsonDocument<JSON_OBJECT_SIZE(MaxFieldsElement)> jsonBuffer;
    DeserializationError payloadDeserializationError = deserializeJson(jsonBuffer, payload, length);
    if (payloadDeserializationError)
    {
      Logger::log(UNABLE_TO_DE_SERIALIZE_ATTRIBUTE_UPDATE);
      return;
    }
    JsonObject data = jsonBuffer.template as<JsonObject>();

    if (data && (data.size() >= 1))
    {
      Logger::log(RECEIVED_ATTRIBUTE_UPDATE);
      if (data.containsKey(SHARED_KEY))
      {
        data = data[SHARED_KEY];
      }
    }
    else
    {
      Logger::log(NOT_FOUND_ATTRIBUTE_UPDATE);
      return;
    }

    for (size_t i = 0; i < this->sharedAttributeUpdateCallbacks.size(); i++)
    {
      char id_message[detectSizeOf(ATTRIBUTE_CALLBACK_ID, i)];
      snprintf_P(id_message, sizeof(id_message), ATTRIBUTE_CALLBACK_ID, i);
      Logger::log(id_message);

      if (this->sharedAttributeUpdateCallbacks.at(i).callbackFunction == nullptr)
      {
        Logger::log(ATTRIBUTE_CALLBACK_IS_NULL);
        continue;
      }
      else if (this->sharedAttributeUpdateCallbacks.at(i).attributes.empty())
      {
        Logger::log(ATTRIBUTE_CALLBACK_NO_KEYS);
        this->sharedAttributeUpdateCallbacks.at(i).callbackFunction(data);
        continue;
      }

      bool containsKey = false;
      const char *requested_att;
      for (const char *att : this->sharedAttributeUpdateCallbacks.at(i).attributes)
      {
        if (att == nullptr)
        {
          Logger::log(ATTRIBUTE_IS_NULL);
          continue;
        }
        containsKey = containsKey || data.containsKey(att);

        if (containsKey)
        {
          char contained_message[JSON_STRING_SIZE(strlen(ATTRIBUTE_IN_ARRAY)) + JSON_STRING_SIZE(strlen(att))];
          snprintf_P(contained_message, sizeof(contained_message), ATTRIBUTE_IN_ARRAY, att);
          Logger::log(contained_message);
          requested_att = att;
          break;
        }
      }

      if (!containsKey || requested_att == nullptr)
      {
        Logger::log(ATTRIBUTE_NO_CHANGE);
        continue;
      }

      char calling_message[JSON_STRING_SIZE(strlen(CALLING_ATTRIBUTE_CALLBACK)) + JSON_STRING_SIZE(strlen(requested_att))];
      snprintf_P(calling_message, sizeof(calling_message), CALLING_ATTRIBUTE_CALLBACK, requested_att);
      Logger::log(calling_message);
      this->sharedAttributeUpdateCallbacks.at(i).callbackFunction(data);
    }
  }

  inline void processSharedAttributeRequestMessage(char *topic, uint8_t *payload, uint32_t length)
  {
    StaticJsonDocument<JSON_OBJECT_SIZE(MaxFieldsElement)> jsonBuffer;
    DeserializationError deserializePayloadError = deserializeJson(jsonBuffer, payload, length);
    if (deserializePayloadError)
    {
      Logger::log(UNABLE_TO_DE_SERIALIZE_ATTRIBUTE_REQUEST);
      return;
    }
    JsonObject data = jsonBuffer.template as<JsonObject>();

    if (data && (data.size() >= 1))
    {
      Logger::log(RECEIVED_ATTRIBUTE);
      if (data.containsKey(SHARED_KEY))
      {
        data = data[SHARED_KEY];
      }
    }
    else
    {
      Logger::log(ATTRIBUTE_KEY_NOT_FOUND);
      return;
    }

    std::string response = topic;
    size_t index = strlen(ATTRIBUTE_RESPONSE_TOPIC) + 1U;
    response = response.substr(index, response.length() - index);
    uint32_t response_id = atoi(response.c_str());

    for (size_t i = 0; i < this->sharedAttributeRequestCallbacks.size(); i++)
    {
      if (this->sharedAttributeRequestCallbacks.at(i).callbackFunction == nullptr)
      {
        Logger::log(ATTRIBUTE_REQUEST_CALLBACK_IS_NULL);
        continue;
      }
      else if (this->sharedAttributeRequestCallbacks.at(i).requestId != response_id)
      {
        continue;
      }

      char message[detectSizeOf(CALLING_REQUEST_ATTRIBUTE_CALLBACK, response_id)];
      snprintf_P(message, sizeof(message), CALLING_REQUEST_ATTRIBUTE_CALLBACK, response_id);
      Logger::log(message);

      this->sharedAttributeRequestCallbacks.at(i).callbackFunction(data);
      this->sharedAttributeRequestCallbacks.erase(std::next(this->sharedAttributeRequestCallbacks.begin(), i));
    }
  }

  template <class InputIterator>
  inline const bool sharedAttributesRequest(const InputIterator &first_itr, const InputIterator &last_itr, SharedAttributeRequestCallback &callback)
  {
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> requestBuffer;
    JsonObject requestObject = requestBuffer.to<JsonObject>();

    std::string sharedKeys;
    for (auto itr = first_itr; itr != last_itr; ++itr)
    {
      // Check if the given attribute is null, if it is skip it.
      if (*itr == nullptr)
      {
        continue;
      }
      sharedKeys.append(*itr);
      sharedKeys.push_back(COMMA);
    }

    // Check if any sharedKeys were requested.
    if (sharedKeys.empty())
    {
      Logger::log(NO_KEYS_TO_REQUEST);
      return false;
    }

    // Remove latest not needed ,
    sharedKeys.pop_back();
    String key = SHARED_KEYS;
    requestObject[key.c_str()] = sharedKeys.c_str();
    int objectSize = measureJson(requestBuffer) + 1;
    char buffer[objectSize];
    serializeJson(requestObject, buffer, objectSize);

    // Print requested keys.
    char message[JSON_STRING_SIZE(strlen(REQUEST_ATTRIBUTE)) + sharedKeys.length() + JSON_STRING_SIZE(strlen(buffer))];
    snprintf_P(message, sizeof(message), REQUEST_ATTRIBUTE, sharedKeys.c_str(), buffer);
    Logger::log(message);

    requestId++;
    callback.requestId = requestId;
    sharedAttributesRequestSubscribe(callback);

    char topic[detectSizeOf(ATTRIBUTE_REQUEST_TOPIC, requestId)];
    snprintf_P(topic, sizeof(topic), ATTRIBUTE_REQUEST_TOPIC, requestId);
    return (*mqttClient).publish(topic, buffer, (*mqttQoS));
  }

  // Subscribes multiple Shared attributes callbacks.
  template <class InputIterator>
  inline const bool sharedAttributesSubscribe(const InputIterator &first_itr, const InputIterator &last_itr)
  {
    const uint32_t size = std::distance(first_itr, last_itr);
    if (this->sharedAttributeUpdateCallbacks.size() + size > this->sharedAttributeUpdateCallbacks.capacity())
    {
      Logger::log(MAX_SHARED_ATTRIBUTE_UPDATE_EXCEEDED);
      return false;
    }
    if (!(*mqttClient).subscribe(ATTRIBUTE_TOPIC, (*mqttQoS) ? 1 : 0))
    {
      return false;
    }

    // Push back complete vector into our local m_sharedAttributeUpdateCallbacks vector.
    this->sharedAttributeUpdateCallbacks.insert(this->sharedAttributeUpdateCallbacks.end(), first_itr, last_itr);
    return true;
  }

  inline const bool sharedAttributesSubscribe(const SharedAttributeCallback &callback)
  {
    if (this->sharedAttributeUpdateCallbacks.size() + 1U > this->sharedAttributeUpdateCallbacks.capacity())
    {
      Logger::log(MAX_SHARED_ATTRIBUTE_UPDATE_EXCEEDED);
      return false;
    }
    if (!(*mqttClient).subscribe(ATTRIBUTE_TOPIC, (*mqttQoS) ? 1 : 0))
    {
      return false;
    }

    // Push back given callback into our local m_sharedAttributeUpdateCallbacks vector.
    this->sharedAttributeUpdateCallbacks.push_back(callback);
    return true;
  }

  inline const bool unsubscribeFromSharedAttribute()
  {
    this->sharedAttributeUpdateCallbacks.clear();
    if (!(*mqttClient).unsubscribe(ATTRIBUTE_TOPIC))
    {
      return false;
    }
    return true;
  }

  inline const bool unsubscribeFromSharedAttributeRequest()
  {
    this->sharedAttributeRequestCallbacks.clear();
    if (!(*mqttClient).unsubscribe(ATTRIBUTE_RESPONSE_SUBSCRIBE_TOPIC))
    {
      return false;
    }
    return true;
  }

private:
  uint32_t requestId; // Allows nearly 4.3 million requests before wrapping back to 0.
  std::vector<SharedAttributeCallback> sharedAttributeUpdateCallbacks;
  std::vector<SharedAttributeRequestCallback> sharedAttributeRequestCallbacks; // Shared attribute request callbacks array

  // Subscribe one Shared attributes request callback.
  inline const bool sharedAttributesRequestSubscribe(const SharedAttributeRequestCallback &callback)
  {
    if (this->sharedAttributeRequestCallbacks.size() + 1 > this->sharedAttributeRequestCallbacks.capacity())
    {
      Logger::log(MAX_SHARED_ATTRIBUTE_REQUEST_EXCEEDED);
      return false;
    }
    if (!(*mqttClient).subscribe(ATTRIBUTE_RESPONSE_SUBSCRIBE_TOPIC, (*mqttQoS) ? 1 : 0))
    {
      return false;
    }

    // Push back given callback into our local m_sharedAttributeRequestCallbacks vector.
    this->sharedAttributeRequestCallbacks.push_back(callback);
    return true;
  }
};

#endif // ATTRIBUTE_H