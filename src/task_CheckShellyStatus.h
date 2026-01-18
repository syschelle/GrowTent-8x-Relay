#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <config.h>

extern Preferences preferences;
extern int amountOfWater;

void taskShellyStatus(void *parameter){
  for (;;) {    
    shHeater = getShellyValues(shellyHeaterDevice, shellyHeatKind);
    shHumidifier = getShellyValues(shellyHumidifierDevice, shellyHumKind);

    // delay  10 seconds
    delay(10000); 
  }
}