#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "cred.h"

#define WIFI_CONNECTION_TIMEOUT     10000
#define SENSOR_SAMPLING_PERIOD      20000   // Must be greater than 5000
#define SDA_PIN                     21
#define SCL_PIN                     22

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

void WiFiconnect() {
    Serial.print("Connecting to ");
    Serial.println(SSID);
    WiFi.begin(SSID, PASSWORD);
    unsigned long timeStart = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - timeStart < WIFI_CONNECTION_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {        
        Serial.println("WiFi connected");
        Serial.print("IP address : ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Wifi connection timeout");
    }
}

void MQTTconnect() {
    Serial.print("Attempting MQTT connection...");
        if (client.connect(macAddr)) {
        Serial.println("connected");
    } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
    }
}

void setup() {
    
    Serial.begin(9600);
    Serial.println();

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

        // Read Measurement
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

        serializeJsonPretty(mqttMsgJson, mqttMsg);
        Serial.println(mqttMsg);

        client.publish("CO2/alexBedroom", mqttMsg);

    }
}