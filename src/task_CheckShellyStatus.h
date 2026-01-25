#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <config.h>

extern Preferences preferences;
extern int amountOfWater;

void taskShellyStatus(void *parameter){
  for (;;) {
    shelly.main.values = getShellyValues(shelly.main, 0);
    shelly.heat.values = getShellyValues(shelly.heat, 0);
    shelly.hum.values = getShellyValues(shelly.hum, 0);
    shelly.fan.values = getShellyValues(shelly.fan, 0);

    // delay  10 seconds
    delay(10000); 
  }
}