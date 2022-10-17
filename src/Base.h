#ifndef BASE_H
#define BASE_H

#include "Arduino.h"
#include "PubSubClient.h"
#include <ArduinoJson.h>
#include <vector>
#include "Logger.h"

class Base
{

public:
    inline Base(PubSubClient *mqttClient, bool *enableQos)
    {
        this->mqttClient = mqttClient;
        this->mqttQoS = enableQos;
    }

protected:
    PubSubClient *mqttClient;
    bool *mqttQoS;

    inline const uint8_t detectSizeOf(const char *msg, ...)
    {
        va_list args;
        va_start(args, msg);
        const int32_t result = JSON_STRING_SIZE(vsnprintf_P(nullptr, 0U, msg, args));
        assert(result >= 0);
        va_end(args);
        return result;
    }

};

#endif // BASE_H