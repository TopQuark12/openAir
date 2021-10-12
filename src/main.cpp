#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "cred.h"

#define WIFI_CONNECTION_TIMEOUT     10000
#define SENSOR_SAMPLING_PERIOD      20000   // Must be greater than 5000

#define SCREEN_WIDTH                128 // OLED display width, in pixels
#define SCREEN_HEIGHT               64 // OLED display height, in pixels

#define SDA_PIN                     21
#define SCL_PIN                     22
#define OLED_MOSI                   23
#define OLED_CLK                    18
#define OLED_DC                     16
#define OLED_CS                     5
#define OLED_RESET                  17

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
char macAddr [18];

uint16_t SCDerror;
char SCDerrorMessage[256];

DynamicJsonDocument mqttMsgJson(1024);
char mqttMsg[1024];

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

void WiFiconnect() {

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Connecting to");
    // display.setCursor(0, 8);
    display.println(SSID);
    display.display();

    Serial.print("Connecting to ");
    Serial.println(SSID);
    WiFi.begin(SSID, PASSWORD);
    unsigned long timeStart = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - timeStart < WIFI_CONNECTION_TIMEOUT) {
        delay(500);
        Serial.print(".");
        // display.setCursor(0, 16);
        display.print(".");
        display.display();
    }
    Serial.println();
    display.println();

    if (WiFi.status() == WL_CONNECTED) {        
        Serial.println("WiFi connected");
        Serial.print("IP address : ");
        Serial.println(WiFi.localIP());

        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("WiFi connected");
        display.println("IP address : ");
        display.println(WiFi.localIP());
        display.display();
    } else {
        Serial.println("Wifi connection timeout");

        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("WiFi connection");
        display.println("timeout");
        display.display();
    }
    delay(1000);
}

void MQTTconnect() {
    Serial.print("Attempting MQTT connection...");

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Connecting to");
    // display.setCursor(0, 8);
    display.println("MQTT server at : ");
    display.println(MQTT_SERVER);
    display.display();

    if (client.connect(macAddr, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println("connected");

        display.println();
        display.println("Successful");
        display.display();
    } else {
        display.println();
        Serial.print("failed, rc=");
        Serial.print(client.state());

        display.print("Fail : ");
        display.println(client.state());
        display.display();
    }
    delay(1000);
}

void setup() {

    Serial.begin(9600);
    Serial.println();

    display.begin(SSD1306_SWITCHCAPVCC, 0, true, true);
    display.clearDisplay();
    display.display(); 

    display.setTextSize(3); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(28, 8);
    display.println("pico");
    display.setCursor(37, 32);
    display.println("AIR");
    display.display();      // Show initial text

    delay(2000);
    display.clearDisplay();
    display.display();

    WiFiconnect();  

    byte mac[6];  
    String macStr;
    WiFi.macAddress(mac);    

    macStr.concat(String(mac[5], HEX));
    macStr.concat(':');
    macStr.concat(String(mac[4], HEX));
    macStr.concat(':');
    macStr.concat(String(mac[3], HEX));
    macStr.concat(':');
    macStr.concat(String(mac[2], HEX));
    macStr.concat(':');
    macStr.concat(String(mac[1], HEX));
    macStr.concat(':');
    macStr.concat(String(mac[0], HEX));
    macStr.toCharArray(macAddr, 18);
    Serial.print("Mac address : ");
    Serial.println(macAddr);

    client.setServer(MQTT_SERVER, 1883);
    MQTTconnect();

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

    Serial.println("Waiting for first measurement...");
    delay(5000);
}


long lastMsg = 0;
void loop() {
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to wifi...");
        WiFiconnect();
    }

    if (!client.connected()) {
        Serial.println("Reconnecting to MQTT server...");
        MQTTconnect();
    }
    client.loop();

    long now = millis();
    if (now - lastMsg > SENSOR_SAMPLING_PERIOD) {
        lastMsg = now;

        // Read SCD41
        uint16_t co2, co2Valid;
        float temperature, temperatureValid;
        float humidity, humidityValid;
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