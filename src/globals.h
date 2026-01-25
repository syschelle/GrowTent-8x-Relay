#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// -------------------- Konstanten / Defines --------------------
#define NUM_RELAYS 8
#define DS18B20_PIN 4
#define STATUS_LED_PIN 23
#define BME_ADDR 0x76

#ifndef I2C_SDA
  #define I2C_SDA 21
#endif
#ifndef I2C_SCL
  #define I2C_SCL 22
#endif

// -------------------- Structs (erst Typen, dann Settings) --------------------

// Shelly (ein Modell – das behalten wir)
struct ShellyValues {
  bool  ok = false;
  bool  isOn = false;
  float powerW = 0.0f;
  float energyWh = 0.0f;
};

struct ShellyDevice {
  String ip;
  int    kind = 0;
  int    gen  = 1;
  ShellyValues values;
};

struct ShellySettings {
  ShellyDevice main;
  ShellyDevice heat;
  ShellyDevice hum;
  ShellyDevice fan;

  String username;
  String password;
};

// Relay schedule (NUR EINMAL)
struct RelaySchedule {
  bool enabled = false;
  int start = 0; // so wie du es bisher nutzt
  int end   = 0;
};

struct RelayGroup {
  // Empfehlung: String statt char* (spart strdup/free Chaos)
  String name[NUM_RELAYS];
  RelaySchedule schedule[NUM_RELAYS];
};

struct GrowGroup {
  String startDate;
  String startFlowering;
  String startDrying;

  int currentPhase = 1;

  float targetTemperature = 22.0f;
  float offsetLeafTemperature = -1.5f;
  float targetVPD = 1.0f;

  int amountOfWater = 20;
  int irrigation = 500;
  int timePerTask = 10;
  int betweenTasks = 5;

  float minTank = 10.0f;
  float maxTank = 90.0f;
};

struct UIGroup {
  String boxName = "newGrowTent";
  String ntpServer = "pool.ntp.org";
  String tzInfo = "Europe/Berlin";

  String language = "de";
  String theme = "light";
  String unit = "metric";
  String timeFormat = "24h";

  bool ds18b20Enabled = false;
  String ds18b20Name;
};

struct NotifyGroup {
  bool pushoverEnabled = false;
  String pushoverAppKey;
  String pushoverUserKey;
  String pushoverDevice;

  bool gotifyEnabled = false;
  String gotifyServer;
  String gotifyToken;
};

struct DebugGroup {
  bool enabled = false;
};

struct Settings {
  DebugGroup debug;
  RelayGroup relay;
  GrowGroup grow;
  ShellySettings shelly;
  UIGroup ui;
  NotifyGroup notify;
};

extern Settings settings;

// Runtime structs (ok im Header)
struct SensorReadings {
  float temperatureC = NAN;
  float humidityPct  = NAN;
  float extTempC     = NAN;
  float vpdKpa       = NAN;
  uint32_t lastUpdateMs = 0;
  bool ok() const { return !isnan(temperatureC) && !isnan(humidityPct); }
};

struct Targets {
  float targetVpdKpa = NAN;
  float targetTempC  = NAN;
  float targetHumPct = NAN;
};

struct AvgAccumulator {
  float sum = 0.0f;
  uint32_t count = 0;
  inline void add(float v) { sum += v; count++; }
  inline float avg() const { return (count == 0) ? 0.0f : (sum / count); }
  inline void reset() { sum = 0.0f; count = 0; }
};

// -------------------- Globale Variablen: nur extern! --------------------

// Debug
extern bool debugLog;
extern String debugLogEnabled;

// Logging
extern const size_t LOG_MAX_LINES;

// WiFi/AP
extern const char* KEY_APSSID;
extern const char* KEY_APPASSWORD;
extern String ssidName;
extern String ssidPassword;
extern bool wifiReady;

// Relays
extern const int relayPins[NUM_RELAYS];
extern bool relayStates[NUM_RELAYS];
extern const int PUMP_COUNT;
extern unsigned long relayOffTime[NUM_RELAYS];
extern bool relayActive[NUM_RELAYS];

// Sensors
extern Adafruit_BME280 bme;
extern bool bmeAvailable;

extern volatile float DS18B20STemperature;
extern bool DS18B20;
extern String DS18B20Enable;
extern String DS18B20Name;

// LED
extern unsigned long previousMillis;
extern const unsigned long blinkInterval;
extern bool ledState;

// Averages instances
extern AvgAccumulator tempAvg;
extern AvgAccumulator humAvg;
extern AvgAccumulator vpdAvg;
extern AvgAccumulator waterTempAvg;

// Time/NTP
extern const char* DEFAULT_NTP_SERVER;
extern String ntpServer;
extern const char* DEFAULT_TZ_INFO;
extern String tzInfo;
extern RTC_DATA_ATTR int lastSyncDay;
extern struct tm local;

// Buffers / circular logs
extern const uint32_t READ_INTERVAL_MS;
extern uint32_t lastRead;
extern const uint32_t MEASUREMENT_INTERVAL_MS;

#define NUM_VALUES 360
extern float temps[NUM_VALUES];
extern float hums[NUM_VALUES];
extern float vpds[NUM_VALUES];
extern float waterTemps[NUM_VALUES];
extern float sumTemp, sumHum, sumVPD, sumWaterTemp;
extern int index_pos;
extern int count;

// Grow runtime
extern char actualDate[10];
extern int daysSinceStart, weeksSinceStart;
extern int daysSinceFlowering, weeksSinceFlowering;
extern int daysSinceDrying, weeksSinceDrying;
extern const char* phaseNames[3];

// Tank / HC-SR04
extern volatile float tankLevel;
extern float minTank, maxTank, tankLevelCm;
extern const uint8_t TRIG;
extern const uint8_t ECHO;
extern unsigned long lastMeasureTime;
extern const unsigned long MEASURE_INTERVAL;

// Log file
extern const char* LOG_PATH;
extern const uint32_t RETAIN_MS;
extern uint32_t lastCompact;
extern const uint32_t COMPACT_EVERY_MS;
extern const unsigned long LOG_INTERVAL_MS;
extern unsigned long lastLog;
