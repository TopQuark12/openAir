#pragma once
#include <string.h>
#include <ArduinoJson.h>

typedef struct datum_t {
    String jsonTag;
    String dispName;
    String unit;
    int decimalPlaces;
} datum_t ;

void dispItem (DynamicJsonDocument msgJson, datum_t datum);
