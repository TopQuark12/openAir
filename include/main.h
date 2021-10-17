#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_SGP40.h>
#include <PubSubClient.h>

#define WIFI_CONNECTION_TIMEOUT     20000
#define CO2_SENSOR_SAMPLING_PERIOD  5000   // Must be greater than 5000
#define VOC_SENSOR_SAMPLING_PERIOD  1000
#define BUTTON_SAMPLING_PERIOD      50
#define IDLE_PERIOD                 15000
#define SCROLL_PERIOD               5000
#define SCREEN_OFF_DELAY_PERIOD     900000
#define ESP_SLEEP_PERIOD            60      // s

#define SPS_AUTOCLEAN_DAYS          4

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
#define BATT_SENSE_PIN              35
#define VBUS_SENSE_PIN              34
#define BOOST_EN_PIN                32
#define LED_PIN                     19
#define GPIO_WAKEUP_SRC_BATT        0x400000010    
#define GPIO_WAKEUP_SRC_PLUGGED     0x10

#define ADC_TO_VOLTS(val)           (val / 564.0F)     

extern Adafruit_SSD1306 display;
extern PubSubClient client;
extern SensirionI2CScd4x scd4x;
extern Adafruit_SGP40 sgp;
// extern int bootCount;

