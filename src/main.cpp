#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "cred.h"

#define SDA_PIN             21
#define SCL_PIN             22


char ssid[] = SSID;
char password[] = PASSWORD;

const char* mqtt_server = "alexwong.duckdns.org";

SensirionI2CScd4x scd4x;

WiFiClient espClient;
PubSubClient client(espClient);

void printUint16Hex(uint16_t value) {
    Serial.print(value < 4096 ? "0" : "");
    Serial.print(value < 256 ? "0" : "");
    Serial.print(value < 16 ? "0" : "");
    Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    Serial.print("Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    Serial.println();
}

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP8266Client")) {
        Serial.println("connected");
        // Subscribe
        client.subscribe("esp32/output");
        } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
        }
    }
}

void setup() {

    Serial.begin(9600);
    while (!Serial) {
        delay(100);
    }

    Wire.begin(SDA_PIN, SCL_PIN);

    uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        printSerialNumber(serial0, serial1, serial2);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    Serial.println("Waiting for first measurement... (5 sec)");

    setup_wifi();
    client.setServer(mqtt_server, 1883);
}


long lastMsg = 0;
void loop() {
    uint16_t error;
    char errorMessage[256];

    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    long now = millis();
    if (now - lastMsg > 5000) {
        lastMsg = now;

        // Read Measurement
        uint16_t co2;
        float temperature;
        float humidity;
        error = scd4x.readMeasurement(co2, temperature, humidity);
        if (error) {
            Serial.print("Error trying to execute readMeasurement(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
        } else if (co2 == 0) {
            Serial.println("Invalid sample detected, skipping.");
        } else {
            String jsonStr;
            jsonStr.concat("{\n");

            jsonStr.concat("\t\"CO2\":");
            jsonStr.concat(co2);
            jsonStr.concat(",\n");

            jsonStr.concat("\t\"Temp\":");
            jsonStr.concat(temperature);
            jsonStr.concat(",\n");

            jsonStr.concat("\t\"Humi\":");
            jsonStr.concat(humidity);

            jsonStr.concat("\n}");

            Serial.println(jsonStr);
            
            char msg[100];
            jsonStr.toCharArray(msg, 100);
            client.publish("CO2/alexBedroom", msg);
        }
    }
}