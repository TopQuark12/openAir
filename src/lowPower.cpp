#include <Arduino.h>
#include <sps30.h>
#include <Adafruit_SGP40.h>
#include <SensirionI2CScd4x.h>
#include "main.h"

RTC_DATA_ATTR VocAlgorithmParams voc_param_backup;

bool isPluggedIn() {
    if (ADC_TO_VOLTS(analogRead(VBUS_SENSE_PIN)) > 2.5) {
        return true;
    } else {
        return false;
    }
}

bool batteryOK () {
    if (ADC_TO_VOLTS(analogRead(BATT_SENSE_PIN)) > 3.2) {
        return true;
    } else {
        return false;
    }
}

void restoreVoc() {

    if(voc_param_backup.mUptime != 0) {
        Serial.println("Restoring SGP...");
        memcpy(&sgp.voc_algorithm_params, &voc_param_backup, sizeof(VocAlgorithmParams));
    } else {
        Serial.println("Warming up SGP...");
        delay(5000);

        uint16_t co2;
        float temperature;
        float humidity;
        uint16_t SCDerror = scd4x.readMeasurement(co2, temperature, humidity);
        if (SCDerror) {
            Serial.print("SCDError trying to execute readMeasurement(): ");
        } else if (co2 == 0) {
            Serial.println("Invalid sample detected, skipping.");
        } else {
            for (int i = 0; i < 46; i++) {
                sgp.measureVocIndex(temperature, humidity);
                Serial.print('.');
                delay(1000);
            }
            Serial.println();
        }
    }

}

esp_sleep_wakeup_cause_t print_wakeup_reason() {
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

    return wakeup_reason;
}

void gotoSleep() {

    if (sgp.voc_algorithm_params.mUptime > 0) {
        memcpy(&voc_param_backup, &sgp.voc_algorithm_params, sizeof(VocAlgorithmParams));
    }

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

    digitalWrite(BOOST_EN_PIN, LOW);

    display.clearDisplay();
    display.display();

    Serial.println("ESP Deep Sleep");

    esp_sleep_enable_timer_wakeup(ESP_SLEEP_PERIOD * 1000000);
    esp_sleep_enable_ext1_wakeup(GPIO_WAKEUP_SRC, ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_deep_sleep_start();

}
