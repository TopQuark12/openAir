#include <Arduino.h>
#include <WiFi.h>
#include "cred.h"
#include "main.h"

char macAddr [18];

void getMacAddr() {

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

}

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
        Serial.println("Wifi 1 connection timeout");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("WiFi connection 1");
        display.println("timeout");

        display.println("Connecting to");
        // display.setCursor(0, 8);
        display.println(SSID2);
        display.display();

        Serial.print("Connecting to ");
        Serial.println(SSID2);
        WiFi.begin(SSID2, PASSWORD2);
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

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Wifi 2 connection timeout");
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("WiFi connection 2");
            display.println("timeout");
        }        
    }
    delay(1000);
}

void MQTTconnect() {

    client.setServer(MQTT_SERVER, 1883);
    
    Serial.print("Attempting MQTT connection...");

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Connecting to");
    // display.setCursor(0, 8);
    display.println("MQTT server at : ");
    display.println(MQTT_SERVER);
    display.display();

    if (macAddr[0] == 0) {
        getMacAddr();
    }

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