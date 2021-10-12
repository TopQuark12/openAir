#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>

#define WIFI_CONNECTION_TIMEOUT     20000
#define SENSOR_SAMPLING_PERIOD      5000   // Must be greater than 5000
#define BUTTON_SAMPLING_PERIOD      100

#define SCREEN_WIDTH                128 // OLED display width, in pixels
#define SCREEN_HEIGHT               64 // OLED display height, in pixels

#define SDA_PIN                     21
#define SCL_PIN                     22
#define OLED_MOSI_PIN               23
#define OLED_CLK_PIN                18
#define OLED_DC_PIN                 16
#define OLED_CS_PIN                 5
#define OLED_RESET_PIN              17
#define BUTTON_PIN                  4

extern Adafruit_SSD1306 display;
extern PubSubClient client;
