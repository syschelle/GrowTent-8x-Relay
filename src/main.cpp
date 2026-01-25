// main.cpp
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WebServer.h>
#include <Preferences.h>
#include <config.h>
#include <function.h>
#include <index_html.h>
#include <style_css.h>
#include <java_script.h>
#include <time.h>
#include <LittleFS.h>
#include <deque>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// tasks
#include <task_Check_Sensor.h>
#include <task_Water_Pump_Off.h>
#include <task_Watering.h>
#include <task_CheckShellyStatus.h>

Preferences preferences;
WebServer server(80);

// define weblog buffer variable
std::deque<String> logBuffer;

// define ShellySettings variable
ShellySettings shelly;

// BME280 and DS18B20 sensor
SensorReadings cur;
Targets target;

// Global OneWire + DallasTemperature instance for DS18B20
OneWire oneWire(DS18B20_PIN);

// Definition for the extern declared in runtime.h
DallasTemperature sensors(&oneWire);

//function prototypes
void handleSave();
void startSoftAP();
void handleSave();

// setup function
void setup() {
  Serial.begin(115200);
  
  logPrint("==== BOOT =====");
  logPrint("[BOOT] FW build: " + String(__DATE__) + " " + String(__TIME__));
  logPrint("[BOOT] Chip MAC: " + WiFi.macAddress());
  logPrint("[BOOT] Sketch MD5: " + String(ESP.getSketchMD5()));
  logPrint("[BOOT] Flash size: " + String(ESP.getFlashChipSize()));



  if (!LittleFS.begin(true)) {
    logPrint("[LITTLEFS] LittleFS mount failed");
  } else {
    logPrint("[LITTLEFS] LittleFS mounted");
  }

  // read stored preferences
  readPreferences();

  // If no SSID is stored, start SoftAP mode
  if (ssidName == "") {
      espMode = true;
      startSoftAP();
  }

  // Try to connect to stored WiFi credentials
  if (ssidName != "") {
    WiFi.begin(ssidName.c_str(), ssidPassword.c_str());
    logPrint("[WIFI] Connecting to: ");
    logPrint(ssidName + " ");

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      logPrint(".");
    }

    // Check connection status
    if (WiFi.status() == WL_CONNECTED) {
      logPrint("[WIFI] Connected! ");
      logPrint("[WIFI] IP-Address: " + WiFi.localIP().toString());

      // Sync NTP time
      syncDateTime();

      // Initialize relay outputs (LOW = OFF)
      for (int i = 0; i < NUM_RELAYS; i++) {
      pinMode(relayPins[i], OUTPUT);
      digitalWrite(relayPins[i], LOW);
      }
      
      // I2C bus scan
      Wire.begin(I2C_SDA, I2C_SCL);
      Wire.setClock(100000);
      logPrint("[I2C] Scanning bus...");
      for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
          logPrint("[I2C] Found device at 0x" + String(addr, HEX));
          delay(2);
        }
      }

      // Initialize BME280 sensor
      uint8_t candidates[2] = { 0x76, 0x77 };
      bool bmeInit = false;
      unsigned long startTime = millis();
      while (!bmeInit && millis() - startTime < 10000) {
        for (uint8_t i = 0; i < 2 && !bmeInit; i++) {
          uint8_t a = candidates[i];
          logPrint("[SENSOR] Trying BME280 at 0x" + String(a, HEX) + " ...");
          if (bme.begin(a, &Wire)) {                 // wichtig: &Wire
            logPrint("[SENSOR] BME280 initialized at 0x" + String(a, HEX));
            bmeAvailable = true;
            readSensorData();
            addReading(cur.extTempC, cur.humidityPct, cur.vpdKpa);
            bmeInit = true;
          } else {
            delay(250);
          }
        }
        if (!bmeInit) {
          logPrint("[SENSOR] BME280 not found, retrying in 500 ms. Check wiring!");
          delay(500);
        }
      }

      // Initialize DS18B20 sensor
      sensors.begin();
   
      // Create a task to turn off water pumps after 10 seconds
      xTaskCreatePinnedToCore(
        taskWaterPumpOff,                       // Task function
        "Turn off water pumps after 10s",       // Task name
        4096,                                   // Stack size
        NULL,                                   // Task input parameters
        1,                                      // Task priority, be careful when changing this
        NULL,                                   // Task handle, add one if you want control over the task (resume or suspend the task)
        1                                       // Core to run the task on
      );

      // Create a task to handle watering based on schedule
      xTaskCreatePinnedToCore(
        taskWatering,                           // Task function
        "Handle watering based on schedule",    // Task name
        8192,                                   // Stack size
        NULL,                                   // Task input parameters
        1,                                      // Task priority, be careful when changing this
        NULL,                                   // Task handle, add one if you want control over the task (resume or suspend the task)
        1                                       // Core to run the task on
      );

      // Create a task to read BME280 sensor every 10 seconds
      if (bmeAvailable) {
        xTaskCreatePinnedToCore(
          taskCheckBMESensor,                     // Task function
          "Read Values of BME280 every 10s",      // Task name
          8192,                                   // Stack size
          NULL,                                   // Task input parameters
          1,                                      // Task priority, be careful when changing this
          NULL,                                   // Task handle, add one if you want control over the task (resume or suspend the task)
          1                                       // Core to run the task on
        );
      } else {
        logPrint("[TASK] Skipping task to read sensor data because sensor is not available.");
      }

      xTaskCreatePinnedToCore(
        taskShellyStatus,                       // Task function
        "Handle watering based on schedule",    // Task name
        4096,                                   // Stack size
        NULL,                                   // Task input parameters
        1,                                      // Task priority, be careful when changing this
        NULL,                                   // Task handle, add one if you want control over the task (resume or suspend the task)
        1                                       // Core to run the task on
      );

    } else {
      // if not connected, start SoftAP mode
      logPrint("[WIFI] Failed to connect. Starting SoftAP mode...");
      startSoftAP();
    }
  }

  // funtion handlers
  server.on("/", handleRoot);
  // route for saving WiFi credentials
  server.on("/save", HTTP_POST, handleSaveWiFi);
  // route for saving runsettings
  server.on("/saverunsettings", HTTP_POST, handleSaveRunsettings);
  // route for saving Shelly settings
   server.on("/saveshellysettings", HTTP_POST, handleSaveShellySettings);
  // route for saving general settings
  server.on("/savesettings", HTTP_POST, handleSaveSettings);
  // route for saving message settings
  server.on("/savemessagesettings", HTTP_POST, handleSaveMessageSettings);
  // route for CSS
  server.on("/style.css", []() {
    server.send(200, "text/css", cssContent);
  });
  // route for JS
  server.on("/script.js", []() {
    server.send(200, "application/javascript", jsContent);
  });
   // route for pure sensor data. for update on the fly in the web page.
  server.on("/sensordata", HTTP_GET, []() {
    // Read sensor data and send as JSON
    String jsonSensorData = readSensorData();
    server.send(200, "application/json; charset=utf-8", jsonSensorData);
  });
  // route for starting watering
  server.on("/startWatering", HTTP_POST, handleStartWatering);
  // route for starting watering
  server.on("/pingTank", HTTP_POST, readTankLevel);
  // route for relay toggle
  //server.on("/relay/", HTTP_GET, handleRelayToggle);
  server.on("/relay/1/toggle", HTTP_POST, []() { handleRelayToggleIdx(0); });
  server.on("/relay/2/toggle", HTTP_POST, []() { handleRelayToggleIdx(1); });
  server.on("/relay/3/toggle", HTTP_POST, []() { handleRelayToggleIdx(2); });
  server.on("/relay/4/toggle", HTTP_POST, []() { handleRelayToggleIdx(3); });
  server.on("/relay/5/toggle", HTTP_POST, []() { handleRelayToggleIdx(4); });
  // additional relays for water pumps
  server.on("/relay/6/onFor10Sec", HTTP_POST, []() { handleRelayIrrigationIdx(5); });
  server.on("/relay/7/onFor10Sec", HTTP_POST, []() { handleRelayIrrigationIdx(6); });
  server.on("/relay/8/onFor10Sec", HTTP_POST, []() { handleRelayIrrigationIdx(7); });
  server.on("/shelly/heater/toggle", HTTP_POST, []() {
    bool ok = false;
    bool newState = false;

    ShellyValues v = getShellyValues(shelly.heat, shelly.heat.gen, 0);
    if (v.ok) {
      newState = !v.isOn;
      ok = shellySwitchSet(shelly.heat.ip, shelly.heat.gen, newState, 0, 80);
    }

    String resp = String("{\"ok\":") + (ok ? "true" : "false") +
                  String(",\"isOn\":") + (newState ? "true" : "false") + "}";

    server.send(ok ? 200 : 500, "application/json", resp);
  });
  server.on("/shelly/humidifier/toggle", HTTP_POST, []() {
    bool ok = false;
    bool newState = false;

    ShellyValues v = getShellyValues(shelly.hum, shelly.hum.gen, 0);
    if (v.ok) {
      newState = !v.isOn;
      ok = shellySwitchSet(shelly.hum.ip, shelly.hum.gen, newState, 0);
    }

    String resp = String("{\"ok\":") + (ok ? "true" : "false") +
                  String(",\"isOn\":") + (newState ? "true" : "false") + "}";

    server.send(ok ? 200 : 500, "application/json", resp);
  });
  // route for factory reset
  server.on("/factory-reset", handleFactoryReset);
  // route for getting the history log as JSON
  server.on("/history", HTTP_GET, handleHistory);
  server.onNotFound([](){
    Serial.printf("404 Not Found: %s (method %d)\n", server.uri().c_str(), (int)server.method());
  });
  // route for downloading the history log as CSV file
  server.on("/download/history", HTTP_GET, handleDownloadHistory);
  // route for deleting the history log file
  server.on("/deletelog", HTTP_GET, handleDeleteLog);
  // route for favicon.ico
  server.on("/favicon.ico", HTTP_GET, []() {
    String data = FAVICON_ICO_BASE64;
    server.send(200, "image/x-icon;base64", data);
  });
  // API endpoints for log buffer
  server.on("/api/logbuffer", HTTP_GET, handleApiLogBuffer);
  // API endpoint to clear log buffer
  server.on("/api/logbuffer/clear", HTTP_POST, handleClearLog);
  // API endpoint to download log buffer
  server.on("/log", HTTP_GET, handleDownloadLog);

  tankLevelCm = pingTankLevel(TRIG, ECHO);

  // start webserver
  server.begin();
  logPrint("[APP] Web server started");

  // Initial Shelly device status fetch
  shelly.heat.values = getShellyValues(shelly.heat, 0);
  shelly.hum.values  = getShellyValues(shelly.hum,  0);
  
  // Initial tank level reading
  readTankLevel();
}

// loop function
void loop() {
  // handle client requests
  server.handleClient();

  // Daily NTP sync at 01:00 AM
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    if (timeinfo.tm_hour == 1 && timeinfo.tm_min == 0 && timeinfo.tm_mday != lastSyncDay) {
      logPrint("Performing daily NTP sync...");
      configTzTime(tzInfo.c_str(), ntpServer.c_str());
      lastSyncDay = timeinfo.tm_mday;
    }
  }
}