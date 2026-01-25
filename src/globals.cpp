#include "globals.h"

// Settings Root
Settings settings;

// Debug
bool debugLog = false;
String debugLogEnabled = "";

// Logging
const size_t LOG_MAX_LINES = 50;

// WiFi/AP
const char* KEY_APSSID = "new growtent";
const char* KEY_APPASSWORD = "12345678";
String ssidName = "";
String ssidPassword = "";
bool wifiReady = false;

// Relays
const int relayPins[NUM_RELAYS] = { 32, 33, 25, 26, 27, 14, 12, 13 };
bool relayStates[NUM_RELAYS] = { false };
const int PUMP_COUNT = 3;
unsigned long relayOffTime[NUM_RELAYS] = {0};
bool relayActive[NUM_RELAYS] = {false};

// Sensors
Adafruit_BME280 bme;
bool bmeAvailable = false;

volatile float DS18B20STemperature = NAN;
bool DS18B20 = false;
String DS18B20Enable = "";
String DS18B20Name = "";

// LED
unsigned long previousMillis = 0;
const unsigned long blinkInterval = 500;
bool ledState = false;

// Averages
AvgAccumulator tempAvg;
AvgAccumulator humAvg;
AvgAccumulator vpdAvg;
AvgAccumulator waterTempAvg;

// Time/NTP
const char* DEFAULT_NTP_SERVER = "de.pool.ntp.org";
String ntpServer = DEFAULT_NTP_SERVER;

const char* DEFAULT_TZ_INFO = "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
String tzInfo = DEFAULT_TZ_INFO;

RTC_DATA_ATTR int lastSyncDay = -1;
struct tm local;

// Timing
const uint32_t READ_INTERVAL_MS = 1000;
uint32_t lastRead = 0;
const uint32_t MEASUREMENT_INTERVAL_MS = 30000;

// Circular buffers
float temps[NUM_VALUES] = {0};
float hums[NUM_VALUES] = {0};
float vpds[NUM_VALUES] = {0};
float waterTemps[NUM_VALUES] = {0};
float sumTemp = 0.0f, sumHum = 0.0f, sumVPD = 0.0f, sumWaterTemp = 0.0f;
int index_pos = 0;
int count = 0;

// Grow runtime
char actualDate[10] = {0};
int daysSinceStart = 0, weeksSinceStart = 0;
int daysSinceFlowering = 0, weeksSinceFlowering = 0;
int daysSinceDrying = 0, weeksSinceDrying = 0;
const char* phaseNames[3] = { "Vegetative", "Flowering", "Drying" };

// Tank / HC-SR04
volatile float tankLevel = 0;
float minTank = 0;
float maxTank = 0;
float tankLevelCm = 0;
const uint8_t TRIG = 5;
const uint8_t ECHO = 18;
unsigned long lastMeasureTime = 0;
const unsigned long MEASURE_INTERVAL = 30UL * 60UL * 1000UL;

// Log file
const char* LOG_PATH = "/envlog.csv";
const uint32_t RETAIN_MS = 48UL * 3600UL * 1000UL;
uint32_t lastCompact = 0;
const uint32_t COMPACT_EVERY_MS = 3600UL * 1000UL;
const unsigned long LOG_INTERVAL_MS = 60000;
unsigned long lastLog = 0;
