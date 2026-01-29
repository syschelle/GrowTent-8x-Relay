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

// Debug variable registry ("Variables" page)
#include "vars_registry.h"

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

  // If empty after reboot: return an empty payload (no stale values)
  if (n <= 0) {
    String json =
      String("{\"n\":0,\"intervalSec\":") + String(HISTORY_INTERVAL_SEC) +
      ",\"targetTempC\":null,\"targetVpdKpa\":null,"
      "\"temp\":[],\"hum\":[],\"vpd\":[],\"water\":[]}";
    server.send(200, "application/json; charset=utf-8", json);
    return;
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json; charset=utf-8", "");

  auto sendNumOrNullN = [](float v, uint8_t dec) -> String {
    if (isnan(v)) return String("null");

    char buf[16];
    dtostrf(v, 0, dec, buf);   // width=0 → no padding
    return String(buf);
  };

  // Header
  server.sendContent("{\"n\":" + String(n) +
                     ",\"intervalSec\":" + String(HISTORY_INTERVAL_SEC) + ",");

  // ---- targets ----
  server.sendContent("\"targetTempC\":" + sendNumOrNullN(targetTemperature, 1) + ",");
  server.sendContent("\"targetVpdKpa\":" + sendNumOrNullN(targetVPD, 2) + ",");

  // ---- temp ----
  server.sendContent("\"temp\":[");
  for (int i = 0; i < n; i++) {
    const int idx = (start + i) % NUM_VALUES;
    if (i) server.sendContent(",");

    if (isnan(temps[idx])) {
      server.sendContent("null");
    } else {
      char buf[16];
      dtostrf(temps[idx], 0, 1, buf);
      server.sendContent(buf);
    }
  }
  server.sendContent("],");
  // ---- hum ----
  server.sendContent("\"hum\":[");
  for (int i = 0; i < n; i++) {
    const int idx = (start + i) % NUM_VALUES;
    if (i) server.sendContent(",");

    if (isnan(hums[idx])) {
      server.sendContent("null");
    } else {
      char buf[16];
      dtostrf(hums[idx], 0, 0, buf);   // humidity: 0 decimals
      server.sendContent(buf);
    }
  }
  server.sendContent("],");

  // ---- vpd ----
  server.sendContent("\"vpd\":[");
  for (int i = 0; i < n; i++) {
    const int idx = (start + i) % NUM_VALUES;
    if (i) server.sendContent(",");

    if (isnan(vpds[idx])) {
      server.sendContent("null");
    } else {
      char buf[16];
      dtostrf(vpds[idx], 0, 2, buf);   // VPD: 2 decimals
      server.sendContent(buf);
    }
  }
  server.sendContent("],");

  // ---- water ----
  // Keep the key name as "water" because your JS expects d.water
  server.sendContent("\"water\":[");
  for (int i = 0; i < n; i++) {
    const int idx = (start + i) % NUM_VALUES;
    if (i) server.sendContent(",");

    if (isnan(waterTemps[idx])) {
      server.sendContent("null");
    } else {
      char buf[16];
      dtostrf(waterTemps[idx], 0, 1, buf);  // water temp: 1 decimal
      server.sendContent(buf);
    }
  }
  server.sendContent("]");
  server.sendContent("}");

  server.client().stop();
}


// -------------------- Grow Diary (CSV in LittleFS) --------------------
static const char* DIARY_PATH = "/growdiary.csv";

// Parse "YYYY-MM-DD" -> time_t (local midnight). Returns 0 if invalid.
static time_t parseYmdToLocalMidnight(const String& ymd) {
  if (ymd.length() < 10) return 0;
  const int y = ymd.substring(0, 4).toInt();
  const int m = ymd.substring(5, 7).toInt();
  const int d = ymd.substring(8, 10).toInt();
  if (y < 1970 || m < 1 || m > 12 || d < 1 || d > 31) return 0;

  struct tm tmv {};
  tmv.tm_year = y - 1900;
  tmv.tm_mon  = m - 1;
  tmv.tm_mday = d;
  tmv.tm_hour = 0;
  tmv.tm_min  = 0;
  tmv.tm_sec  = 0;
  tmv.tm_isdst = -1; // let libc determine DST

  // mktime interprets tm as local time
  return mktime(&tmv);
}

static int daysSince(const String& startYmd, time_t nowLocal) {
  const time_t start = parseYmdToLocalMidnight(startYmd);
  if (start <= 0 || nowLocal <= 0) return -1;

  const double diffSec = difftime(nowLocal, start);
  const int diffDays = (int)floor(diffSec / 86400.0);
  return diffDays;
}

// CSV-safe: remove newlines and escape quotes by doubling them.
static String csvEscape(const String& in) {
  String s = in;
  s.replace("\r", " ");
  s.replace("\n", " ");
  s.replace("\"", "\"\"");
  return "\"" + s + "\"";
}

