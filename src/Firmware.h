#ifndef FIRMWARE_H
#define FIRMWARE_H

#include "Base.h"
#include "Attribute.h"

#if defined(ESP8266)
#include <Updater.h>
#elif defined(ESP32)
#include <Update.h>
#include <MD5Builder.h>
#endif

constexpr char *FIRMWARE_RESPONSE_TOPIC PROGMEM = "v2/fw/response";

#if defined(ESP8266) || defined(ESP32) || !defined(ARDUINO_AVR_MEGA)
constexpr char *FIRMWARE_RESPONSE_SUBSCRIBE_TOPIC PROGMEM = "v2/fw/response/#";
constexpr char *FIRMWARE_REQUEST_TOPIC PROGMEM = "v2/fw/request/0/chunk/%u";

// Firmware data keys.
constexpr char *CURRENT_FIRMWARE_TITLE_KEY PROGMEM = "current_fw_title";
constexpr char *CURRENT_FIRMWARE_VERSION_KEY PROGMEM = "current_fw_version";
constexpr char *CURRENT_FIRMWARE_STATE_KEY PROGMEM = "current_fw_state";
constexpr char *FIRMWARE_VERSION_KEY PROGMEM = "fw_version";
constexpr char *FIRMWARE_TITLE_KEY PROGMEM = "fw_title";
constexpr char *FIRMWARE_CHECKSUM_KEY PROGMEM = "fw_checksum";
constexpr char *FIRMWARE_CHECKSUM_ALGO_KEY PROGMEM = "fw_checksum_algorithm";
constexpr char *FIRMWARE_SIZE_KEY PROGMEM = "fw_size";
constexpr char *FIRMWARE_CHECKSUM_VALUE PROGMEM = "MD5";
constexpr char *FIRMWARE_STATE_READY PROGMEM = "READY";
constexpr char *FIRMWARE_STATE_CHECKING PROGMEM = "CHECKING FIRMWARE";
constexpr char *FIRMWARE_STATE_NO_FIRMWARE PROGMEM = "NO FIRMWARE FOUND";
constexpr char *FIRMWARE_STATE_UP_TO_DATE PROGMEM = "UP TO DATE";
constexpr char *FIRMWARE_STATE_INVALID_CHECKSUM PROGMEM = "CHKS IS NOT MD5";
constexpr char *FIRMWARE_STATE_DOWNLOADING PROGMEM = "DOWNLOADING";
constexpr char *FIRMWARE_STATE_FAILED PROGMEM = "FAILED";
constexpr char *FIRMWARE_STATE_UPDATE_ERROR PROGMEM = "UPDATE ERROR";
constexpr char *FIRMWARE_STATE_CHECKSUM_ERROR PROGMEM = "CHECKSUM ERROR";
#endif // defined(ESP8266) || defined(ESP32) || !defined(ARDUINO_AVR_MEGA)

template <
    size_t PayloadSize,
    size_t MaxFieldsElement,
    typename Logger = Logger>
class FirmwareTemplate : public Base
{

public:
  inline FirmwareTemplate(PubSubClient *mqttClient, bool *enableQos, AttributeTemplate<PayloadSize, MaxFieldsElement, Logger> *attribute)
      : Base(mqttClient, enableQos)
  {
    this->attribute = attribute;
  }

  inline bool isFirmwareResponseTopic(const char *const topic)
  {
    return strncmp_P(topic, FIRMWARE_RESPONSE_TOPIC, strlen(FIRMWARE_RESPONSE_TOPIC)) == 0;
  }

#if defined(ESP8266) || defined(ESP32)

