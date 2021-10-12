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
#include <sps30.h>
#include "main.h"
#include "networking.h"
#include "button.h"
#include "dispMeas.h"

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

int16_t spsRet;


DynamicJsonDocument mqttMsgJson(1024);
char mqttMsg[1024];

unsigned long lastSampleTime = 0;
unsigned long lastPressed = 0;
unsigned long lastScrolled = 0;

datum_t datum [] = {
    {"CO2", "CO2", "PPM", 0},
    {"Temp", "TEMP", "°C", 1},
    {"Humi", "HUMI", "%RH", 1},
    {"PM10", "PM 1.0", "ug/m3", 1},
    {"PM25", "PM 2.5", "ug/m3", 1},
    {"PM100", "PM 10", "ug/m3", 1}
};

// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI_PIN, OLED_CLK_PIN, OLED_DC_PIN, OLED_RESET_PIN, OLED_CS_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC_PIN, OLED_RESET_PIN, OLED_CS_PIN, 8000000);

void setup() {

    Serial.begin(9600);
    Serial.println();

    display.begin(SSD1306_SWITCHCAPVCC, 0, true, true);
    display.clearDisplay();
    display.display(); 

    display.setFont(&FreeSansBold18pt7b);
    // display.setTextSize(3); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(28, 25);
    display.println("pico");
    display.setCursor(37, 55);
    display.println("AIR");
    display.display();      // Show initial text
    display.setFont();

    Wire.begin(SDA_PIN, SCL_PIN);    

    scd4x.begin(Wire);
    
    while (sps30_probe() != 0) {
        Serial.print("SPS sensor probing failed\n");
        delay(500);
    }

    Serial.print("SPS sensor probing successful\n");

    spsRet = sps30_set_fan_auto_cleaning_interval_days(SPS_AUTOCLEAN_DAYS);
    if (spsRet) {
        Serial.print("error setting the auto-clean interval: ");
        Serial.println(spsRet);
    }

    spsRet = sps30_start_measurement();
    if (spsRet < 0) {
        Serial.print("error starting measurement\n");
    }

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

    WiFiconnect();
    MQTTconnect();

    delay(1000);

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(1, 1);
    display.println("Warming up sensors");
    display.display();

    unsigned long now = millis();

    while (now - lastSampleTime < SENSOR_SAMPLING_PERIOD) {
        now = millis();
    }

    lastSampleTime = now;
    lastScrolled = now;
    lastPressed = now;
}

void loop() {
    
    static unsigned long long buttonHist = 0;
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

        static int count = 0;

        buttonHist = buttonHist << 1 | (digitalRead(BUTTON_PIN) & 0x1);
        // Serial.println(buttonHist, BIN);
        if (buttonIsHeld(buttonHist)) {
            Serial.println("Button is held");
            lastPressed = millis();
            buttonHist = 0;
        } else {           
            if (buttonIsShortPressed(buttonHist)) {
                Serial.println("Button is short pressed");
                lastPressed = millis();
                count++;
            }            
        }
        if (millis() - lastPressed > IDLE_PERIOD) {
            // we're idle
            if (millis() - lastScrolled > SCROLL_PERIOD) {
                count++;
                lastScrolled = millis();
            }
        }
        if (count >= sizeof(datum) / sizeof(datum_t)) {
            count = 0;
        }
        if ((int) mqttMsgJson["CO2"] != 0) {
            dispItem(mqttMsgJson, datum[count]);
        }
    }

    now = millis();
    if (now - lastSampleTime > SENSOR_SAMPLING_PERIOD) {
        lastSampleTime = now;

        // Read SCD41
        uint16_t co2;
        float temperature;
        float humidity;
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
        }

        // Read SPS30
        struct sps30_measurement spsMeas;
        uint16_t spsDataReady;
        int16_t spsRet;
        spsRet = sps30_read_data_ready(&spsDataReady);
        if (spsRet < 0) {
            Serial.print("error reading data-ready flag: ");
            Serial.println(spsRet);
        } else if (spsDataReady) {
            spsRet = sps30_read_measurement(&spsMeas);
            if (spsRet >= 0) {
                mqttMsgJson["PM10"] = spsMeas.mc_1p0;
                mqttMsgJson["PM25"] = spsMeas.mc_2p5;
                mqttMsgJson["PM40"] = spsMeas.mc_4p0;  
                mqttMsgJson["PM100"] = spsMeas.mc_10p0;              
            } else {
                Serial.println("error reading SPS measurement");
            }
        }   
        
        serializeJsonPretty(mqttMsgJson, mqttMsg);
        Serial.println(mqttMsg);

        client.publish("CO2/alexBedroom", mqttMsg);

    }
}