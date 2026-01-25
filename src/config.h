#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// BEGIN Variable definitions

// debug flag default
bool debugLog = false;
String debugLogEnabled = "";

// max log lines to weblog
const size_t LOG_MAX_LINES = 50;

// WiFi Access Point credentials
const char* KEY_APSSID = "new growtent";
const char* KEY_APPASSWORD = "12345678";
// WiFi credentials storage
String ssidName = "";
String ssidPassword = "";
bool wifiReady = false;

// Relay Configuration
#define NUM_RELAYS 8
const int relayPins[NUM_RELAYS] = { 32, 33, 25, 26, 27, 14, 12, 13 };
// track relay output states (internal linkage so header can be included safely)
static bool relayStates[NUM_RELAYS] = { false };
// Number of water pumps (relays 6,7,8)
static const int PUMP_COUNT = 3;

unsigned long relayOffTime[NUM_RELAYS] = {0};
bool relayActive[NUM_RELAYS] = {false};

// BEGIN Struct definitions
// ---------- Relay ----------
struct RelaySchedule {
  bool enabled = false;
  int start = 0;
  int end = 0;
};

struct RelayGroup {
  char* name[NUM_RELAYS] = {nullptr, nullptr, nullptr, nullptr, nullptr};
  RelaySchedule schedule[NUM_RELAYS];
};

// ---------- Grow / Running ----------
struct GrowGroup {
  char startDate[16] = "";
  char startFlowering[16] = "";
  char startDrying[16] = "";

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

struct ShellyGroup {
  ShellyDevice main;
  ShellyDevice heat;
  ShellyDevice hum;
  ShellyDevice fan;

  char username[32] = "";
  char password[32] = "";
};

// ---------- UI / Box ----------
struct UIGroup {
  char boxName[32] = "newGrowTent";
  char ntpServer[64] = "pool.ntp.org";
  char tzInfo[64] = "Europe/Berlin";

  char language[4] = "de";
  char theme[8] = "light";
  char unit[8] = "metric";
  char timeFormat[4] = "24h";

  bool ds18b20Enabled = false;
  char ds18b20Name[32] = "";
};

// ---------- Notifications ----------
struct NotifyGroup {
  bool pushoverEnabled = false;
  char pushoverAppKey[64] = "";
  char pushoverUserKey[64] = "";
  char pushoverDevice[32] = "";

  bool gotifyEnabled = false;
  char gotifyServer[64] = "";
  char gotifyToken[64] = "";
};

// ---------- Debug ----------
struct DebugGroup {
  bool enabled = false;
};

// ---------- Root Settings (verschachtelt) ----------
struct Settings {
  DebugGroup debug;
  RelayGroup relay;
  GrowGroup grow;
  ShellyGroup shelly;
  UIGroup ui;
  NotifyGroup notify;
};

extern Settings settings;

struct ShellyValues {
  bool  ok = false;     // API reachable
  bool  isOn = false;   // Relay state
  float powerW = 0.0f;  // current power consumption (W)
  float energyWh = 0.0f;
};

struct ShellyDevice {
  String ip;
  int    kind;
  int    gen;
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

extern ShellySettings shelly;

// Last published sensor values
struct SensorReadings {
  float temperatureC      = NAN;  // lastTemperature
  float humidityPct       = NAN;  // lastHumidity
  float extTempC          = NAN;  // lastExtSensorTemperature
  float vpdKpa            = NAN;  // lastVPD

  uint32_t lastUpdateMs   = 0;    // optional: wann aktualisiert
  bool ok() const { return !isnan(temperatureC) && !isnan(humidityPct); }
};

struct Targets {
  float targetVpdKpa = NAN;   // targetVPD
  float targetTempC  = NAN;   // targetTemperature
  float targetHumPct = NAN;   // targetHumidity
};

struct AvgAccumulator {
  float sum = 0.0f;
  uint32_t count = 0;

  inline void add(float v) {
    sum += v;
    count++;
  }

  inline float avg() const {
    return (count == 0) ? 0.0f : (sum / count);
  }

