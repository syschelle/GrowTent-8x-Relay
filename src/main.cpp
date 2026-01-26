// main.cpp (optimized fast boot)

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include <LittleFS.h>
#include <deque>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// global, functions, html code, js code and css code includes
#include "globals.h"
#include "function.h"
#include "index_html.h"
#include "style_css.h"
#include "java_script.h"

// tasks
#include "task_Check_Sensor.h"
#include "task_Water_Pump_Off.h"
#include "task_Watering.h"
#include "task_CheckShellyStatus.h"

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  Serial.printf("\n*** STACK OVERFLOW in task: %s ***\n", pcTaskName ? pcTaskName : "(null)");
  Serial.flush();
  esp_restart();
}

Preferences preferences;
WebServer server(80);

// define weblog buffer variable
std::deque<String> logBuffer;

// ShellySettings is still used as a global across runtime.h/task headers
ShellySettings shelly;

// BME280 and DS18B20 sensor
SensorReadings cur;
Targets target;

// Global OneWire + DallasTemperature instance for DS18B20
OneWire oneWire(DS18B20_PIN);

// Definition for the extern declared in runtime.h
DallasTemperature sensors(&oneWire);

// Task handle for sensor task
TaskHandle_t sensorTaskHandle = nullptr;

// Forward declarations
void startSoftAP();

// -------------------- History API (ring buffer -> JSON) --------------------
static void handleApiHistory() {
  // Snapshot (simple; good enough for UI)
  const int n = (count < NUM_VALUES) ? count : NUM_VALUES;
  const int start = (index_pos - n + NUM_VALUES) % NUM_VALUES;

  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json; charset=utf-8", "");

  auto sendNumOrNull = [](float v) {
    if (isnan(v)) return String("null");
    // keep payload small
    return String(v, 2);
  };

  server.sendContent("{\"n\":" + String(n) + ",\"intervalSec\":10,");

  // temps
  server.sendContent("\"temp\":[");
  for (int i = 0; i < n; i++) {
    int idx = (start + i) % NUM_VALUES;
    if (i) server.sendContent(",");
    server.sendContent(sendNumOrNull(temps[idx]));
  }
  server.sendContent("],");

  // hum
  server.sendContent("\"hum\":[");
  for (int i = 0; i < n; i++) {
    int idx = (start + i) % NUM_VALUES;
    if (i) server.sendContent(",");
    server.sendContent(sendNumOrNull(hums[idx]));
  }
  server.sendContent("],");

  // vpd
  server.sendContent("\"vpd\":[");
  for (int i = 0; i < n; i++) {
    int idx = (start + i) % NUM_VALUES;
    if (i) server.sendContent(",");
    server.sendContent(sendNumOrNull(vpds[idx]));
  }
  server.sendContent("],");

  // water
  server.sendContent("\"water\":[");
  for (int i = 0; i < n; i++) {
    int idx = (start + i) % NUM_VALUES;
    if (i) server.sendContent(",");
    server.sendContent(sendNumOrNull(waterTemps[idx]));
  }
  server.sendContent("]}");
}

// -------------------- Deferred init task (moves slow stuff out of setup) --------------------
static void taskDeferredInit(void* /*pv*/) {
  // Read full preferences (relays, run settings, shelly, notifications, etc.)
  readPreferences();

  // DS18B20 init (quick)
  sensors.begin();

  // I2C + BME280 init (no full bus scan, no 10s retry)
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000); // faster I2C

  bmeAvailable = false;
  if (bme.begin(0x76, &Wire) || bme.begin(0x77, &Wire)) {
    bmeAvailable = true;
    logPrint("[SENSOR] BME280 initialized", true);
    // seed with one reading
    readSensorData();
    addReading(cur.extTempC, cur.humidityPct, cur.vpdKpa);
  } else {
    logPrint("[SENSOR] BME280 not found (will run without it)", true);
  }

  // Start tasks (they should be written defensively: check wifiReady/bmeAvailable)
  xTaskCreatePinnedToCore(sensorTask, "sensor", 2048, nullptr, 1, &sensorTaskHandle, 1);

  xTaskCreatePinnedToCore(taskWaterPumpOff, "PumpOff", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(taskWatering, "Watering", 2048, nullptr, 1, nullptr, 1);

  if (bmeAvailable) {
    xTaskCreatePinnedToCore(taskCheckBMESensor, "BME", 2048, nullptr, 1, nullptr, 1);
  }

  // Shelly status task can run even if WiFi isn't ready yet; it should handle that.
  xTaskCreatePinnedToCore(taskShellyStatus, "Shelly", 4096, nullptr, 1, nullptr, 1);

  // Slow one-time operations AFTER webserver is already up
  tankLevelCm = pingTankLevel(TRIG, ECHO);
  readTankLevel();

  if (wifiReady) {
    // Initial Shelly fetch (may block if device unreachable) – do it only with WiFi.
    shelly.heat.values = getShellyValues(shelly.heat, 0);
    shelly.hum.values  = getShellyValues(shelly.hum, 0);
  }

  vTaskDelete(nullptr);
}

