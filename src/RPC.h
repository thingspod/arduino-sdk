#ifndef RPC_H
#define RPC_H

#include "Base.h"

constexpr char *RPC_TOPIC  PROGMEM = "v1/devices/me/rpc";
constexpr char *RPC_SUBSCRIBE_TOPIC  PROGMEM = "v1/devices/me/rpc/request/+";

constexpr char *RPC_METHOD_KEY  PROGMEM = "method";
constexpr char *RPC_PARAMS_KEY  PROGMEM = "params";
constexpr char *RPC_REQUEST_KEY  PROGMEM = "request";
constexpr char *RPC_RESPONSE_KEY  PROGMEM = "response";

using RPCResponse = Telemetry;
using RPCData = const JsonVariantConst;

class RPCCallback
{
    template <size_t PayloadSize, size_t MaxFieldsElement, typename Logger>
    friend class RPCTemplate;

public:
    using processFunction = std::function<RPCResponse(const RPCData &data)>;

    inline RPCCallback()
        : methodName(), callbackFunction(nullptr) {}

    inline RPCCallback(const char *methodName, processFunction callback)
        : methodName(methodName), callbackFunction(callback) {}

private:
    const char *methodName;
    processFunction callbackFunction;
};

template <
    size_t PayloadSize,
    size_t MaxFieldsElement,
    typename Logger = Logger>
class RPCTemplate : public Base
{

public:
    inline RPCTemplate(PubSubClient *mqttClient, bool *enableQos) : Base(mqttClient, enableQos)
    {
        this->reserveCallbakSize();
        Logger::log("rpc template created");
    }

    inline const bool unsubscribeFromRPC()
    {
        this->rpcCallbacks.clear();
        return (*this->mqttClient).unsubscribe(RPC_SUBSCRIBE_TOPIC);
    }

    inline bool isRPCMessage(const char *const topic)
    {
        return strncmp_P(topic, RPC_TOPIC, strlen(RPC_TOPIC)) == 0;
    }

    inline void processRPCMessage(char *topic, uint8_t *payload, uint32_t length)
    {
        RPCResponse rpcResponse;
        {
            StaticJsonDocument<JSON_OBJECT_SIZE(MaxFieldsElement)> jsonBuffer;
            DeserializationError deserializationPayloadError = deserializeJson(jsonBuffer, payload, length);

            if (deserializationPayloadError)
            {
                Logger::log(UNABLE_TO_DE_SERIALIZE_RPC);
                return;
            }

            const JsonObject &data = jsonBuffer.template as<JsonObject>();
            const char *methodName = data[RPC_METHOD_KEY].as<const char *>();
            const char *params = data[RPC_PARAMS_KEY].as<const char *>();

            if (methodName)
            {
                Logger::log(RECEIVED_RPC_LOG_MESSAGE);
                Logger::log(methodName);
            }
            else
            {
                Logger::log(RPC_METHOD_NULL);
                return;
            }

            for (const RPCCallback &callback : this->rpcCallbacks)
            {
                if (callback.callbackFunction == nullptr || callback.methodName == nullptr)
                {
                    Logger::log(RPC_CALLBACK_NULL);
                    continue;
                }
                else if (strncmp(callback.methodName, methodName, strlen(callback.methodName)) != 0)
                {
                    continue;
                }

                Logger::log(CALLING_RPC);
                Logger::log(methodName);

                if (!data.containsKey(RPC_PARAMS_KEY))
                {
                    Logger::log(NO_RPC_PARAMS_PASSED);
                }

                StaticJsonDocument<JSON_OBJECT_SIZE(MaxFieldsElement)> doc;
                DeserializationError paramDeserializationError = deserializeJson(doc, params);
                Logger::log(RPC_PARAMS_KEY);

                if (paramDeserializationError)
                {
                    const JsonVariant &param = data[RPC_PARAMS_KEY].as<JsonVariant>();
                    const uint32_t json_size = JSON_STRING_SIZE(measureJson(param));
                    char json[json_size];
                    serializeJson(param, json, json_size);
                    Logger::log(json);
                    rpcResponse = callback.callbackFunction(param);
                }
                else
                {
                    Logger::log(params);
                    const JsonObject &param = doc.template as<JsonObject>();
                    rpcResponse = callback.callbackFunction(param);
                }
                break;
            }
        }
        // Fill in response
        char responsePayload[PayloadSize] = {0};
        StaticJsonDocument<JSON_OBJECT_SIZE(1)> responseBuffer;
        JsonVariant responseObject = responseBuffer.template to<JsonVariant>();

        if (rpcResponse.serializeKeyValue(responseObject) == false)
        {
            Logger::log(UNABLE_TO_SERIALIZE);
            return;
        }

        const uint32_t json_size = JSON_STRING_SIZE(measureJson(responseBuffer));
        if (json_size > PayloadSize)
        {
            char message[detectSizeOf(INVALID_BUFFER_SIZE, PayloadSize, json_size)];
            snprintf_P(message, sizeof(message), INVALID_BUFFER_SIZE, PayloadSize, json_size);
            Logger::log(message);
            return;
        }

        serializeJson(responseObject, responsePayload, sizeof(responsePayload));

        String responseTopic = topic;
        responseTopic.replace(RPC_REQUEST_KEY, RPC_RESPONSE_KEY);
        Logger::log(RPC_RESPONSE_KEY);
        Logger::log(responseTopic.c_str());
        Logger::log(responsePayload);
        (*this->mqttClient).publish(responseTopic.c_str(), responsePayload, this->mqttQoS);
    }

    template <class InputIterator>
    inline const bool RPCSubscribe(const InputIterator &first_itr, const InputIterator &last_itr)
    {
        const uint32_t size = std::distance(first_itr, last_itr);
        if (this->rpcCallbacks.size() + size > this->rpcCallbacks.capacity())
        {
            Logger::log(MAX_RPC_EXCEEDED);
            return false;
        }
        if (!(*mqttClient).subscribe(RPC_SUBSCRIBE_TOPIC, (*mqttQoS) ? 1 : 0))
        {
            return false;
        }

        this->rpcCallbacks.insert(this->rpcCallbacks.end(), first_itr, last_itr);
        return true;
    }

    inline const bool RPCSubscribe(const RPCCallback &callback)
    {
        if (this->rpcCallbacks.size() + 1 > this->rpcCallbacks.capacity())
        {
            Logger::log(MAX_RPC_EXCEEDED);
            return false;
        }
        if (!(*mqttClient).subscribe(RPC_SUBSCRIBE_TOPIC, (*mqttQoS) ? 1 : 0))
        {
            return false;
        }

        this->rpcCallbacks.push_back(callback);
        Logger::log(RPC_SUBSCRIBE_TOPIC);
        return true;
    }

    inline const bool RPCUnsubscribe()
    {
        this->rpcCallbacks.clear();
        return (*mqttClient).unsubscribe(RPC_SUBSCRIBE_TOPIC);
    }

private:
    std::vector<RPCCallback> rpcCallbacks;

    inline void reserveCallbakSize(void)
    {
        this->rpcCallbacks.reserve(MaxFieldsElement);
    }
};

#endif // RPC_H