// POST /api/diary/add
// Accepts either JSON body {"note":"...","phase":"grow|flower|dry"} or form fields note=...&phase=...
static void handleDiaryAdd() {
  // --- read note + optional phase ---
  String note;
  String phaseStr;

  if (server.hasArg("plain") && server.arg("plain").length()) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (!err) {
      note = (const char*) (doc["note"] | "");
      phaseStr = (const char*) (doc["phase"] | "");
    }
  }
  if (!note.length() && server.hasArg("note")) note = server.arg("note");
  if (!phaseStr.length() && server.hasArg("phase")) phaseStr = server.arg("phase");

  note.trim();
  if (note.length() > 265) note = note.substring(0, 265);

  // Determine current phase (fallback to global curPhase if not provided)
  int phase = curPhase;
  if (phaseStr == "grow") phase = 1;
  else if (phaseStr == "flower") phase = 2;
  else if (phaseStr == "dry") phase = 3;

  const char* phaseCsv = (phase == 2) ? "flower" : (phase == 3) ? "dry" : "grow";

  // --- compute day/week for total grow and current phase ---
  time_t nowT = time(nullptr);
  struct tm nowLocalTm {};
  localtime_r(&nowT, &nowLocalTm);

  // normalize "now" to local midnight for stable day counts
  struct tm midnightTm = nowLocalTm;
  midnightTm.tm_hour = 0; midnightTm.tm_min = 0; midnightTm.tm_sec = 0;
  time_t nowLocalMidnight = mktime(&midnightTm);

  const int growDiff = daysSince(startDate, nowLocalMidnight);
  const int growDay  = (growDiff >= 0) ? (growDiff + 1) : 0;
  const int growWeek = (growDay > 0) ? ((growDay - 1) / 7 + 1) : 0;

  String phaseStart = startDate;
  if (phase == 2 && startFlowering.length() >= 10) phaseStart = startFlowering;
  if (phase == 3 && startDrying.length()   >= 10) phaseStart = startDrying;

  const int phaseDiff = daysSince(phaseStart, nowLocalMidnight);
  const int phaseDay  = (phaseDiff >= 0) ? (phaseDiff + 1) : 0;
  const int phaseWeek = (phaseDay > 0) ? ((phaseDay - 1) / 7 + 1) : 0;

  // --- timestamp ---
  char tsBuf[32];
  // ISO-like local time: YYYY-MM-DD HH:MM:SS
  snprintf(tsBuf, sizeof(tsBuf), "%04d-%02d-%02d %02d:%02d:%02d",
           nowLocalTm.tm_year + 1900, nowLocalTm.tm_mon + 1, nowLocalTm.tm_mday,
           nowLocalTm.tm_hour, nowLocalTm.tm_min, nowLocalTm.tm_sec);

  // --- write file ---
  if (!LittleFS.begin(true)) {
    server.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"err\":\"LittleFS\"}");
    return;
  }

  const bool exists = LittleFS.exists(DIARY_PATH);
  File f = LittleFS.open(DIARY_PATH, FILE_APPEND);
  if (!f) {
    server.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"err\":\"open\"}");
    return;
  }

  // Write header once
  if (!exists || f.size() == 0) {
    f.println("ts_local,phase,grow_day,grow_week,phase_day,phase_week,note");
  }

  // CSV row
  // note is quoted/escaped
  String row;
  row.reserve(512);
  row += tsBuf;
  row += ",";
  row += phaseCsv;
  row += ",";
  row += String(growDay);
  row += ",";
  row += String(growWeek);
  row += ",";
  row += String(phaseDay);
  row += ",";
  row += String(phaseWeek);
  row += ",";
  row += csvEscape(note);

  f.println(row);
  f.close();

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

// GET /api/diary.csv  -> download the diary CSV
static void handleDiaryDownload() {
  server.sendHeader("Cache-Control", "no-store");

  if (!LittleFS.begin(true) || !LittleFS.exists(DIARY_PATH)) {
    // Return empty file with header so the browser still downloads something
    server.sendHeader("Content-Disposition", "attachment; filename=\"growdiary.csv\"");
    server.send(200, "text/csv; charset=utf-8", "ts_local,phase,grow_day,grow_week,phase_day,phase_week,note\n");
    return;
  }

  File f = LittleFS.open(DIARY_PATH, FILE_READ);
  if (!f) {
    server.send(500, "text/plain; charset=utf-8", "open failed");
    return;
  }

  server.sendHeader("Content-Disposition", "attachment; filename=\"growdiary.csv\"");
  server.streamFile(f, "text/csv; charset=utf-8");
  f.close();
}

// POST /api/diary/clear  -> delete diary file
static void handleDiaryClear() {
  server.sendHeader("Cache-Control", "no-store");
  if (!LittleFS.begin(true)) {
    server.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"err\":\"LittleFS\"}");
    return;
  }
  if (LittleFS.exists(DIARY_PATH)) LittleFS.remove(DIARY_PATH);
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

// -------------------- State/Variables API (registry -> JSON) --------------------
void handleApiState() {
  const char* nl  = "\n";
  const char* ind = "  ";

  String json;
  json.reserve(2048);   // optional, aber gut gegen Fragmentierung

  json += "{";
  json += nl;

  for (size_t i = 0; i < VARS_COUNT; i++) {
    if (i) {
      json += ",";
      json += nl;
    }
    json += ind;
    json += "\"";
    json += VARS[i].key;
    json += "\": ";
    json += VARS[i].get();
  }

  json += nl;
  json += "}";

  server.send(200, "application/json", json);
}

