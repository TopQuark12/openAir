#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include "main.h"
#include "dispMeas.h"

void dispItem (DynamicJsonDocument msgJson, datum_t datum) {
    int16_t  x1, y1, x2, y2;
    uint16_t w1, h1, w2, h2;
    display.clearDisplay();    

    display.setFont(&FreeSansBold9pt7b);
    display.getTextBounds(datum.dispName, 0, 0, &x1, &y1, &w1, &h1);
    display.setCursor(64 - w1 / 2, h1 + 1);
    display.print(datum.dispName);

    display.setFont(&FreeSansBold18pt7b);
    display.getTextBounds(String( (float) msgJson[datum.jsonTag], datum.decimalPlaces), 0, 0, &x2, &y2, &w2, &h2);
    display.setCursor(64 - w2 / 2, 32 + h2 / 2);
    display.print(String( (float) msgJson[datum.jsonTag], datum.decimalPlaces));

    display.setFont(&FreeSansBold9pt7b);
    display.getTextBounds(datum.unit, 0, 0, &x1, &y1, &w1, &h1);
    display.setCursor(64 - w1 / 2, 63);
    display.print(datum.unit);

    display.display(); 
}