// -------------------- setup --------------------
void setup() {
  Serial.begin(115200);

  // Mount FS quickly (don't auto-format on boot)
  if (!LittleFS.begin(false)) {
    logPrint("[LITTLEFS] mount failed", false);
  } else {
    logPrint("[LITTLEFS] mounted", false);
  }

  // Initialize relay outputs ASAP (prevent random toggles)
  for (int i = 0; i < NUM_RELAYS; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }

  // Load WiFi credentials only (fast)
  preferences.begin(PREF_NS, true);
  loadPrefString(KEY_SSID, ssidName, "", true, "ssidName");
  loadPrefString(KEY_PASS, ssidPassword, "", false, "ssidPassword");
  preferences.end();

  // Decide WiFi mode quickly: try STA for 3s, else AP
  wifiReady = false;
  if (ssidName.length() == 0) {
    espMode = true;
    startSoftAP();
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssidName.c_str(), ssidPassword.c_str());
    logPrint("[WIFI] Connecting to: " + ssidName, true);

    const uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0) < 3000) {
      delay(50);
    }

    if (WiFi.status() == WL_CONNECTED) {
      wifiReady = true;
      espMode = false;
      logPrint("[WIFI] Connected: " + WiFi.localIP().toString(), true);

      // Minimal boot info (fast)
      logPrint("[BOOT] FW build: " + String(__DATE__) + " " + String(__TIME__), true);

      // NTP sync now (only if WiFi up)
      syncDateTime();
    } else {
      logPrint("[WIFI] Connect timeout -> start AP", true);
      espMode = true;
      startSoftAP();
    }
  }

  // Routes (unchanged)
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSaveWiFi);
  server.on("/saverunsettings", HTTP_POST, handleSaveRunsettings);
  server.on("/saveshellysettings", HTTP_POST, handleSaveShellySettings);
  server.on("/savesettings", HTTP_POST, handleSaveSettings);
  server.on("/savemessagesettings", HTTP_POST, handleSaveMessageSettings);

  server.on("/style.css", []() { server.send_P(200, "text/css", cssContent); });
  server.on("/script.js", []() { server.send_P(200, "application/javascript", jsContent); });

  server.on("/sensordata", HTTP_GET, []() {
    String jsonSensorData = readSensorData();
    server.send(200, "application/json; charset=utf-8", jsonSensorData);
  });

  // history (last hour) for charts
  server.on("/api/history", HTTP_GET, handleApiHistory);

  server.on("/startWatering", HTTP_POST, handleStartWatering);
  server.on("/pingTank", HTTP_POST, readTankLevel);

  // relay toggle
  server.on("/relay/1/toggle", HTTP_POST, []() { handleRelayToggleIdx(0); });
  server.on("/relay/2/toggle", HTTP_POST, []() { handleRelayToggleIdx(1); });
  server.on("/relay/3/toggle", HTTP_POST, []() { handleRelayToggleIdx(2); });
  server.on("/relay/4/toggle", HTTP_POST, []() { handleRelayToggleIdx(3); });
  server.on("/relay/5/toggle", HTTP_POST, []() { handleRelayToggleIdx(4); });
  // water pumps
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

  server.on("/factory-reset", handleFactoryReset);
  server.on("/history", HTTP_GET, handleHistory);
  server.on("/download/history", HTTP_GET, handleDownloadHistory);
  server.on("/deletelog", HTTP_GET, handleDeleteLog);

  server.on("/favicon.ico", HTTP_GET, []() {
    String data = FAVICON_ICO_BASE64;
    server.send(200, "image/x-icon;base64", data);
  });

  server.on("/api/logbuffer", HTTP_GET, handleApiLogBuffer);
  server.on("/api/logbuffer/clear", HTTP_POST, handleClearLog);
  server.on("/log", HTTP_GET, handleDownloadLog);

  server.onNotFound([]() {
    Serial.printf("404 Not Found: %s (method %d)\n", server.uri().c_str(), (int)server.method());
  });

  server.begin();
  logPrint("[APP] Web server started", false);

  // Do the slow init after the webserver is up.
  xTaskCreatePinnedToCore(taskDeferredInit, "deferredInit", 4096, nullptr, 1, nullptr, 0);
}

// -------------------- loop --------------------
void loop() {
  server.handleClient();

  // Daily NTP sync at 01:00 AM
  struct tm timeinfo;
  if (wifiReady && getLocalTime(&timeinfo)) {
    if (timeinfo.tm_hour == 1 && timeinfo.tm_min == 0 && timeinfo.tm_mday != lastSyncDay) {
      logPrint("Performing daily NTP sync...", false);
      configTzTime(tzInfo.c_str(), ntpServer.c_str());
      lastSyncDay = timeinfo.tm_mday;
    }
  }
}