// -------------------- Deferred init task (moves slow stuff out of setup) --------------------
static void taskDeferredInit(void* /*pv*/) {
  // Read full preferences (relays, run settings, shelly, notifications, etc.)
  readPreferences();

  // DS18B20 init (quick)
  sensors.begin();

  // I2C + BME280 init
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000); // robust for longer wiring

  // I2C bus scan
  logPrint("[I2C] Scanning bus...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      logPrint("[I2C] Found device at 0x" + String(addr, HEX));
      delay(2);
    }
  }

  // Initialize BME280 sensor (retry up to 10s)
  bmeAvailable = false;
  uint8_t candidates[2] = { 0x76, 0x77 };
  bool bmeInit = false;

  unsigned long startTime = millis();
  while (!bmeInit && (millis() - startTime < 10000)) {
    for (uint8_t i = 0; i < 2 && !bmeInit; i++) {
      uint8_t a = candidates[i];
      logPrint("[SENSOR] Trying BME280 at 0x" + String(a, HEX) + " ...");

      if (bme.begin(a, &Wire)) { // wichtig: &Wire
        logPrint("[SENSOR] BME280 initialized at 0x" + String(a, HEX));
        bmeAvailable = true;

        // seed with one reading
        readSensorData();
        addReading(cur.temperatureC, cur.humidityPct, cur.vpdKpa);

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

  if (!bmeAvailable) {
    logPrint("[SENSOR] BME280 not found (will run without it)");
  }

  // Start tasks (they should be written defensively: check wifiReady/bmeAvailable)
  xTaskCreatePinnedToCore(sensorTask, "sensor", 2048, nullptr, 1, &sensorTaskHandle, 1);

  xTaskCreatePinnedToCore(taskWaterPumpOff, "PumpOff", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(taskWatering, "Watering", 3096, nullptr, 1, nullptr, 1);

  if (bmeAvailable) {
    xTaskCreatePinnedToCore(taskCheckBMESensor, "BME", 4096, nullptr, 1, nullptr, 1);
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
    logPrint("[LITTLEFS] mount failed");
  } else {
    logPrint("[LITTLEFS] mounted");
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
    Serial.println("[WIFI] Connecting to: " + ssidName);

    const uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0) < 3000) {
      delay(50);
    }

    if (WiFi.status() == WL_CONNECTED) {
      wifiReady = true;
      espMode = false;
      Serial.println("[WIFI] Connected: " + WiFi.localIP().toString());

      // Minimal boot info (fast)
      Serial.println("[BOOT] FW build: " + String(__DATE__) + " " + String(__TIME__));

      // NTP sync now (only if WiFi up)
      logPrint("[BOOT] Starting initial NTP sync...");
      configTzTime(tzInfo.c_str(), ntpServer.c_str());
      ntpSyncPending = true;
      ntpStartMs = millis();
    } else {
      Serial.println("[WIFI] Connect timeout -> start AP");
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

  // grow diary (LittleFS CSV)
  server.on("/api/diary/add", HTTP_POST, handleDiaryAdd);
  server.on("/api/diary.csv", HTTP_GET, handleDiaryDownload);
  server.on("/api/diary/clear", HTTP_POST, handleDiaryClear);

  // full state / variables (debug page)
  server.on("/api/state", HTTP_GET, handleApiState);

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

  // diary list (for UI)
  server.on("/api/diary/list", HTTP_GET, []() {
    server.send(200, "application/json", "{\"items\":[]}");
  });

  server.on("/shelly/heater/toggle", HTTP_POST, []() {
    bool ok = false;
    bool newState = false;

    ShellyValues v = getShellyValues(settings.shelly.heat, 0);
    if (v.ok) {
      newState = !v.isOn;
      ok = shellySwitchSet(settings.shelly.heat.ip, settings.shelly.heat.gen, newState, 0, 80);
    }

    String resp = String("{\"ok\":") + (ok ? "true" : "false") +
                  String(",\"isOn\":") + (newState ? "true" : "false") + "}";

    server.send(ok ? 200 : 500, "application/json", resp);
  });

  server.on("/shelly/humidifier/toggle", HTTP_POST, []() {
    bool ok = false;
    bool newState = false;

    ShellyValues v = getShellyValues(settings.shelly.hum, 0);
    if (v.ok) {
      newState = !v.isOn;
      ok = shellySwitchSet(settings.shelly.hum.ip, settings.shelly.hum.gen, newState, 0);
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
  logPrint("[APP] Web server started");

  // Do the slow init after the webserver is up.
  xTaskCreatePinnedToCore(taskDeferredInit, "deferredInit", 4096, nullptr, 1, nullptr, 0);
}

// -------------------- loop --------------------
void loop() {
  server.handleClient();
  delay(1);

  dailyNtpTrigger();
  ntpSyncTick();
}
