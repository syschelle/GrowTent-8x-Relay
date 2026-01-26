#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "globals.h"
#include <cstdarg>

extern Preferences preferences;
extern int amountOfWater;

void taskWaterPumpOff(void *parameter){
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
        "[TASK][Water_Pump_Off] free=%u words (%u bytes), min=%u words",
        freeWords,
        freeWords * 4,
        minFree
      );

      logPrint(String(buf));
    }

    // Handle relay off timing for irrigation pumps (relays 6,7,8)
    unsigned long now = millis();

    for (int idx = 5; idx <= 7; idx++) {  // Relay 6,7,8
        if (relayActive[idx] && now >= relayOffTime[idx]) {
            setRelay(idx, false);
            relayActive[idx] = false;
            tankLevelCm = pingTankLevel(TRIG, ECHO);
        }
    }
    // delay  500 milliseconds
    delay(500); 
  }
}