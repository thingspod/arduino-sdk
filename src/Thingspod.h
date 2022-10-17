#ifndef THINGSPOD_H
#define THINGSPOD_H

#include <Arduino.h>
#include "PubSubClient.h"
#include "ArduinoJson.h"
#include <vector>

#include "Provisioning.h"
#include "Firmware.h"
#include "Claiming.h"
#include "Attribute.h"
#include "Telemetry.h"
#include "Logger.h"
#include "RPC.h"

#define DEFAULT_PAYLOAD_SIZE 64
#define DEFAULT_FIELDS_ELEMENT 32

constexpr char *DEFAULT_CLIENT_ID PROGMEM = "thingspodDevice";

template <
	size_t PayloadSize = DEFAULT_PAYLOAD_SIZE,
	size_t MaxFieldsElement = DEFAULT_FIELDS_ELEMENT,
	typename Logger = Logger>
class ThingspodTemplate
{

public:
	inline ThingspodTemplate(Client &wifiClient, PubSubClient *mqttClient, const bool &enableQoS = false)
		: attribute(mqttClient, &mqttQoS),
		  rpc(mqttClient, &mqttQoS),
		  provisioning(mqttClient, &mqttQoS),
		  firmware(mqttClient, &mqttQoS, &attribute)
	{
		this->mqttQoS = enableQoS;
		this->mqttClient = mqttClient;
	}

	inline ThingspodTemplate(const bool &enableQoS = false)
		: mqttClient(),
		  attribute(&mqttClient, &mqttQoS),
		  rpc(&mqttClient, &mqttQoS),
		  provisioning(&mqttClient, &mqttQoS),
		  firmware(&mqttClient, &mqttQoS, &attribute)
	{
		this->mqttQoS = enableQoS;
	}

	inline ~ThingspodTemplate()
	{
		// Nothing to do.
	}

	inline PubSubClient &getMqttClient(void)
	{
		return (*mqttClient);
	}

	inline void enableMQTTQoS(const bool &enableQoS)
	{
		this->mqttQoS = enableQoS;
	}

	inline const bool connect(const char *host, int port = 1883, const String accessToken = PROVISION_ACCESS_TOKEN, const String clientId = DEFAULT_CLIENT_ID, const char *password = NULL)
	{
		if (!host)
		{
			return false;
		}

		(*mqttClient).setServer(host, port);
		const bool connection = (*mqttClient).connect(clientId.c_str(), accessToken.c_str(), password);
		if (connection)
		{
			this->rpc.unsubscribeFromRPC();
			this->unsubscribeFromSharedAttribute();
			this->unsubscribeFromSharedAttributeRequest();
#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_AVR_MEGA)
			this->unsubscribeFromProvisioning();
#endif
#if defined(ESP8266) || defined(ESP32)
			this->firmware.unsubscribeFromOTAFirmware();
#endif
		}
		else
		{
			Logger::log(CONNECT_FAILED);
		}
		return connection;
	}

	inline void disconnect()
	{
		(*mqttClient).disconnect();
	}

	inline const bool connected()
	{
		return (*mqttClient).connected();
	}

	inline void mqttClientLoop()
	{
		(*mqttClient).loop();
	}

	//----------------------------------------------------------------------------
	// Claiming API

#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_AVR_MEGA)

	inline const bool sendClaimingRequest(const char *secretKey, uint32_t durationMs)
	{
		StaticJsonDocument<JSON_OBJECT_SIZE(2)> requestBuffer;
		JsonObject responseObject = requestBuffer.to<JsonObject>();

		String secret = SECRET_KEY;
		String durationKey = DURATION_KEY;
		responseObject[secret.c_str()] = secretKey;
		responseObject[durationKey.c_str()] = durationMs;

		uint8_t objectSize = JSON_STRING_SIZE(measureJson(requestBuffer));
		char responsePayload[objectSize];
		serializeJson(responseObject, responsePayload, objectSize);

		return (*mqttClient).publish(CLAIM_TOPIC, responsePayload, this->mqttQoS);
	}

	inline const bool sendProvisionRequest(const char *deviceName, const char *provisionDeviceKey, const char *provisionDeviceSecret)
	{
		return this->provisioning.sendProvisionRequest(deviceName, provisionDeviceKey, provisionDeviceSecret);
	}

#endif

	//----------------------------------------------------------------------------
	// Telemetry API

	template <class T>
	inline const bool sendTelemetryData(const char *key, T value)
	{
		return sendKeyval(key, value);
	}

	inline const bool sendTelemetryInt(const char *key, int value)
	{
		return sendKeyval(key, value);
	}

	inline const bool sendTelemetryBool(const char *key, bool value)
	{
		return sendKeyval(key, value);
	}

	inline const bool sendTelemetryFloat(const char *key, float value)
	{
		return sendKeyval(key, value);
	}

