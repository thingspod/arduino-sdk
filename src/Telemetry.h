#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include <ArduinoJson.h>

constexpr char *TELEMETRY_TOPIC PROGMEM = "v1/devices/me/telemetry";

class Telemetry
{
	
public:
	inline Telemetry() : type(NONE), key(NULL), value() {}

	template <typename T, typename = ARDUINOJSON_NAMESPACE::enable_if<ARDUINOJSON_NAMESPACE::is_integral<T>::value>>
	inline Telemetry(const char *key, T val) : type(INT), key(key), value()
	{
		value.integer = val;
	}

	inline Telemetry(const char *key, bool val)
		: type(BOOL), key(key), value()
	{
		value.boolean = val;
	}

	inline Telemetry(const char *key, float val)
		: type(REAL), key(key), value()
	{
		value.real = val;
	}

	inline Telemetry(const char *key, const char *val)
		: type(STRING), key(key), value()
	{
		value.string = val;
	}

	const bool serializeKeyValue(JsonVariant &jsonObj) const;

// private:
	union data
	{
		const char *string;
		bool boolean;
		int integer;
		float real;
	};

	enum dataType
	{
		NONE,
		BOOL,
		INT,
		REAL,
		STRING,
	};

	dataType type;
	const char *key;
	data value;
};

#endif // TELEMETRY_H