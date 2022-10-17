#ifndef LOGGER_H
#define LOGGER_H

#include "Arduino.h"
#include "HardwareSerial.h"

constexpr char *INVALID_BUFFER_SIZE PROGMEM = "PayloadSize (%u) to small for the given payloads size (%u)";
constexpr char *CALLBACK_FUNCTION_CALLED_MESSAGE PROGMEM = "Callback onMessage from topic: (%s)";
constexpr char *UNABLE_TO_DE_SERIALIZE_ATTRIBUTE_REQUEST PROGMEM = "Unable to de-serialize shared attribute request";
constexpr char *RECEIVED_RPC_LOG_MESSAGE PROGMEM = "Received RPC:";
constexpr char *RPC_METHOD_NULL PROGMEM = "RPC method is NULL";
constexpr char *RPC_CALLBACK_NULL PROGMEM = "RPC callback or subscribed rpc method name is NULL";
constexpr char *NO_RPC_PARAMS_PASSED PROGMEM = "No parameters passed with RPC, passing null JSON";
constexpr char *CALLING_RPC PROGMEM = "Calling RPC:";
constexpr char *UNABLE_TO_SERIALIZE PROGMEM = "Unable to serialize data";
constexpr char *CONNECT_FAILED PROGMEM = "Connecting to server failed";
constexpr char *MAX_RPC_EXCEEDED PROGMEM = "Too many rpc subscriptions, increase MaxFieldsAmt or unsubscribe";
constexpr char *MAX_SHARED_ATTRIBUTE_UPDATE_EXCEEDED PROGMEM = "Too many shared attribute update callback subscriptions, increase MaxFieldsAmt or unsubscribe";
constexpr char *MAX_SHARED_ATTRIBUTE_REQUEST_EXCEEDED PROGMEM = "Too many shared attribute request callback subscriptions, increase MaxFieldsAmt";
constexpr char *NUMBER_PRINTF PROGMEM = "%u";
constexpr char COMMA PROGMEM = ',';
constexpr char *NO_KEYS_TO_REQUEST PROGMEM = "No keys to request were given";
constexpr char *REQUEST_ATTRIBUTE PROGMEM = "Requesting shared attributes transformed from (%s) into json (%s)";
constexpr char *UNABLE_TO_DE_SERIALIZE_RPC PROGMEM = "Unable to de-serialize RPC";
constexpr char *UNABLE_TO_DE_SERIALIZE_ATTRIBUTE_UPDATE PROGMEM = "Unable to de-serialize shared attribute update";
constexpr char *RECEIVED_ATTRIBUTE_UPDATE PROGMEM = "Received shared attribute update";
constexpr char *NOT_FOUND_ATTRIBUTE_UPDATE PROGMEM = "Shared attribute update key not found";
constexpr char *ATTRIBUTE_CALLBACK_ID PROGMEM = "Shared attribute update callback id: (%u)";
constexpr char *ATTRIBUTE_CALLBACK_IS_NULL PROGMEM = "Shared attribute update callback is NULL";
constexpr char *ATTRIBUTE_CALLBACK_NO_KEYS PROGMEM = "No keys subscribed. Calling subscribed callback for any updated attributes (assumed to be subscribed to every possible key)";
constexpr char *ATTRIBUTE_IS_NULL PROGMEM = "Subscribed shared attribute update key is NULL";
constexpr char *ATTRIBUTE_IN_ARRAY PROGMEM = "Shared attribute update key: (%s) is subscribed";
constexpr char *ATTRIBUTE_NO_CHANGE PROGMEM = "No keys that we subscribed too were changed, skipping callback";
constexpr char *CALLING_ATTRIBUTE_CALLBACK PROGMEM = "Calling subscribed callback for updated shared attribute (%s)";
constexpr char *RECEIVED_ATTRIBUTE PROGMEM = "Received shared attribute request";
constexpr char *ATTRIBUTE_KEY_NOT_FOUND PROGMEM = "Shared attribute key not found";
constexpr char *ATTRIBUTE_REQUEST_CALLBACK_IS_NULL PROGMEM = "Shared attribute request callback is NULL";
constexpr char *CALLING_REQUEST_ATTRIBUTE_CALLBACK PROGMEM = "Calling subscribed callback for response id (%u)";
constexpr char *TOO_MANY_JSON_FIELDS PROGMEM = "Too many JSON fields passed (%u), increase MaxFieldsAmt (%u) accordingly";
constexpr char CALLBACK_ON_MESSAGE[] PROGMEM = "Callback on_message from topic: (%s)";

#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_AVR_MEGA)
constexpr char *PROVISION_REQUEST PROGMEM = "Provision request:";
constexpr char *UNABLE_TO_DE_SERIALIZE_PROVISION_RESPONSE PROGMEM = "Unable to de-serialize provision response";
constexpr char *PROVISION_RESPONSE PROGMEM = "Process provisioning response";
constexpr char *RECEIVED_PROVISION_RESPONSE PROGMEM = "Received provision response";
constexpr char *X509_NOT_SUPPORTED PROGMEM = "Provision response contains X509_CERTIFICATE credentials, this is not supported yet";

#if !defined(ARDUINO_AVR_MEGA)
constexpr char *NO_FIRMWARE PROGMEM = "No new firmware assigned on the given device";
constexpr char *FIRMWARE_UP_TO_DATE PROGMEM = "Firmware is already up to date";
constexpr char *FIRMWARE_NOT_FOR_US PROGMEM = "Firmware is not for us (title is different)";
constexpr char *FIRMWARE_CHECKSUM_ALGO_NOT_SUPPORTED PROGMEM = "Checksum algorithm is not supported, please use MD5 only";
constexpr char *PAGE_BREAK = "=================================";
constexpr char *NEW_FIRMWARE PROGMEM = "A new Firmware is available:";
constexpr char *FROM_TOO = "(%s) => (%s)";
constexpr char *DOWNLOADING_FIRMWARE PROGMEM = "Attempting to download over MQTT...";
constexpr char *NOT_ENOUGH_RAM PROGMEM = "Not enough RAM";
constexpr char SLASH PROGMEM = '/';
constexpr char *UNABLE_TO_WRITE PROGMEM = "Unable to write firmware";
constexpr char *UNABLE_TO_DOWNLOAD PROGMEM = "Unable to download firmware";
constexpr char *FIRMWARE_CHUNK PROGMEM = "Receive chunk (%i), with size (%u) bytes";
constexpr char *ERROR_UPDATE_BEGIN PROGMEM = "Error during Update.begin";
constexpr char *ERROR_UPDATE_WITE PROGMEM = "Error during Update.write";
constexpr char *MD5_ACTUAL PROGMEM = "MD5 actual checksum: (%s)";
constexpr char *MD5_EXPECTED PROGMEM = "MD5 expected checksum: (%s)";
constexpr char *CHECKSUM_VERIFICATION_FAILED PROGMEM = "Checksum verification failed";
constexpr char *CHECKSUM_VERIFICATION_SUCCESS PROGMEM = "Checksum is the same as expected";
constexpr char *FIRMWARE_UPDATE_SUCCESS PROGMEM = "Update success";
#endif // !defined(ARDUINO_AVR_MEGA)

#endif // defined(ESP8266) || defined(ESP32) || defined(ARDUINO_AVR_MEGA)

class Logger
{
public:
    static void log(const char *msg)
    {
        Serial.print(F("[Thingspod] "));
        Serial.println(msg);
    }
};

#endif // LOGGER_H