	inline const bool sendTelemetryString(const char *key, const char *value)
	{
		return sendKeyval(key, value);
	}

	inline const bool sendTelemetry(const Telemetry *data, size_t data_count)
	{
		return sendDataArray(data, data_count);
	}

	inline const bool sendTelemetryJsonChar(const char *json)
	{
		if (json == nullptr)
		{
			return false;
		}

		const uint32_t json_size = JSON_STRING_SIZE(strlen(json));
		if (json_size > PayloadSize)
		{
			char message[detectSizeOf(INVALID_BUFFER_SIZE, PayloadSize, json_size)];
			snprintf_P(message, sizeof(message), INVALID_BUFFER_SIZE, PayloadSize, json_size);
			Logger::log(message);
			return false;
		}
		return (*mqttClient).publish(TELEMETRY_TOPIC, json, this->mqttQoS);
	}

	inline const bool sendTelemetryJson(const JsonObject &jsonObject)
	{
		const uint32_t json_object_size = jsonObject.size();
		if (MaxFieldsElement < json_object_size)
		{
			char message[detectSizeOf(TOO_MANY_JSON_FIELDS, json_object_size, MaxFieldsElement)];
			snprintf_P(message, sizeof(message), TOO_MANY_JSON_FIELDS, json_object_size, MaxFieldsElement);
			Logger::log(message);
			return false;
		}
		const uint32_t json_size = JSON_STRING_SIZE(measureJson(jsonObject));
		char json[json_size];
		serializeJson(jsonObject, json, json_size);
		return sendTelemetryJsonChar(json);
	}

	//----------------------------------------------------------------------------
	// Attribute API

	template <class T>
	inline const bool sendAttributeData(const char *attrName, T value)
	{
		return sendKeyval(attrName, value, false);
	}

	inline const bool sendAttributeInt(const char *attrName, int value)
	{
		return sendKeyval(attrName, value, false);
	}

	inline const bool sendAttributeBool(const char *attrName, bool value)
	{
		return sendKeyval(attrName, value, false);
	}

	inline const bool sendAttributeFloat(const char *attrName, float value)
	{
		return sendKeyval(attrName, value, false);
	}

	inline const bool sendAttributeString(const char *attrName, const char *value)
	{
		return sendKeyval(attrName, value, false);
	}

	inline const bool sendAttributes(const Attribute *data, size_t data_count)
	{
		return sendDataArray(data, data_count, false);
	}

	inline const bool sendAttributeJSONChar(const char *json)
	{
		if (json == nullptr)
		{
			return false;
		}

		const uint32_t json_size = JSON_STRING_SIZE(strlen(json));
		if (json_size > PayloadSize)
		{
			char message[detectSizeOf(INVALID_BUFFER_SIZE, PayloadSize, json_size)];
			snprintf_P(message, sizeof(message), INVALID_BUFFER_SIZE, PayloadSize, json_size);
			Logger::log(message);
			return false;
		}
		return (*mqttClient).publish(ATTRIBUTE_TOPIC, json, this->mqttQoS);
	}

	inline const bool sendAttributeJSON(const JsonObject &jsonObject)
	{
		const uint32_t json_object_size = jsonObject.size();
		if (MaxFieldsElement < json_object_size)
		{
			char message[detectSizeOf(TOO_MANY_JSON_FIELDS, json_object_size, MaxFieldsElement)];
			snprintf_P(message, sizeof(message), TOO_MANY_JSON_FIELDS, json_object_size, MaxFieldsElement);
			Logger::log(message);
			return false;
		}
		const uint32_t json_size = JSON_STRING_SIZE(measureJson(jsonObject));
		char json[json_size];
		serializeJson(jsonObject, json, json_size);
		return sendAttributeJSONChar(json);
	}

	//----------------------------------------------------------------------------
	// Server-side RPC API

	template <class InputIterator>
	inline const bool RPCSubscribe(const InputIterator &first_itr, const InputIterator &last_itr)
	{
		return this->rpc.RPCSubscribe(first_itr, last_itr);
	}

	inline const bool RPCSubscribe(const RPCCallback &callback)
	{
		return this->rpc.RPCSubscribe(callback);
	}

	inline const bool RPCUnsubscribe()
	{
		return this->rpc.RPCUnsubscribe();
	}

	//----------------------------------------------------------------------------
	// Firmware OTA API
#if defined(ESP8266) || defined(ESP32)

	inline const bool startFirmwareUpdate(const char *currFwTitle, const char *currFwVersion, const std::function<void(const bool &)> &updatedCallback)
	{

		this->firmware.startFirmwareUpdate(currFwTitle, currFwVersion, updatedCallback);
	}

