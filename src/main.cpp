#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include "main.h"
#include "networking.h"

#ifndef SSID
    #define SSID            "WIFI SSID HERE"
#endif
#ifndef PASSWORD        
    #define PASSWORD        "WIFI PASSWORD HERE"
#endif
#ifndef MQTT_SERVER
    #define MQTT_SERVER     "MQTT SERVER ADDRESS HERE"
#endif

SensirionI2CScd4x scd4x;

WiFiClient espClient;
PubSubClient client(espClient);

uint16_t SCDerror;
char SCDerrorMessage[256];

DynamicJsonDocument mqttMsgJson(1024);
char mqttMsg[1024];

unsigned long lastSampleTime = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI_PIN, OLED_CLK_PIN, OLED_DC_PIN, OLED_RESET_PIN, OLED_CS_PIN);

void setup() {

    Serial.begin(9600);
    Serial.println();

    display.begin(SSD1306_SWITCHCAPVCC, 0, true, true);
    display.clearDisplay();
    display.display(); 

    display.setFont(&FreeSansBold18pt7b);
    // display.setTextSize(3); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(28, 24);
    display.println("pico");
    display.setCursor(37, 56);
    display.println("AIR");
    display.display();      // Show initial text
    display.setFont();

    Wire.begin(SDA_PIN, SCL_PIN);    

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    SCDerror = scd4x.stopPeriodicMeasurement();
    if (SCDerror) {
        Serial.print("SCDError trying to execute stopPeriodicMeasurement(): ");
        errorToString(SCDerror, SCDerrorMessage, 256);
        Serial.println(SCDerrorMessage);
    }

    // Start Measurement
    SCDerror = scd4x.startPeriodicMeasurement();
    if (SCDerror) {
        Serial.print("SCDError trying to execute startPeriodicMeasurement(): ");
        errorToString(SCDerror, SCDerrorMessage, 256);
        Serial.println(SCDerrorMessage);
    }

    lastSampleTime = millis();

    pinMode(BUTTON_PIN, INPUT);

    delay(2000);
    display.clearDisplay();
    display.display();

    WiFiconnect();
    MQTTconnect();

    delay(2000);

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Warming up sensors");
    display.display();

    unsigned long now = millis();
    lastSampleTime = millis();

    while (now - lastSampleTime < SENSOR_SAMPLING_PERIOD) {
        now = millis();
    }

    lastSampleTime = now;
}

void loop() {
    
    static unsigned long buttonHist = 0;
    static unsigned long lastButtonSampleTime = 0;
    static unsigned long now = 0;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to wifi...");
        WiFiconnect();
    }

    if (!client.connected()) {
        Serial.println("Reconnecting to MQTT server...");
        MQTTconnect();
    }
    client.loop();

    now = millis();
    if (now - lastButtonSampleTime > BUTTON_SAMPLING_PERIOD) {
        lastButtonSampleTime = now;

        buttonHist = buttonHist << 1 | (digitalRead(BUTTON_PIN) & 0x1);
        Serial.println(buttonHist, BIN);
    }

    now = millis();
    if (now - lastSampleTime > SENSOR_SAMPLING_PERIOD) {
        lastSampleTime = now;

        // Read SCD41
        uint16_t co2, co2Valid = 0;
        float temperature, temperatureValid = 0;
        float humidity, humidityValid = 0;
        SCDerror = scd4x.readMeasurement(co2, temperature, humidity);
        if (SCDerror) {
            Serial.print("SCDError trying to execute readMeasurement(): ");
            errorToString(SCDerror, SCDerrorMessage, 256);
            Serial.println(SCDerrorMessage);
        } else if (co2 == 0) {
            Serial.println("Invalid sample detected, skipping.");
        } else {
            mqttMsgJson["CO2"] = co2;
            mqttMsgJson["Temp"] = temperature;
            mqttMsgJson["Humi"] = humidity;
            co2Valid = co2;
            temperatureValid = temperature;
            humidityValid = humidity;
        }

        serializeJsonPretty(mqttMsgJson, mqttMsg);
        Serial.println(mqttMsg);

        client.publish("CO2/alexBedroom", mqttMsg);

        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("CO2  : ");
        display.print(co2Valid);
        display.println(" ppm");
        display.println();
        display.print("Temp : ");
        display.print(temperatureValid);
        display.println(" C");
        display.println();
        display.print("Humi : ");
        display.print(humidityValid);
        display.println(" %");
        display.display();

    }
}