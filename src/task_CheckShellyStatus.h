#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "globals.h"
#include <cstdarg>

extern Preferences preferences;
extern int amountOfWater;

void taskShellyStatus(void *parameter){
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
        "[TASK][CheckShellyStatus] free=%u words (%u bytes), min=%u words",
        freeWords,
        freeWords * 4,
        minFree
      );

      logPrint(String(buf), true);
    }

    shelly.main.values = getShellyValues(settings.shelly.main, 0);
    shelly.heat.values = getShellyValues(settings.shelly.heat, 0);
    shelly.hum.values = getShellyValues(settings.shelly.hum, 0);
    shelly.fan.values = getShellyValues(settings.shelly.fan, 0);

    // delay  10 seconds
    delay(10000); 
  }
}