  inline void processFirmwareResponseMessage(char *topic, uint8_t *payload, uint32_t length)
  {
    static uint32_t sizeReceive = 0;
    static MD5Builder md5;
    uint16_t chunReceive = atoi(strrchr(topic, SLASH) + 1U);
    if (this->firmwareChunkProcessed > chunReceive + 1)
    {
      return;
    }
    this->firmwareChunkReceive = chunReceive;

    char message[detectSizeOf(FIRMWARE_CHUNK, firmwareChunkReceive, length)];
    snprintf_P(message, sizeof(message), FIRMWARE_CHUNK, this->firmwareChunkReceive, length);
    Logger::log(message);

    if (strncmp_P(this->firmwareState.c_str(), FIRMWARE_STATE_DOWNLOADING, strlen(FIRMWARE_STATE_DOWNLOADING)) != 0)
    {
      this->firmwareState = FIRMWARE_STATE_DOWNLOADING;
    }

    if (this->firmwareChunkReceive == 0)
    {
      sizeReceive = 0;
      md5 = MD5Builder();
      md5.begin();
      if (!Update.begin(this->firmwareSize))
      {
        Logger::log(ERROR_UPDATE_BEGIN);
        Update.printError(Serial);
        this->firmwareState = FIRMWARE_STATE_UPDATE_ERROR;
        return;
      }
    }

    if (Update.write(payload, length) != length)
    {
      Logger::log(ERROR_UPDATE_WITE);
      Update.printError(Serial);
      this->firmwareState = FIRMWARE_STATE_UPDATE_ERROR;
      return;
    }

    md5.add(payload, length);
    sizeReceive += length;
    this->firmwareChunkProcessed++;
    if (this->firmwareSize == sizeReceive)
    {
      md5.calculate();
      String md5Str = md5.toString();
      char actual[JSON_STRING_SIZE(strlen(MD5_ACTUAL)) + md5Str.length()];
      snprintf_P(actual, sizeof(actual), MD5_ACTUAL, md5Str.c_str());
      Logger::log(actual);

      char expected[JSON_STRING_SIZE(strlen(MD5_EXPECTED)) + JSON_STRING_SIZE(strlen(this->firmwareChecksum.c_str()))];
      snprintf_P(expected, sizeof(expected), MD5_EXPECTED, this->firmwareChecksum.c_str());
      Logger::log(expected);

      if (strncmp(md5Str.c_str(), this->firmwareChecksum.c_str(), md5Str.length()) != 0)
      {
        Logger::log(CHECKSUM_VERIFICATION_FAILED);
#if defined(ESP32)
        Update.abort();
#endif
        this->firmwareState = FIRMWARE_STATE_CHECKSUM_ERROR;
        return;
      }
      else
      {
        Logger::log(CHECKSUM_VERIFICATION_SUCCESS);
        if (Update.end())
        {
          Logger::log(FIRMWARE_UPDATE_SUCCESS);
          this->firmwareState = STATUS_SUCCESS;
          return;
        }
      }
    }
  }

  inline const bool startFirmwareUpdate(const char *currFwTitle, const char *currFwVersion, const std::function<void(const bool &)> &updatedCallback)
  {
    if (this->registered == true)
    {
      return this->registered;
    }

    if (currFwTitle == nullptr || currFwVersion == nullptr)
    {
      return false;
    }
    else if (!firmwareSendFirmwareInfo(currFwTitle, currFwVersion))
    {
      return false;
    }

    if (!firmwareSendState(FIRMWARE_STATE_CHECKING))
    {
      return false;
    }

    const std::array<const char *, 5U> fwSharedKeys{FIRMWARE_CHECKSUM_KEY, FIRMWARE_CHECKSUM_ALGO_KEY, FIRMWARE_SIZE_KEY, FIRMWARE_TITLE_KEY, FIRMWARE_VERSION_KEY};

    SharedAttributeCallback sharedReqCallback(fwSharedKeys.cbegin(), fwSharedKeys.cend(), std::bind(&FirmwareTemplate::firmwareSharedAttributeReceived, this, std::placeholders::_1));

    if (!(*attribute).sharedAttributesSubscribe(sharedReqCallback))
    {
      return false;
    }

    // Set private members needed for update.
    this->currentFirmwareTitle = currFwTitle;
    this->currentFirmwareVersion = currFwVersion;
    this->firmwareUpdatedCallbackFunction = updatedCallback;
    this->registered = true;
    if (!firmwareSendState(FIRMWARE_STATE_READY))
    {
      return false;
    }
    return true;
  }

