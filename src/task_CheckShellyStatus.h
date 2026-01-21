#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <config.h>

extern Preferences preferences;
extern int amountOfWater;

void taskShellyStatus(void *parameter){
  for (;;) {
    shMain = getShellyValues(shMainDevice, shMainKind);    
    shHeater = getShellyValues(shellyHeaterDevice, shellyHeatKind);
    shHumidifier = getShellyValues(shellyHumidifierDevice, shellyHumKind);
    shFan = getShellyValues(shFanDevice, shFanKind);

    // delay  10 seconds
    delay(10000); 
  }
}