#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <config.h>

extern Preferences preferences;
extern int amountOfWater;

void taskWatering(void *parameter){
  for (;;) {
    if (irrigationRuns > 0) {
        wTimeLeft = calculateEndtimeWatering();

        setRelay(5, true); // Relay 6
        delay(secondsToMilliseconds(timePerTask)); // Pump on for 10 seconds
        setRelay(5, false);
        setRelay(6, true);
        delay(secondsToMilliseconds(timePerTask)); // Pump on for 10 seconds
        setRelay(6, false);
        setRelay(7, true);
        delay(secondsToMilliseconds(timePerTask)); // Pump on for 10 seconds
        setRelay(7, false);
        if (irrigationRuns == 1) {
          if (language == "de") {
            sendPushover("Bewässerung abgeschlossen.", "Bewässerung abgeschlossen.");
          } else {
            sendPushover("Irrigation completed.", "Irrigation completed.");
          }
        }
        irrigationRuns = irrigationRuns - 1;
    }

    // delay between checks
    if (irrigationRuns > 0) {
      tankLevelCm = pingTankLevel(TRIG, ECHO);
      // Log remaining irrigation runs
      logPrint("[IRRIGATION] Remaining irrigation runs: " + String(irrigationRuns));
      delay(minutesToMilliseconds(betweenTasks)); // wait 5 minutes before next run
    } else {
      wTimeLeft = "00:00";
      delay(10000); // check again in 10 seconds
    } 
  }
}