  inline const bool unsubscribeFromOTAFirmware()
  {
    if (!(*mqttClient).unsubscribe(FIRMWARE_RESPONSE_SUBSCRIBE_TOPIC))
    {
      return false;
    }

    return true;
  }

#endif // defined(ESP8266) || defined(ESP32)

private:
#if defined(ESP8266) || defined(ESP32)
  AttributeTemplate<PayloadSize, MaxFieldsElement, Logger> *attribute;
  const char *currentFirmwareTitle;
  String targetFirmwareTitle;
  const char *currentFirmwareVersion;
  String targetFirmwareVersion;
  String firmwareState;
  bool registered = false;
  uint32_t firmwareSize;
  String firmwareChecksum;
  std::function<void(const bool &)> firmwareUpdatedCallbackFunction;
  uint16_t firmwareChunkReceive = std::numeric_limits<int>::max();
  uint16_t firmwareChunkProcessed = 0;

  inline const bool firmwareSendFirmwareInfo(const char *currFwTitle, const char *currFwVersion)
  {
    StaticJsonDocument<JSON_OBJECT_SIZE(2)> currentFirmwareInfo;
    JsonObject currentFirmwareInfoObject = currentFirmwareInfo.to<JsonObject>();
    String title = CURRENT_FIRMWARE_TITLE_KEY;
    String version = CURRENT_FIRMWARE_VERSION_KEY;

    currentFirmwareInfoObject[title.c_str()] = currFwTitle;
    currentFirmwareInfoObject[version.c_str()] = currFwVersion;

    return sendTelemetryJson(currentFirmwareInfoObject);
  }

  inline const bool firmwareSendState(const char *currFwState)
  {
    StaticJsonDocument<JSON_OBJECT_SIZE(1)> currentFirmwareState;
    JsonObject currentFirmwareStateObject = currentFirmwareState.to<JsonObject>();

    String state = CURRENT_FIRMWARE_STATE_KEY;
    currentFirmwareStateObject[state.c_str()] = currFwState;

    return sendTelemetryJson(currentFirmwareStateObject);
  }

  inline const bool firmwareOTASubscribe()
  {
    if (!(*mqttClient).subscribe(FIRMWARE_RESPONSE_SUBSCRIBE_TOPIC, (*mqttQoS) ? 1 : 0))
    {
      return false;
    }

    return true;
  }

