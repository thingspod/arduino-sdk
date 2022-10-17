#ifndef CLAIMING_H
#define CLAIMING_H

#include <Arduino.h>

#if defined(ESP8266) || defined(ESP32) || defined(ARDUINO_AVR_MEGA)
constexpr char *CLAIM_TOPIC  PROGMEM = "v1/devices/me/claim";

constexpr char *SECRET_KEY  PROGMEM = "secretKey";
constexpr char *DURATION_KEY  PROGMEM = "durationMs";
constexpr char *DEVICE_NAME_KEY  PROGMEM = "deviceName";


#endif
#endif