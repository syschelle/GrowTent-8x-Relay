//check ever 30 seconds the current vpd with target vpd, if current vpd higer than taget vpd then power on the humidifyer shelly
//after 11 second the shelly for the humidifyer turns automaticly off. Configure that in the Webinterface oft the shelly (auto off).
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <Adafruit_BME280.h>
#include "globals.h"
#include <cstdarg>

extern Preferences preferences;
extern bool bmeAvailable;
extern Adafruit_BME280 bme;

void taskCheckBMESensor(void *parameter){
  static UBaseType_t minFree = UINT32_MAX;

  for (;;) {
    UBaseType_t freeWords = uxTaskGetStackHighWaterMark(NULL);

    if (freeWords < minFree) minFree = freeWords;
      static uint32_t last = 0;
      if (millis() - last > 5000) {
        last = millis();

        char buf[96];
        snprintf(
        buf,
        sizeof(buf),
        "[TASK][Check_Sensor] free=%u words (%u bytes), min=%u words",
        freeWords,
        freeWords * 4,
        minFree
      );

      logPrint(String(buf));
    }

    // Check every MEASURE_INTERVAL the tank level
    unsigned long now = millis();
    if (now - lastMeasureTime >= MEASURE_INTERVAL) {
      lastMeasureTime = now;
      
      tankLevelCm = pingTankLevel(TRIG, ECHO);
    }
    // Read sensor temperatur, humidity and vpd
    readSensorData();
    // delay  10 seconds
    delay(10000); 
  }
}