  inline void reset() {
    sum = 0.0f;
    count = 0;
  }
};

AvgAccumulator tempAvg;
AvgAccumulator humAvg;
AvgAccumulator vpdAvg;
AvgAccumulator waterTempAvg;

// Shelly device configuration
//No TLS support
//ShellyDevice plugGen1_ipv4 { ShellyGen::Gen1,    "192.168.1.40", 80, 0 };
enum class ShellyGen : uint8_t { Gen1 = 1, Gen2Plus = 2 };

// Host kind enumeration
enum class HostKind : uint8_t {
  IPv4,
  IPv6,
  DNS
};

// Pushover notification settings
bool pushoverSent = false;
String pushoverEnabled = "";
String pushoverAppKey = "";
String pushoverUserKey = "";
String pushoverDevice = "";

// Gotify notification settings
bool gotifySent = false;
String gotifyEnabled = "";
String gotifyServer = "";
String gotifyToken = "";

// Namespace for Preferences
// WIFI keys
static const char* KEY_SSID    = "ssid";
static const char* KEY_PASS    = "password";
// namespace for preferences
static const char* KEY_DEBUG_ENABLED = "dbg";
static const char* PREF_NS     = "growtent";
//runningsettings
static const char* KEY_STARTDATE = "startDate";
static const char* KEY_FLOWERDATE = "startFlowering";
static const char* KEY_DRYINGDATE = "startDrying";
static const char* KEY_CURRENTPHASE = "curPhase";
static const char* KEY_TARGETTEMP = "targetTemp";
static const char* KEY_LEAFTEMP = "offsetLeaf";
static const char* KEY_TARGETVPD = "targetVPD";
static const char* KEY_TIMEPERTASK = "timePerTask";
static const char* KEY_BETWEENTASKS = "betweenTasks";
static const char* KEY_AMOUNTOFWATER = "amountOfWater";
static const char* KEY_IRRIGATION = "irrigation";
static const char* KEY_MINTANK = "minTank";
static const char* KEY_MAXTANK = "maxTank";

// relay scheduling keys
static const bool KEY_RELAY_ENABLE_1 = false;
static const char* KEY_RELAY_START_1;
static const char* KEY_RELAY_END_1;
static const bool KEY_RELAY_ENABLE_2 = false;
static const char* KEY_RELAY_START_2;
static const char* KEY_RELAY_END_2; 
static const bool KEY_RELAY_ENABLE_3 = false;
static const char* KEY_RELAY_START_3;
static const char* KEY_RELAY_END_3;
static const bool KEY_RELAY_ENABLE_4 = false;
static const char* KEY_RELAY_START_4;
static const char* KEY_RELAY_END_4;
static const bool KEY_RELAY_ENABLE_5 = false;
static const char* KEY_RELAY_START_5;
static const char* KEY_RELAY_END_5;
static const bool KEY_RELAY_ENABLE_6 = false; 
static const char* KEY_RELAY_START_6;
static const char* KEY_RELAY_END_6;
static const bool KEY_RELAY_ENABLE_7 = false;
static const char* KEY_RELAY_START_7;
static const char* KEY_RELAY_END_7;
static const bool KEY_RELAY_ENABLE_8 = false;
static const char* KEY_RELAY_START_8;
static const char* KEY_RELAY_END_8;

// Shelly device keys
static const char* KEY_SHMAINIP;
static const char* KEY_SHMAINGEN;
static const char* KEY_SHELLYHEATIP;
static const char* KEY_SHELLYHEATGEN;
static const char* KEY_SHELLYHUMIP;
static const char* KEY_SHELLYFANIP;
static const char* KEY_SHELLYHUMGEN;
static const char* KEY_SHELLYFANGEN;
static const char* KEY_SHELLYUSERNAME;
static const char* KEY_SHELLYPASSWORD;

// settings
static const char* KEY_NAME     = "boxName";
static const char* KEY_LANG     = "lang";
static const char* KEY_THEME    = "theme";
static const char* KEY_UNIT     = "unit";
static const char* KEY_TFMT     = "timeFmt";
static const char* KEY_NTPSRV   = "ntpSrv";
static const char* KEY_TZINFO   = "tzInfo";
static const char* KEY_DS18B20ENABLE  = "ds18b20enable";
static const char* KEY_DS18NAME = "ds18b20Name";
static const char* KEY_RELAY_1  = "relay1";
static const char* KEY_RELAY_2  = "relay2";
static const char* KEY_RELAY_3  = "relay3";
static const char* KEY_RELAY_4  = "relay4";
static const char* KEY_RELAY_5  = "relay5";
static const char* KEY_RELAY_6  = "relay6";
static const char* KEY_RELAY_7  = "relay7";
static const char* KEY_RELAY_8  = "relay8";

// notification keys
static const char* KEY_PUSHOVER = "pushover";
static const char* KEY_PUSHOVERAPP = "pushoverAppKey";
static const char* KEY_PUSHOVERUSER = "pushoverUser";
static const char* KEY_PUSHOVERDEVICE = "pushoverDevice";
static const char* KEY_GOTIFY = "gotify";
static const char* KEY_GOTIFYSERVER = "gotifyServer";
static const char* KEY_GOTIFYTOKEN = "gotifyToken";

// Include sensor libraries
Adafruit_BME280 bme;         // global instance
#ifndef I2C_SDA
  #define I2C_SDA 21
#endif
#ifndef I2C_SCL
  #define I2C_SCL 22
#endif
#define BME_ADDR 0x76
bool bmeAvailable = false;

// DS18B20 Configuration
#define DS18B20_PIN 4
volatile float DS18B20STemperature = NAN;
bool DS18B20 = false;
String DS18B20Enable = "";
String DS18B20Name = "";



// Status LED Configuration
#define STATUS_LED_PIN 23
unsigned long previousMillis = 0;
const unsigned long blinkInterval = 500; // Blinkrate in Millisekunden
bool ledState = false;
// Structure to hold relay schedule
struct RelaySchedule {
  bool enabled;
  uint16_t startMinute; // 0..59
  uint16_t endMinute;   // 0..59
};

// Array to hold schedules for each relay
bool relaySchedulesEnabled[NUM_RELAYS];
int relaySchedulesStart[NUM_RELAYS];
int relaySchedulesEnd[NUM_RELAYS];

// human designations for the relays
//static const char* relayNames[NUM_RELAYS];
String relayNames[NUM_RELAYS];

// Global configuration variables
String boxName;
String language;
String theme; // light or dark
String unit = "metric"; // metric or imperial
String timeFormat = "24h"; // 12h or 24h
bool espMode = false; // false = Station mode, true = Access Point mode

// Measurement interval in milliseconds
static constexpr uint32_t MEASUREMENT_INTERVAL_MS = 30000; // 30s

// default-NTP-Server
const char* DEFAULT_NTP_SERVER = "de.pool.ntp.org";
String ntpServer;

// default-timezone (POSIX-String)
const char* DEFAULT_TZ_INFO    = "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";  // Western European Time
String tzInfo;

// day of month for last NTP sync
RTC_DATA_ATTR int lastSyncDay = -1;

// structure to hold time info
struct tm local;

// Timestamp of last sensor read
const uint32_t READ_INTERVAL_MS = 1000;
uint32_t lastRead = 0;

// Circular buffer configuration
#define NUM_VALUES 360     // 60 min * (60 / 10) = 360 readings

// Circular buffers for Temperature, Humidity, and VPD
float temps[NUM_VALUES] = {0};
float hums[NUM_VALUES]  = {0};
float vpds[NUM_VALUES]  = {0};
float waterTemps[NUM_VALUES]  = {0};

// Running sums
float sumTemp = 0.0f;
float sumHum  = 0.0f;
float sumVPD  = 0.0f;
float sumWaterTemp  = 0.0f;

// Current position in the circular buffer
int index_pos = 0;
int count     = 0;

// Default growlight setting
char actualDate[10];
String startDate;
String startFlowering;
String startDrying;
int daysSinceStart = 0;
int weeksSinceStart = 0;
int daysSinceFlowering = 0;
int weeksSinceFlowering = 0;
int daysSinceDrying = 0;
int weeksSinceDrying = 0;

// Growth phase configuration
const char* phaseNames[3] = { "Vegetative", "Flowering", "Drying"};
int curPhase;
float targetTemperature;
float offsetLeafTemperature;
// Default VPD targets per phase
float targetVPD;
// watering
int amountOfWater;
int irrigation;
int irrigationRuns = 0;
int timePerTask;
int betweenTasks;
String wTimeLeft = "";

// Tank level
volatile float tankLevel = 0;
float minTank = 0;
float maxTank = 0;
float tankLevelCm = 0;

// HCSR04 sensor Pins
static const uint8_t TRIG = 5;
static const uint8_t ECHO = 18;
unsigned long lastMeasureTime = 0;
const unsigned long MEASURE_INTERVAL = 30UL * 60UL * 1000UL; // every 30 min

// Logfile-Settings
static const char* LOG_PATH = "/envlog.csv"; // CSV: ts_ms,tempC,hum,vpd
static const uint32_t RETAIN_MS = 48UL * 3600UL * 1000UL; // 48h
static uint32_t lastCompact = 0;
static const uint32_t COMPACT_EVERY_MS = 3600UL * 1000UL; // hourly
// Logging nur 1x pro Minute
const unsigned long LOG_INTERVAL_MS  = 60000; // 60 s
static unsigned long lastLog = 0;