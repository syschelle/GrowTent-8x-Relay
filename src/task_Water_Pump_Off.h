#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <config.h>

extern Preferences preferences;
extern int amountOfWater;

void taskWaterPumpOff(void *parameter){
  for (;;) {
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