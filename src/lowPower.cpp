#include <Arduino.h>
#include <sps30.h>
#include <Adafruit_SGP40.h>
#include <SensirionI2CScd4x.h>
#include "main.h"

void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch(wakeup_reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
        default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
    }
}

void gotoSleep() {

    bool sgpSleep = sgp.heaterOff();
    if (sgpSleep == true)
        Serial.println("SGP Sleep Success");
    else
        Serial.println("SGP Sleep Fail");

    scd4x.stopPeriodicMeasurement();
    delay(500);
    int scdSleep = scd4x.powerDown();
    if (scdSleep)
        Serial.println("SCD Sleep Fail");
    else
        Serial.println("SCD Sleep Success");

    int spsSleep = 0;
    spsSleep = spsSleep | sps30_stop_measurement();
    spsSleep = spsSleep | sps30_sleep();
    if (spsSleep)
        Serial.println("SPS Sleep Fail");
    else
        Serial.println("SPS Sleep Success");

    display.clearDisplay();
    display.display();

    Serial.println("ESP Deep Sleep");

    esp_sleep_enable_timer_wakeup(ESP_SLEEP_PERIOD * 1000000);
    esp_deep_sleep_start();

}