	inline const bool unsubscribeFromOTAFirmware()
	{
		this->firmware.unsubscribeFromOTAFirmware();
	}

#endif

	//----------------------------------------------------------------------------
	// Shared attributes API

	template <class InputIterator>
	inline const bool sharedAttributesRequest(const InputIterator &first_itr, const InputIterator &last_itr, SharedAttributeRequestCallback &callback)
	{
		return this->attribute.sharedAttributesRequest(first_itr, last_itr, callback);
	}

	// Subscribes multiple Shared attributes callbacks.
	template <class InputIterator>
	inline const bool sharedAttributesSubscribe(const InputIterator &first_itr, const InputIterator &last_itr)
	{
		return this->attribute.sharedAttributesSubscribe(first_itr, last_itr);
	}

	// Subscribe one Shared attributes callback.
	inline const bool sharedAttributesSubscribe(const SharedAttributeCallback &callback)
	{
		return this->attribute.sharedAttributesSubscribe(callback);
	}

	inline const bool unsubscribeFromSharedAttribute()
	{
		return this->attribute.unsubscribeFromSharedAttribute();
	}

	inline const bool unsubscribeFromSharedAttributeRequest()
	{
		return this->attribute.unsubscribeFromSharedAttributeRequest();
	}

	// -------------------------------------------------------------------------------
	// Provisioning API

	inline const bool provisionSubscribe(const ProvisionCallback callback)
	{
		return this->provisioning.provisionSubscribe(callback);
	}

	inline const bool unsubscribeFromProvisioning()
	{
		return this->provisioning.unsubscribeFromProvisioning();
	}

	inline void onMessage(char *topic, uint8_t *payload, uint32_t length)
	{
		char message[JSON_STRING_SIZE(strlen(CALLBACK_FUNCTION_CALLED_MESSAGE)) + JSON_STRING_SIZE(strlen(topic))];
		snprintf_P(message, sizeof(message), CALLBACK_FUNCTION_CALLED_MESSAGE, topic);
		Logger::log(message);

		if (this->rpc.isRPCMessage(topic))
		{
			this->rpc.processRPCMessage(topic, payload, length);
		}
		else if (this->attribute.isAttributeResponseMessage(topic))
		{
			this->attribute.processSharedAttributeRequestMessage(topic, payload, length);
		}
		else if (this->attribute.isAttributeMessage(topic))
		{
			this->attribute.processSharedAttributeUpdateMessage(topic, payload, length);
		}
#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_AVR_MEGA)
		else if (this->provisioning.isProvisionResponseTopic(topic))
		{
			this->provisioning.processProvisioningResponseMessage(topic, payload, length);
		}
#endif
#if defined(ESP8266) || defined(ESP32)
		else if (this->firmware.isFirmwareResponseTopic(topic))
		{
			this->firmware.processFirmwareResponseMessage(topic, payload, length);
		}
#endif
	}

private:
	PubSubClient *mqttClient;
	bool mqttQoS;
	RPCTemplate<PayloadSize, MaxFieldsElement, Logger> rpc;
	AttributeTemplate<PayloadSize, MaxFieldsElement, Logger> attribute;
	ProvisioningTemplate<PayloadSize, MaxFieldsElement, Logger> provisioning;
	FirmwareTemplate<PayloadSize, MaxFieldsElement, Logger> firmware;

	inline const uint8_t detectSizeOf(const char *msg, ...)
	{
		va_list args;
		va_start(args, msg);
		const int32_t result = JSON_STRING_SIZE(vsnprintf_P(nullptr, 0U, msg, args));
		assert(result >= 0);
		va_end(args);
		return result;
	}

	template <typename T>
	inline const bool sendKeyval(const char *key, T value, bool telemetry = true)
	{
		Telemetry t(key, value);
		StaticJsonDocument<JSON_OBJECT_SIZE(1)> jsonBuffer;
		JsonVariant object = jsonBuffer.template to<JsonVariant>();
		if (!t.serializeKeyValue(object))
		{
			Logger::log(UNABLE_TO_SERIALIZE);
			return false;
		}

		return telemetry ? sendTelemetryJson(object) : sendAttributeJSON(object);
	}

	inline const bool sendDataArray(const Telemetry *data, size_t data_count, bool telemetry = true)
	{
		StaticJsonDocument<JSON_OBJECT_SIZE(MaxFieldsElement)> jsonBuffer;
		JsonVariant object = jsonBuffer.template to<JsonVariant>();

		for (size_t i = 0; i < data_count; ++i)
		{
			if (data[i].serializeKeyValue(object) == false)
			{
				Logger::log(UNABLE_TO_SERIALIZE);
				return false;
			}
		}

		return telemetry ? sendTelemetryJson(object) : sendAttributeJSON(object);
	}
};

using Thingspod = ThingspodTemplate<>;

#endif // THINGSPOD_H