  inline void firmwareSharedAttributeReceived(const SharedAttributeData &data)
  {
    if (!data.containsKey(FIRMWARE_VERSION_KEY) || !data.containsKey(FIRMWARE_TITLE_KEY))
    {
      Logger::log(NO_FIRMWARE);
      firmwareSendState(FIRMWARE_STATE_NO_FIRMWARE);
      return;
    }

    this->targetFirmwareTitle = data[FIRMWARE_TITLE_KEY].as<const char *>();
    this->targetFirmwareVersion = data[FIRMWARE_VERSION_KEY].as<const char *>();
    this->firmwareChecksum = data[FIRMWARE_CHECKSUM_KEY].as<const char *>();
    this->firmwareSize = data[FIRMWARE_SIZE_KEY].as<const uint32_t>();
    const char *fw_checksum_algorithm = data[FIRMWARE_CHECKSUM_ALGO_KEY].as<const char *>();

    if (strncmp_P(this->currentFirmwareTitle, this->targetFirmwareTitle.c_str(), strlen(this->currentFirmwareTitle)) == 0 && strncmp_P(this->currentFirmwareVersion, this->targetFirmwareVersion.c_str(), strlen(this->currentFirmwareVersion)) == 0)
    {
      Logger::log(FIRMWARE_UP_TO_DATE);
      firmwareSendState(FIRMWARE_STATE_UP_TO_DATE);
      return;
    }

    if (strncmp_P(this->currentFirmwareTitle, this->targetFirmwareTitle.c_str(), strlen(this->currentFirmwareTitle)) != 0)
    {
      Logger::log(FIRMWARE_NOT_FOR_US);
      firmwareSendState(FIRMWARE_STATE_NO_FIRMWARE);
      return;
    }

    if (strncmp_P(fw_checksum_algorithm, FIRMWARE_CHECKSUM_VALUE, strlen(FIRMWARE_CHECKSUM_VALUE)) != 0)
    {
      Logger::log(FIRMWARE_CHECKSUM_ALGO_NOT_SUPPORTED);
      firmwareSendState(FIRMWARE_STATE_INVALID_CHECKSUM);
      return;
    }

    firmwareOTASubscribe();

    Logger::log(PAGE_BREAK);
    Logger::log(NEW_FIRMWARE);
    char firmware[JSON_STRING_SIZE(strlen(FROM_TOO)) + JSON_STRING_SIZE(strlen(this->currentFirmwareVersion)) + JSON_STRING_SIZE(strlen(this->targetFirmwareVersion.c_str()))];
    snprintf_P(firmware, sizeof(firmware), FROM_TOO, this->currentFirmwareVersion, this->targetFirmwareVersion.c_str());
    Logger::log(firmware);
    Logger::log(DOWNLOADING_FIRMWARE);

    const uint16_t chunkSize = 4096U; // maybe less if we don't have enough RAM
    const uint16_t numberOfChunk = static_cast<uint16_t>(this->firmwareSize / chunkSize) + 1U;
    this->firmwareChunkReceive = std::numeric_limits<int>::max();
    uint16_t currChunk = 0U;
    uint8_t nbRetry = 5U;

    const uint16_t previousBufferSize = (*mqttClient).getBufferSize();
    const bool changeBufferSize = previousBufferSize < (chunkSize + 50U);

    // Increase size of receive buffer
    if (changeBufferSize && !(*mqttClient).setBufferSize(chunkSize + 50U))
    {
      Logger::log(NOT_ENOUGH_RAM);
      return;
    }

    firmwareSendState(FIRMWARE_STATE_DOWNLOADING);
    this->firmwareState = FIRMWARE_STATE_DOWNLOADING;
    char size[detectSizeOf(NUMBER_PRINTF, chunkSize)];
    do
    {
      char topic[detectSizeOf(FIRMWARE_REQUEST_TOPIC, currChunk)]; // Size adjuts dynamically to the current length of the currChunk number to ensure we don't cut it out of the topic string.
      snprintf_P(topic, sizeof(topic), FIRMWARE_REQUEST_TOPIC, currChunk);
      snprintf_P(size, sizeof(size), NUMBER_PRINTF, chunkSize);

      (*mqttClient).publish(topic, size, (*mqttQoS));

      const uint64_t timeout = millis() + 3000U; // Amount of time we wait until we declare the download as failed in milliseconds.
      do
      {
        delay(5);
        // loop();
        (*mqttClient).loop();

      } while ((this->firmwareChunkReceive != currChunk) && (timeout >= millis()));

      if (this->firmwareChunkReceive == currChunk)
      {

        if (numberOfChunk != (currChunk + 1))
        {
          // Check if state is still DOWNLOADING and did not fail.
          if (strncmp_P(this->firmwareState.c_str(), FIRMWARE_STATE_DOWNLOADING, strlen(FIRMWARE_STATE_DOWNLOADING)) == 0)
          {
            currChunk++;
          }
          else
          {
            nbRetry--;
            if (nbRetry == 0)
            {
              Logger::log(UNABLE_TO_WRITE);
              break;
            }
          }
        }
        // The last chunk
        else
        {
          currChunk++;
        }
      }
      // Timeout
      else
      {
        nbRetry--;
        if (nbRetry == 0)
        {
          Logger::log(UNABLE_TO_DOWNLOAD);
          break;
        }
      }
    } while (numberOfChunk != currChunk);

    // Buffer size has been set to another value by the method return to the previous value.
    if (changeBufferSize)
    {
      (*mqttClient).setBufferSize(previousBufferSize);
    }
    // Unsubscribe from now not needed topics anymore.
    unsubscribeFromOTAFirmware();
    bool success = false;
    // Update current_fw_title and current_fw_version if updating was a success.
    if (strncmp_P(this->firmwareState.c_str(), STATUS_SUCCESS, strlen(STATUS_SUCCESS)) == 0)
    {
      this->currentFirmwareTitle = this->targetFirmwareTitle.c_str();
      this->currentFirmwareVersion = this->targetFirmwareVersion.c_str();
      firmwareSendFirmwareInfo(this->currentFirmwareTitle, this->currentFirmwareVersion);
      firmwareSendState(STATUS_SUCCESS);
      success = true;
    }
    else
    {
      firmwareSendState(FIRMWARE_STATE_FAILED);
    }

    if (this->firmwareUpdatedCallbackFunction != nullptr)
    {
      this->firmwareUpdatedCallbackFunction(success);
    }
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

#endif // defined(ESP8266) || defined(ESP32)
};

#endif // FIRMWARE_H