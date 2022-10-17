
#include "Telemetry.h"

const bool Telemetry::serializeKeyValue(JsonVariant &jsonObj) const {
  if (key) {
    switch (type) {
      case BOOL:
        jsonObj[key] = value.boolean;
      break;
      case INT:
        jsonObj[key] = value.integer;
      break;
      case REAL:
        jsonObj[key] = value.real;
      break;
      case STRING:
        jsonObj[key] = value.string;
      break;
      default:
      break;
    }
  } else {
    switch (type) {
      case BOOL:
        return jsonObj.set(value.boolean);
      break;
      case INT:
        return jsonObj.set(value.integer);
      break;
      case REAL:
        return jsonObj.set(value.real);
      break;
      case STRING:
        return jsonObj.set(value.string);
      break;
      default:
      break;
    }
  }
  return true;
}