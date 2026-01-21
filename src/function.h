#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <index_html.h>
#include <time.h>
#include <deque>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiClientSecure.h>
#include <IPAddress.h>
#include <HTTPClient.h>
#include <base64.h>
#include <mbedtls/md.h>

// Include platform-specific headers for inet_ntoa
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include <lwip/inet.h>
#endif


// If no pin is defined elsewhere, default to GPIO4
#ifndef DS18B20_PIN
#define DS18B20_PIN 4
#endif

// Create OneWire + DallasTemperature objects (internal linkage so header can be included safely)
static OneWire oneWire(DS18B20_PIN);
static DallasTemperature sensors(&oneWire);

// declare the global WebServer instance defined elsewhere
extern WebServer server;
extern Preferences preferences;
extern const char* htmlPage;
extern std::deque<String> logBuffer;
extern volatile float DS18B20STemperature;
extern unsigned long relayOffTime[];
extern bool relayActive[];

// log buffer to store recent log lines
void logPrint(const String& msg) {
  // Output serially
  Serial.println(msg);

  // Write to the weblog buffer
  logBuffer.push_back(msg);
  if (logBuffer.size() > LOG_MAX_LINES) {
    logBuffer.pop_front();  // Remove old rows if exceeding max lines logBuffer.size()
  }
}

// Helper function: Send JSON from PROGMEM
void sendJSON_P(const char* jsonP) {
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json");
  // stream the content:
  WiFiClient client = server.client();
  // Attention: PROGMEM -> Read in pieces
  PGM_P p = reinterpret_cast<PGM_P>(jsonP);
  char c;
  while ((c = pgm_read_byte(p++)) != 0) {
    client.write(c);
  }
}

// soft ap configuration
void startSoftAP() {
  IPAddress ip = WiFi.softAPIP();
  Serial.println("[SoftAP] Start SoftAP mode. IP address: " + ip.toString());

  
  // Disconnect previous connections
  WiFi.disconnect(true, true);  
  delay(100);
  
  WiFi.mode(WIFI_AP_STA);

  //last parameter 'false' = do not hide SSID
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  String apName = String(KEY_APSSID) + "_" + chipId;
  bool ok = WiFi.softAP(apName.c_str(), KEY_APPASSWORD, /*channel*/ 1, /*hidden*/ false);
  if (!ok) {
    Serial.println("[SoftAP] Error starting the SoftAP!");
    return;
  }
}


// Forward declaration so this header can call the function defined later
String readSensorData();
void calculateTimeSince(const String& startDate, int& daysSinceStartInt, int& weeksSinceStartInt);
float avgTemp();
float avgHum();
float avgVPD();
float avgWaterTemp();

// Forward-declare notification functions used before their definitions
bool sendPushover(const String& message, const String& title);
bool sendGotify(const String& msg, const String& title, int priority = 5);
String calculateEndtimeWatering();


// Handle root path "/"
void handleRoot() {
  
  String html;
  if (espMode) {
    String sensorData = readSensorData();

    // Build HTML
    html = FPSTR(apPage);
    // Replace placeholders in index_html.h
    html.replace("%CONTROLLERNAME%",  boxName);
    } else {
    html = FPSTR(htmlPage);

    if (startDate != "") {
      int daysSinceStartInt = 0;
      int weeksSinceStartInt = 0;
      calculateTimeSince(startDate, daysSinceStartInt, weeksSinceStartInt);
      String days = String(daysSinceStartInt);
      String weeks = String(weeksSinceStartInt);
      if (language == "de") {
        html.replace("%CURRENTGROW%", "Grow seit: Tag " + days + " / Woche " + weeks);
      } else {
        html.replace("%CURRENTGROW%", "Growing since: day " + days + " / week " + weeks);
      } 
    } else {
      html.replace("%CURRENTGROW%", "");
    }

    if (curPhase == 1) {
      int daysSinceStartInt = 0;
      int weeksSinceStartInt = 0;
      calculateTimeSince(startDate, daysSinceStartInt, weeksSinceStartInt);
      String days = String(daysSinceStartInt);
      String weeks = String(weeksSinceStartInt);
      if (language == "de") {
        html.replace("%CURRENTPHASE%", "<font color=\"lightgreen\">Wachstum: Tag " + days + " / Woche " + weeks + "</font>");
      } else {
        html.replace("%CURRENTPHASE%", "<font color=\"lightgreen\">Vegetative: day " + days + " / week " + weeks + "</font>");
      }
    } else if (curPhase == 2) {
      int daysSinceStartInt = 0;
      int weeksSinceStartInt = 0;
      calculateTimeSince(startFlowering, daysSinceStartInt, weeksSinceStartInt);
      String days = String(daysSinceStartInt);
      String weeks = String(weeksSinceStartInt);
      if (language == "de") {
        html.replace("%CURRENTPHASE%", "<font color=\"#ff9900\">Blüte: Tag " + days + " / Woche " + weeks + "</font>");
      } else {
        html.replace("%CURRENTPHASE%", "<font color=\"#ff9900\">Flowering: day " + days + " / week " + weeks + "</font>");
      }
    } else if (curPhase == 3) {
      int daysSinceStartInt = 0;
      int weeksSinceStartInt = 0;
      calculateTimeSince(startDrying, daysSinceStartInt, weeksSinceStartInt);
      String days = String(daysSinceStartInt);
      String weeks = String(weeksSinceStartInt);
      if (language == "de") {
        html.replace("%CURRENTPHASE%", "<font color=\"lightblue\">Trocknung: Tage " + days + " / Woche " + weeks + "</font>");
      } else {
        html.replace("%CURRENTPHASE%", "<font color=\"lightblue\">Drying: day " + days + " / week " + weeks + "</font>");
      }
    } else {
      html.replace("%CURRENTPHASE%", "");
    }

    // Replace placeholders in index_html.h
    html.replace("%TARGETTEMPERATURE%", String(targetTemperature, 1));
    html.replace("%WATERTEMPERATURE%", String(DS18B20STemperature, 1));
    html.replace("%LEAFTEMPERATURE%", String(offsetLeafTemperature, 1));
    html.replace("%HUMIDITY%", String(lastHumidity, 0));
    html.replace("%TARGETVPD%",  String(targetVPD, 1));
    html.replace("%AVGTEMP%",  String(avgTemp(), 1));
    html.replace("%AVGWATERTEMP%",  String(avgWaterTemp(), 1));
    html.replace("%AVGHUM%",  String(avgHum(), 0));
    html.replace("%AVGVPD%",  String(avgVPD(), 1));
    html.replace("%RELAYNAMES1%", String(relayNames[0]));
    html.replace("%RELAYNAMES2%", String(relayNames[1]));
    html.replace("%RELAYNAMES3%", String(relayNames[2]));
    html.replace("%RELAYNAMES4%", String(relayNames[3]));
    html.replace("%RELAYNAMES5%", String(relayNames[4]));

    html.replace("%CONTROLLERNAME%", boxName);
    html.replace("%GROWSTARTDATE%", String(startDate));
    html.replace("%GROWFLOWERDATE%", String(startFlowering));
    html.replace("%GROWDRAYINGDATE%", String(startDrying));
    html.replace("%TIMEPERTASK%", String(timePerTask));
    html.replace("%BETWEENTASKS%", String(betweenTasks));
    html.replace("%AMOUNTOFWATER%", String(amountOfWater));
    html.replace("%IRRIGATION%", String(irrigation));
    html.replace("%MINTANK%", String(minTank, 0));
    html.replace("%MAXTANK%", String(maxTank, 0));

    if (curPhase == 1) {
      html.replace("%PHASE1_SEL%", "selected");
      html.replace("%PHASE2_SEL%", "");
      html.replace("%PHASE3_SEL%", "");
    } else if (curPhase == 2) {
      html.replace("%PHASE1_SEL%", "");
      html.replace("%PHASE2_SEL%", "selected");
      html.replace("%PHASE3_SEL%", "");
    } else if (curPhase == 3) {
      html.replace("%PHASE1_SEL%", "");
      html.replace("%PHASE2_SEL%", "");
      html.replace("%PHASE3_SEL%", "selected");
    } else {
      html.replace("%PHASE1_SEL%", "");
      html.replace("%PHASE2_SEL%", "");
      html.replace("%PHASE3_SEL%", "");
    }

    html.replace("%TARGETVPD%", String(targetVPD, 1));

    html.replace("%SHMAINIP%", shMainDevice);
    if (shMainKind == 1) {
      html.replace("%SHMAINSWKIND1%", "selected");
      html.replace("%SHMAINSWKIND2%", "");
      html.replace("%SHMAINSWKIND3%", "");
    } else if (shMainKind == 2) {
      html.replace("%SHMAINSWKIND1%", "");
      html.replace("%SHMAINSWKIND2%", "selected");
      html.replace("%SHMAINSWKIND3%", "");
    } else if (shMainKind == 3) {
      html.replace("%SHMAINSWKIND1%", "");
      html.replace("%SHMAINSWKIND2%", "");
      html.replace("%SHMAINSWKIND3%", "selected");
    } else {
      html.replace("%SHMAINSWKIND1%", "");
      html.replace("%SHMAINSWKIND2%", "");
      html.replace("%SHMAINSWKIND3%", "");
    }

    html.replace("%SHELLYHEATERIP%", shellyHeaterDevice);
    if (shellyHeatKind == 1) {
      html.replace("%SHHEATKIND1%", "selected");
      html.replace("%SHHEATKIND2%", "");
      html.replace("%SHHEATKIND3%", "");
    } else if (shellyHeatKind == 2) {
      html.replace("%SHHEATKIND1%", "");
      html.replace("%SHHEATKIND2%", "selected");
      html.replace("%SHHEATKIND3%", "");
    } else if (shellyHeatKind == 3) {
      html.replace("%SHHEATKIND1%", "");
      html.replace("%SHHEATKIND2%", "");
      html.replace("%SHHEATKIND3%", "selected");
    } else {
      html.replace("%SHHEATKIND1%", "");
      html.replace("%SHHEATKIND2%", "");
      html.replace("%SHHEATKIND3%", "");
    }

    html.replace("%SHELLYHUMIDIFIERIP%", shellyHumidifierDevice);
    if (shellyHumKind == 1) {
      html.replace("%SHHUMIDKIND1%", "selected");
      html.replace("%SHHUMIDKIND2%", "");
      html.replace("%SHHUMIDKIND3%", "");
    } else if (shellyHumKind == 2) {
      html.replace("%SHHUMIDKIND1%", "");
      html.replace("%SHHUMIDKIND2%", "selected");
      html.replace("%SHHUMIDKIND3%", "");
    } else if (shellyHumKind == 3) {
      html.replace("%SHHUMIDKIND1%", "");
      html.replace("%SHHUMIDKIND2%", "");
      html.replace("%SHHUMIDKIND3%", "selected");
    } else {
      html.replace("%SHHUMIDKIND1%", "");
      html.replace("%SHHUMIDKIND2%", "");
      html.replace("%SHHUMIDKIND3%", "");
    }

    html.replace("%SHELLYFANIP%", shFanDevice);
    if (shFanKind == 1) {
      html.replace("%SHFANKIND1%", "selected");
      html.replace("%SHFANKIND2%", "");
      html.replace("%SHFANKIND3%", "");
    } else if (shFanKind == 2) {
      html.replace("%SHFANKIND1%", "");
      html.replace("%SHFANKIND2%", "selected");
      html.replace("%SHFANKIND3%", "");
    } else if (shFanKind == 3) {
      html.replace("%SHFANKIND1%", "");
      html.replace("%SHFANKIND2%", "");
      html.replace("%SHFANKIND3%", "selected");
    } else {
      html.replace("%SHFANKIND1%", "");
      html.replace("%SHFANKIND2%", "");
      html.replace("%SHFANKIND3%", "");
    }

    html.replace("%SHUSER%", shUser);
    html.replace("%SHPASSWORD%", shPass);

    html.replace("%NTPSERVER%", ntpServer);
    html.replace("%TZINFO%", tzInfo);
    html.replace("%THEME%", theme);
    html.replace("%LANGUAGE%", language);
    html.replace("%TIMEFORMAT%", timeFormat);
    html.replace("%UNIT%", unit);
    html.replace("%DS18B20ENABLE%", DS18B20Enable);
    html.replace("%DS18B20NAME%", DS18B20Name);

    html.replace("%PUSHOVERENABLED%", pushoverEnabled);
    html.replace("%PUSHOVERAPPKEY%", pushoverAppKey);
    html.replace("%PUSHOVERUSERKEY%", pushoverUserKey);
    html.replace("%PUSHOVERDEVICE%", pushoverDevice);
    html.replace("%GOTIFYENABLED%", gotifyEnabled);
    html.replace("%GOTIFYURL%", gotifyServer);
    html.replace("%GOTIFYTOKEN%", gotifyToken);
  }

  server.send(200, "text/html", html);
}

// Read stored preferences
void readPreferences() {
  preferences.begin(PREF_NS, true);
  //WIFI
  ssidName = preferences.isKey(KEY_SSID) ? preferences.getString(KEY_SSID) : String();
  ssidPassword = preferences.isKey(KEY_PASS) ? preferences.getString(KEY_PASS) : String();
  // relays
  relayNames[0] = preferences.isKey(KEY_RELAY_1) ? strdup(preferences.getString(KEY_RELAY_1).c_str()) : strdup("relay 1");
  relayNames[1] = preferences.isKey(KEY_RELAY_2) ? strdup(preferences.getString(KEY_RELAY_2).c_str()) : strdup("relay 2");
  relayNames[2] = preferences.isKey(KEY_RELAY_3) ? strdup(preferences.getString(KEY_RELAY_3).c_str()) : strdup("relay 3");
  relayNames[3] = preferences.isKey(KEY_RELAY_4) ? strdup(preferences.getString(KEY_RELAY_4).c_str()) : strdup("relay 4");
  relayNames[4] = preferences.isKey(KEY_RELAY_5) ? strdup(preferences.getString(KEY_RELAY_5).c_str()) : strdup("relay 5");
  // running settings
  startDate = preferences.isKey(KEY_STARTDATE) ? preferences.getString(KEY_STARTDATE) : String();
  startFlowering = preferences.isKey(KEY_FLOWERDATE) ? preferences.getString(KEY_FLOWERDATE) : String();
  startDrying = preferences.isKey(KEY_DRYINGDATE) ? preferences.getString(KEY_DRYINGDATE) : String();
  curPhase = preferences.isKey(KEY_CURRENTPHASE) ? preferences.getInt(KEY_CURRENTPHASE) : 1;
  targetTemperature = preferences.isKey(KEY_TARGETTEMP) ? preferences.getFloat(KEY_TARGETTEMP) : 22.0;
  offsetLeafTemperature = preferences.isKey(KEY_LEAFTEMP) ? preferences.getFloat(KEY_LEAFTEMP) : -1.5;
  targetVPD = preferences.isKey(KEY_TARGETVPD) ? preferences.getFloat(KEY_TARGETVPD) : 1.0;
  amountOfWater = preferences.isKey(KEY_AMOUNTOFWATER) ? preferences.getInt(KEY_AMOUNTOFWATER) : 20;
  irrigation = preferences.isKey(KEY_IRRIGATION) ? preferences.getInt(KEY_IRRIGATION) : 500;
  timePerTask = preferences.isKey(KEY_TIMEPERTASK) ? preferences.getInt(KEY_TIMEPERTASK) : 10;
  betweenTasks = preferences.isKey(KEY_BETWEENTASKS) ? preferences.getInt(KEY_BETWEENTASKS) : 5;
  minTank = preferences.isKey(KEY_MINTANK) ? preferences.getFloat(KEY_MINTANK) : 10.0f;
  maxTank = preferences.isKey(KEY_MAXTANK) ? preferences.getFloat(KEY_MAXTANK) : 90.0f;

  // relay schedules
  // Use explicit key names and provide a default value for getBool() to match the Preferences API
  relaySchedulesEnabled[0] = preferences.getBool("relay_enable_1", false);
  relaySchedulesStart[0] = preferences.getInt(KEY_RELAY_START_1, 0);
  relaySchedulesEnd[0] = preferences.getInt(KEY_RELAY_END_1, 0);
  relaySchedulesEnabled[1] = preferences.getBool("relay_enable_2", false);
  relaySchedulesStart[1] = preferences.getInt(KEY_RELAY_START_2, 0);
  relaySchedulesEnd[1] = preferences.getInt(KEY_RELAY_END_2, 0);
  relaySchedulesEnabled[2] = preferences.getBool("relay_enable_3", false);
  relaySchedulesStart[2] = preferences.getInt(KEY_RELAY_START_3, 0);
  relaySchedulesEnd[2] = preferences.getInt(KEY_RELAY_END_3, 0);
  relaySchedulesEnabled[3] = preferences.getBool("relay_enable_4", false);
  relaySchedulesStart[3] = preferences.getInt(KEY_RELAY_START_4, 0);
  relaySchedulesEnd[3] = preferences.getInt(KEY_RELAY_END_4, 0);
  relaySchedulesEnabled[4] = preferences.getBool("relay_enable_5", false);
  relaySchedulesStart[4] = preferences.getInt(KEY_RELAY_START_5, 0);
  relaySchedulesEnd[4] = preferences.getInt(KEY_RELAY_END_5, 0);

  // Shelly devices
  shMainDevice = preferences.isKey(KEY_SHMAINIP) ? preferences.getString(KEY_SHMAINIP) : String("");
  shMainKind = preferences.isKey(KEY_SHMAINKIND) ? preferences.getInt(KEY_SHMAINKIND) : 0;
  shellyHeaterDevice = preferences.isKey(KEY_SHELLYHEATIP) ? preferences.getString(KEY_SHELLYHEATIP) : String("");
  shellyHeatKind = preferences.isKey(KEY_SHELLYHEATKIND) ? preferences.getInt(KEY_SHELLYHEATKIND) : 0;
  shellyHumidifierDevice = preferences.isKey(KEY_SHELLYHUMIP) ? preferences.getString(KEY_SHELLYHUMIP) : String("");
  shellyHumKind = preferences.isKey(KEY_SHELLYHUMKIND) ? preferences.getInt(KEY_SHELLYHUMKIND) : 0;
  shFanDevice = preferences.isKey(KEY_SHELLYFANIP) ? preferences.getString(KEY_SHELLYFANIP) : String("");
  shFanKind = preferences.isKey(KEY_SHELLYFANKIND) ? preferences.getInt(KEY_SHELLYFANKIND) : 0;
  // optional Basic Auth:
  shUser = preferences.isKey(KEY_SHELLYUSERNAME) ? preferences.getString(KEY_SHELLYUSERNAME) : String("");
  shPass = preferences.isKey(KEY_SHELLYPASSWORD) ? preferences.getString(KEY_SHELLYPASSWORD) : String("");

  // settings
  boxName = preferences.isKey(KEY_NAME) ? preferences.getString(KEY_NAME) : String("newGrowTent");
  ntpServer = preferences.isKey(KEY_NTPSRV) ? preferences.getString(KEY_NTPSRV) : String(DEFAULT_NTP_SERVER);
  tzInfo = preferences.isKey(KEY_TZINFO) ? preferences.getString(KEY_TZINFO) : String(DEFAULT_TZ_INFO);
  language = preferences.isKey(KEY_LANG) ? preferences.getString(KEY_LANG) : String("de");
  theme = preferences.isKey(KEY_THEME) ? preferences.getString(KEY_THEME) : String("light");
  unit = preferences.isKey(KEY_UNIT) ? preferences.getString(KEY_UNIT) : String("metric");
  timeFormat = preferences.isKey(KEY_TFMT) ? preferences.getString(KEY_TFMT) : String("24h");
  DS18B20Enable = preferences.isKey(KEY_DS18B20ENABLE) ? preferences.getString(KEY_DS18B20ENABLE) : String("");
  if (DS18B20Enable == "checked") DS18B20 = true;
  DS18B20Name = preferences.isKey(KEY_DS18NAME) ? preferences.getString(KEY_DS18NAME) : String("");
  // notification settings
  pushoverEnabled = preferences.isKey(KEY_PUSHOVER) ? preferences.getString(KEY_PUSHOVER) : String("");
  if (pushoverEnabled == "checked") pushoverSent = true;  
  pushoverAppKey = preferences.isKey(KEY_PUSHOVERAPP) ? preferences.getString(KEY_PUSHOVERAPP) : String("");
  pushoverUserKey = preferences.isKey(KEY_PUSHOVERUSER) ? preferences.getString(KEY_PUSHOVERUSER) : String("");
  pushoverDevice = preferences.isKey(KEY_PUSHOVERDEVICE) ? preferences.getString(KEY_PUSHOVERDEVICE) : String("");
  gotifyEnabled = preferences.isKey(KEY_GOTIFY) ? preferences.getString(KEY_GOTIFY) : String("");
  if (gotifyEnabled == "checked") gotifySent = true;
  gotifyServer = preferences.isKey(KEY_GOTIFYSERVER) ? preferences.getString(KEY_GOTIFYSERVER) : String("");
  gotifyToken = preferences.isKey(KEY_GOTIFYTOKEN) ? preferences.getString(KEY_GOTIFYTOKEN) : String("");

  preferences.end();
  logPrint("[PREF] loading - ssid:" + ssidName + " boxName:" + boxName + " language:" + language + " theme:" + theme +
           " unit:" + unit + " timeFormat:" + timeFormat + " ntpServer:" + ntpServer + " tzInfo:" + tzInfo + "curentPhase:" + String(curPhase) +
           " startDate:" + startDate + " floweringStart:" + startFlowering + " dryingStart:" + startDrying +
           " targetTemperature:" + targetTemperature + " offsetLeafTemperature:" + offsetLeafTemperature +
           " Shelly Heater Kind: " + String(shellyHeatKind)  + " Shelly Humidifier Kind: " + String(shellyHumKind) + 
           " targetVPD:" + targetVPD + " curPhase:" + String(curPhase) + " Relayname1:" + relayNames[0] + 
           " Relayname2:" + relayNames[1] + " Relayname3:" + relayNames[2] + " Relayname4:" + relayNames[3] +
           " AmountOfWater:" + String(amountOfWater) + " Irrigation:" + String(irrigation));
}

void handleSaveRunsettings() {
  // Open the Preferences namespace with write access (readOnly = false)
  // Only call begin() once — calling it twice can cause writes to fail!
  if (!preferences.begin(PREF_NS, false)) {
    logPrint("[PREF][ERROR] preferences.begin() failed. "
             "Check that PREF_NS length <= 15 characters.");
    server.send(500, "text/plain", "Failed to open Preferences");
    return;
  }

  // Save grow start date if provided
  if (server.hasArg("webGrowStart")) {
    startDate = server.arg("webGrowStart");
    preferences.putString(KEY_STARTDATE, startDate);
    logPrint("[PREFERENCES] " + String(KEY_STARTDATE) + " written : " + startDate);
  }

  // Save flowering start date if provided
  if (server.hasArg("webFloweringStart")) {
    startFlowering = server.arg("webFloweringStart");
    preferences.putString(KEY_FLOWERDATE, startFlowering);
    logPrint("[PREFERENCES] " + String(KEY_FLOWERDATE) + " written: " + startFlowering);
  }

  // Save drying start date if provided
  if (server.hasArg("webDryingStart")) {
    String startDrying = server.arg("webDryingStart");
    preferences.putString(KEY_DRYINGDATE, startDrying);
    logPrint("[PREFERENCES] " + String(KEY_DRYINGDATE) + " written: " + startDrying);
  }

  // Save current phase if provided
  if (server.hasArg("webCurrentPhase")) {
    curPhase = server.arg("webCurrentPhase").toInt();
    preferences.putInt(KEY_CURRENTPHASE, curPhase);
    logPrint("[PREFERENCES] " + String(KEY_CURRENTPHASE) + " written: " + curPhase);
  }

  // Save target temperature if provided
  if (server.hasArg("webTargetTemp")) {
    targetTemperature = server.arg("webTargetTemp").toFloat();
    preferences.putFloat(KEY_TARGETTEMP, targetTemperature);
    logPrint("[PREFERENCES] " + String(KEY_TARGETTEMP) + " written: " + targetTemperature);
  }

  // Save target VPD if provided
  if (server.hasArg("webTargetVPD")) {
    targetVPD = server.arg("webTargetVPD").toFloat();
    preferences.putFloat(KEY_TARGETVPD, targetVPD);
    logPrint("[PREFERENCES] " + String(KEY_TARGETVPD) + " written: " + targetVPD);
  }

  // Save leaf temperature offset if provided
  if (server.hasArg("webOffsetLeafTemp")) {
    preferences.putFloat(KEY_LEAFTEMP, server.arg("webOffsetLeafTemp").toFloat());
    logPrint("[PREFERENCES] " + String(KEY_LEAFTEMP) + " written: " + offsetLeafTemperature);
  }

  // Save time per task if provided
  if (server.hasArg("webTimePerTask")) {
    timePerTask = server.arg("webTimePerTask").toInt();
    preferences.putInt(KEY_TIMEPERTASK, timePerTask);
    logPrint("[PREFERENCES] " + String(KEY_TIMEPERTASK) + " written: " + timePerTask);
  }

  // Save pause between tasks if provided
  if (server.hasArg("webBetweenTasks")) {
    betweenTasks = server.arg("webBetweenTasks").toInt();
    preferences.putInt(KEY_BETWEENTASKS, betweenTasks);
    logPrint("[PREFERENCES] " + String(KEY_BETWEENTASKS) + " written: " + betweenTasks);
  }
  // Save amount of water if provided
  if (server.hasArg("webAmountOfWater")) {
    amountOfWater = server.arg("webAmountOfWater").toInt();
    preferences.putInt(KEY_AMOUNTOFWATER, amountOfWater);
    logPrint("[PREFERENCES] " + String(KEY_AMOUNTOFWATER) + " written: " + amountOfWater);
  }

  // Save amount of irrigation if provided
  if (server.hasArg("webIrrigation")) {
    irrigation = server.arg("webIrrigation").toInt();
    preferences.putInt(KEY_IRRIGATION, irrigation);
    logPrint("[PREFERENCES] " + String(KEY_IRRIGATION) + " written: " + irrigation);
  }

  // Save min tank level if provided
  if (server.hasArg("webMinTank")) {
    minTank = server.arg("webMinTank").toFloat();
    preferences.putFloat(KEY_MINTANK, minTank);
    logPrint("[PREFERENCES] " + String(KEY_MINTANK) + " written: " + String(minTank, 0));
  }

  // Save max tank level if provided
  if (server.hasArg("webMaxTank")) {
    maxTank = server.arg("webMaxTank").toFloat();
    preferences.putFloat(KEY_MAXTANK, maxTank);
    logPrint("[PREFERENCES] " + String(KEY_MAXTANK) + " written: " + String(maxTank, 0));
  }

  preferences.end(); // always close Preferences handle

  // Send redirect response and restart the ESP
  server.sendHeader("Location", "/");
  server.send(303);  // HTTP redirect to status page
  delay(250);
  ESP.restart();
}

// Handle Shelly settings save
void handleSaveShellySettings() {
  // Open the Preferences namespace with write access (readOnly = false)
  // Only call begin() once — calling it twice can cause writes to fail!
  if (!preferences.begin(PREF_NS, false)) {
    logPrint("[PREFERENCES][ERROR] preferences.begin() failed. "
             "Check that PREF_NS length <= 15 characters.");
    server.send(500, "text/plain", "Failed to open Preferences");
    return;
  }

  if (server.hasArg("webShellyMainSwIP")) {
    shMainDevice = server.arg("webShellyMainSwIP");
    preferences.putString(KEY_SHMAINIP, shMainDevice);
    logPrint("[PREFERENCES] " + String(KEY_SHMAINIP) + " written bytes: " + shMainDevice);
  }

  if (server.hasArg("webShMainSwKind")) {
    shMainKind = server.arg("webShMainSwKind").toInt();
    preferences.putInt(KEY_SHMAINKIND, shMainKind);
    logPrint("[PREFERENCES] Shelly Main Switch Kind set to: " + String(shMainKind));
  }

  // Save Shelly Heater if provided
  if (server.hasArg("webShellyHeatIP")) {
    shellyHeaterDevice = server.arg("webShellyHeatIP");
    preferences.putString(KEY_SHELLYHEATIP, shellyHeaterDevice);
    logPrint("[PREFERENCES] " + String(KEY_SHELLYHEATIP) + " written bytes: " + shellyHeaterDevice);
  }
  if (server.hasArg("webShHeatKind")) {
    shellyHeatKind = server.arg("webShHeatKind").toInt();
    preferences.putInt(KEY_SHELLYHEATKIND, shellyHeatKind);
    logPrint("[PREFERENCES] Shelly Heater Kind set to: " + String(shellyHeatKind));
  }

  // Save Shelly Humidifier provided
  if (server.hasArg("webShellyHumIP")) {
    shellyHumidifierDevice = server.arg("webShellyHumIP");
    preferences.putString(KEY_SHELLYHUMIP, shellyHumidifierDevice);
    logPrint("[PREFERENCES] " + String(KEY_SHELLYHUMIP) + " written bytes: " + shellyHumidifierDevice);
  }
  if (server.hasArg("webShHumKind")) {
    shellyHumKind = server.arg("webShHumKind").toInt();
    preferences.putInt(KEY_SHELLYHUMKIND, shellyHumKind);
    logPrint("[PREFERENCES] Shelly Humidifier Kind set to: " + String(shellyHumKind));
  }

  // Save Shelly Fan if provided
  if (server.hasArg("webShFanIp")) {
    shFanDevice = server.arg("webShFanIp");
    preferences.putString(KEY_SHELLYFANIP, shFanDevice);
    logPrint("[PREFERENCES] " + String(KEY_SHELLYFANIP) + " written bytes: " + shFanDevice);
  }
  if (server.hasArg("webShFanKind")) {
    shFanKind = server.arg("webShFanKind").toInt();
    preferences.putInt(KEY_SHELLYFANKIND, shFanKind);
    logPrint("[PREFERENCES] Shelly Fan Kind set to: " + String(shFanKind));
  }

  // Save Shelly Username if provided
  if (server.hasArg("webShellyUsername")) {
    shUser = server.arg("webShellyUsername");
    preferences.putString(KEY_SHELLYUSERNAME, shUser);
    logPrint("[PREFERENCES] " + String(KEY_SHELLYUSERNAME) + " written bytes: " + shUser);
  }

  // Save Shelly Password if provided
  if (server.hasArg("webShellyPassword")) {
    shPass = server.arg("webShellyPassword");
    preferences.putString(KEY_SHELLYPASSWORD, shPass);
    logPrint("[PREFERENCES] " + String(KEY_SHELLYPASSWORD) + " written bytes: " + shPass);
  }

  preferences.end(); // always close Preferences handle

  // Send redirect response and restart the ESP
  server.sendHeader("Location", "/");
  server.send(303);  // HTTP redirect to status page
  delay(250);
  ESP.restart();
}

// Handle general settings save
void handleSaveSettings() {
  // Open the Preferences namespace with write access (readOnly = false)
  // Only call begin() once — calling it twice can cause writes to fail!
  if (!preferences.begin(PREF_NS, false)) {
    logPrint("[PREFERENCES][ERROR] preferences.begin() failed. "
             "Check that PREF_NS length <= 15 characters.");
    server.send(500, "text/plain", "Failed to open Preferences");
    return;
  }

  // Save box name if provided
  if (server.hasArg("webBoxName")) {
    boxName = server.arg("webBoxName");
    preferences.putString(KEY_NAME, boxName);
    logPrint("[PREFERENCES] " + String(KEY_NAME) + " written bytes: " + boxName);
  }
  
  // Save NTP server if provided
  if (server.hasArg("webNTPServer")) {
    ntpServer = server.arg("webNTPServer");
    preferences.putString(KEY_NTPSRV, ntpServer);
    logPrint("[PREFERENCES] " + String(KEY_NTPSRV) + " written bytes: " + ntpServer);
  }

  // Save timezone info if provided
  if (server.hasArg("webTimeZoneInfo")) {
    tzInfo = server.arg("webTimeZoneInfo");
    preferences.putString(KEY_TZINFO, tzInfo);
    logPrint("[PREFERENCES] " + String(KEY_TZINFO) + " written bytes: " + tzInfo);
  } 

  // Save language if provided
  if (server.hasArg("webLanguage")) {
    language = server.arg("webLanguage");
    preferences.putString(KEY_LANG, language);
    logPrint("[PREFERENCES] " + String(KEY_LANG) + " written bytes: " + language);
  }

  // Save theme if provided
  if (server.hasArg("webTheme")) {
    theme = server.arg("webTheme");
    preferences.putString(KEY_THEME, theme);
    logPrint("[PREFERENCES] " + String(KEY_THEME) + " written bytes: " + theme);
  }
  // 7) Save time format if provided
  if (server.hasArg("webTimeFormat")) {
    timeFormat = server.arg("webTimeFormat");
    preferences.putString(KEY_TFMT, timeFormat);
    logPrint("[PREFERENCES] " + String(KEY_TFMT) + " written bytes: " + timeFormat);
  }
  // 8) Save unit if provided
  if (server.hasArg("webTempUnit")) {
    unit = server.arg("webTempUnit");
    preferences.putString(KEY_UNIT, unit);
    logPrint("[PREFERENCES] " + String(KEY_UNIT) + " written bytes: " + unit);
  }

  if (server.arg("webDS18B20Enable") == "on") {
    DS18B20Enable = "checked";
  } else {
    DS18B20Enable = "";
  }
  logPrint("[PREFERENCES] " + String(KEY_DS18B20ENABLE) + " " + String(DS18B20Enable));
  preferences.putString(KEY_DS18B20ENABLE, DS18B20Enable);

  if (server.hasArg("webDS18B20Name")) {
    DS18B20Name = server.arg("webDS18B20Name");
    preferences.putString(KEY_DS18NAME, DS18B20Name);
    logPrint("[PREFERENCES] ds18b20_name written bytes: " + DS18B20Name);
  }

  if (server.hasArg("webRelayName1")) {
    String v = server.arg("webRelayName1");
    preferences.putString(KEY_RELAY_1, v);
    logPrint("[PREFERENCES] " + String(KEY_RELAY_1) + " written bytes: " + v);
    relayNames[0] = strdup(v.c_str());
  }

  if (server.hasArg("webRelayName2")) {
    String v = server.arg("webRelayName2");
    preferences.putString(KEY_RELAY_2, v);
    logPrint("[PREFERENCES] " + String(KEY_RELAY_2) + " written bytes: " + v);
    relayNames[1] = strdup(v.c_str());
  }

  if (server.hasArg("webRelayName3")) {
    String v = server.arg("webRelayName3");
    preferences.putString(KEY_RELAY_3, v);
    logPrint("[PREFERENCES] " + String(KEY_RELAY_3) + " written bytes: " + v);
    relayNames[2] = strdup(v.c_str());
  }

  if (server.hasArg("webRelayName4")) {
    String v = server.arg("webRelayName4");
    preferences.putString(KEY_RELAY_4, v);
    logPrint("[PREFERENCES] " + String(KEY_RELAY_4) + " written bytes: " + v);
    relayNames[3] = strdup(v.c_str());
  }

  if (server.hasArg("webRelayName5")) {
    String v = server.arg("webRelayName5");
    preferences.putString(KEY_RELAY_5, v);
    logPrint("[PREFERENCES] " + String(KEY_RELAY_5) + " written bytes: " + v);
    relayNames[4] = strdup(v.c_str());
  }

  preferences.end(); // always close Preferences handle

  // 11) Send redirect response and restart the ESP
  server.sendHeader("Location", "/");
  server.send(303);  // HTTP redirect to status page
  delay(250);
  ESP.restart();
}

void handleSaveMessageSettings() {
  if (!preferences.begin(PREF_NS, false)) {
    logPrint("[PREFERENCES][ERROR] preferences.begin() failed. "
             "Check that PREF_NS length <= 15 characters.");
    server.send(500, "text/plain", "Failed to open Preferences");
    return;
  }

  if (server.arg("webPushoverEnabled") == "on") {
    pushoverEnabled = "checked";
  } else {
    pushoverEnabled = "";
  }

  logPrint("[PREFERENCES] Pushover " + String(pushoverEnabled));
  preferences.putString(KEY_PUSHOVER, pushoverEnabled);

  if (server.hasArg("webPushoverUserKey")) {
    pushoverUserKey = server.arg("webPushoverUserKey");
    preferences.putString(KEY_PUSHOVERUSER, pushoverUserKey);
    logPrint("[PREFERENCES] " + String(KEY_PUSHOVERUSER) + " written bytes: " + pushoverUserKey);
  }

  if (server.hasArg("webPushoverAppKey")) {
    pushoverAppKey = server.arg("webPushoverAppKey");
    preferences.putString(KEY_PUSHOVERAPP, pushoverAppKey);
    logPrint("[PREFERENCES] " + String(KEY_PUSHOVERAPP) + " written bytes: " + pushoverAppKey);
  }

  if (server.hasArg("webPushoverDevice")) {
    pushoverDevice = server.arg("webPushoverDevice");
    preferences.putString(KEY_PUSHOVERDEVICE, pushoverDevice);
    logPrint("[PREFERENCES] " + String(KEY_PUSHOVERDEVICE) + " written bytes: " + pushoverDevice);
  }
  
  if (server.arg("webGotifyEnabled") == "on") {
    gotifyEnabled = "checked";
  } else {
    gotifyEnabled = "";
  }
  logPrint("[PREFERENCES] Gotify " + String(gotifyEnabled));
  preferences.putString(KEY_GOTIFY, gotifyEnabled);

  if (server.hasArg("webGotifyURL")) { 
    gotifyServer = server.arg("webGotifyURL");
    preferences.putString(KEY_GOTIFYSERVER, gotifyServer);
    logPrint("[PREFERENCES] " + String(KEY_GOTIFYSERVER) + " written bytes: " + gotifyServer);
  }

  if (server.hasArg("webGotifyToken")) {
    gotifyToken = server.arg("webGotifyToken");
    preferences.putString(KEY_GOTIFYTOKEN, gotifyToken);
    logPrint("[PREFERENCES] " + String(KEY_GOTIFYTOKEN) + " written bytes: " + gotifyToken);
  }

  preferences.end(); // always close Preferences handle

  if (pushoverEnabled == "checked") {
    if (language == "de") {
      sendPushover("Testnachricht", "Die Pushover-Benachrichtigungen wurden erfolgreich eingerichtet.");
    } else {
      sendPushover("Test Message", "Pushover notifications have been set up successfully.");
    }
  }

  if (gotifyEnabled == "checked") {
    if (language == "de") {
      sendGotify("Testnachricht", "Die Gotify-Benachrichtigungen wurden erfolgreich eingerichtet.", 5);
    } else {
      sendGotify("Test Message", "Gotify notifications have been set up successfully.", 5);
    }
  }
  
  // 11) Send redirect response and restart the ESP
  server.sendHeader("Location", "/");
  server.send(303);  // HTTP redirect to status page
  delay(250);
  ESP.restart();
}

// Handle form submission save WIFI credentials
void handleSaveWiFi() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    preferences.begin(PREF_NS, false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    server.send(200, "text/html", "<h1>Saved! Restarting...</h1>");
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing data");
  }
}

// Handle factory reset
void handleFactoryReset() {
  preferences.begin(PREF_NS, false);
  preferences.clear();  // Deletes all keys in the namespace"
  preferences.end();

  server.send(200, "text/html", "<h1>Factory reset performed. Restarting...</h1>");
  delay(2000);
  ESP.restart();
}

// NTP sync
void syncDateTime() {
  // syncing NTP time
  logPrint("[DATETIME] syncing NTP time to server: " + ntpServer + " TZ: " + tzInfo);
  configTzTime(tzInfo.c_str(), ntpServer.c_str());  // Synchronizing ESP32 system time with NTP
  if (getLocalTime(&local, 10000)) { // Try to synchronize up to 10s
    // set actual date in global variable actualDate
    char readDate[11]; // YYYY-MM-DD + null
    strftime(readDate, sizeof(readDate), "%Y-%m-%d", &local);
    lastSyncDay = local.tm_mday;
    char buf[64];
    strftime(buf, sizeof(buf), "now: %d.%m.%y  Zeit: %H:%M:%S", &local);
    logPrint(String("[DATETIME] ") + buf);  // Format date print output
  } else {
    logPrint("[DATETIME] Failed to obtain time");
  }
}

// calculate elapsed days and weeks from defined date
void calculateTimeSince(const String& startDate, int &days, int &weeks) {
  struct tm tmStart = { 0 };
  int y, m, d;
  sscanf(startDate.c_str(), "%d-%d-%d", &y, &m, &d);
  tmStart.tm_mday = d;
  tmStart.tm_mon = m - 1;
  tmStart.tm_year = y - 1900;
  tmStart.tm_hour = 0;
  tmStart.tm_min = 1;
  time_t startEpoch = mktime(&tmStart);
  time_t nowEpoch = time(nullptr);
  long diffSec = nowEpoch - startEpoch;
  days = (diffSec / 86400) + 1;
  weeks = (days / 7) + 1;

  //logPrint(String("Running since ") + String(days) + String(" days (") + String(weeks) + String(" weeks + ")  + String(" days)\n"));
}

// Convert minutes to milliseconds (int return type)
int minutesToMilliseconds(int minutes) {
    return minutes * 60 * 1000;
}

// Convert seconds to milliseconds (int return type)
int secondsToMilliseconds(int seconds) {
    return seconds * 1000;
}

// CSV: ts_ms,tempC,hum,vpd\n
static void appendLog(uint32_t ts, float t, float h, float v) {
  File f = LittleFS.open(LOG_PATH, FILE_APPEND);
  if (!f) { logPrint("appendLog: open failed"); return; }
  logPrint("[LITTLEFS] Loging data: " + String(ts) + "," + String(t,1) + "," + String(h,0) + "," + String(v,1));
  // Zahlen schlank formatieren
  String line;
  line.reserve(40);
  line += String(ts); line += ',';
  line += String(t, 2); line += ',';
  line += String(h, 0); line += ',';
  line += String(v, 3); line += '\n';
  if(f.print(line)){
        logPrint("[LITTLEFS] " + String(LOG_PATH) + " file written");
    } else {
        logPrint("[LITTLEFS] " + String(LOG_PATH) + " write failed");
    }
    f.close();
}

// Compaction: discard everything < (now-RETAIN_MS)
static void compactLog() {
  const uint32_t now = millis();
  const uint32_t cutoff = (now > RETAIN_MS) ? (now - RETAIN_MS) : 0;

  File in = LittleFS.open(LOG_PATH, FILE_READ);
  if (!in) return; // nichts zu tun

  File out = LittleFS.open("/envlog.tmp", FILE_WRITE);
  if (!out) { in.close(); return; }

  // Zeilenweise kopieren
  String line;
  while (in.available()) {
    line = in.readStringUntil('\n');
    if (line.length() < 5) continue;
    // ts am Anfang extrahieren
    int c1 = line.indexOf(',');
    if (c1 <= 0) continue;
    uint32_t ts = strtoul(line.substring(0, c1).c_str(), nullptr, 10);
    if (ts >= cutoff) {
      out.print(line); out.print('\n');
    }
  }
  in.close();
  out.close();

  LittleFS.remove(LOG_PATH);
  LittleFS.rename("/envlog.tmp", LOG_PATH);
}

// calculate elapsed days and weeks from defined unix timestamp
float calcVPD(float valLastTemperature,float valOffsetLeafTemperature , float valLastHumidity) {
      float FT = valLastTemperature;
      float FARH = valLastHumidity;
      float FLTO = valOffsetLeafTemperature;
      float FLT = FT + FLTO;
      float VPLEAF = (610.7 * pow(10, (7.5 * FLT) / (237.3 + FLT)) / 1000);
      float ASVPF = (610.7 * pow(10, (7.5 * FT) / (237.3 + FT)) / 1000);
      float VPAIR = (FARH / 100) * ASVPF;
      float VPD = VPLEAF - VPAIR;
      return VPD;
}

// Store new reading and update running sums
void addReading(float temp, float hum, float vpd) {
  // Remove old values from sums
  sumTemp -= temps[index_pos];
  sumHum  -= hums[index_pos];
  sumVPD  -= vpds[index_pos];
  sumWaterTemp -= waterTemps[index_pos];

  // Add new values
  temps[index_pos] = temp;
  hums[index_pos]  = hum;
  vpds[index_pos]  = vpd;
  waterTemps[index_pos] = DS18B20STemperature;

  sumTemp += temp;
  sumHum  += hum;
  sumVPD  += vpd;
  sumWaterTemp += waterTemps[index_pos];

  // Move index (circular buffer)
  index_pos = (index_pos + 1) % NUM_VALUES;

  // Count how many values are valid (up to NUM_VALUES)
  if (count < NUM_VALUES) {
    count++;
  }
}

// Averages
float avgTemp() {
  if (count == 0) return 0.0f;
  return sumTemp / count;
}

float avgHum() {
  if (count == 0) return 0.0f;
  return sumHum / count;
}

float avgVPD() {
  if (count == 0) return 0.0f;
  return sumVPD / count;
}

float avgWaterTemp() {
  if (count == 0) return 0.0f;
  return sumWaterTemp / count;
}

// Read sensor temperature, humidity and vpd and DS18B20 water temperature
String readSensorData() {
  // read DS18B20 water temperature if enabled
  if (DS18B20) {
    sensors.requestTemperatures();
    float dsTemp = sensors.getTempCByIndex(0);
    // only update global water temp if valid
    if (dsTemp != DEVICE_DISCONNECTED_C && dsTemp > -100.0) {
      DS18B20STemperature = dsTemp;
    } else {
      logPrint("[SENSOR] DS18B20 sensor error or disconnected. Please check wiring.");
    }
  }
  
  // we will ALWAYS return valid JSON, even if BME not available or not time yet
  unsigned long now = millis();
  struct tm timeinfo;
  char timeStr[32] = "";
  if (getLocalTime(&timeinfo)) {
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  }

  // toggle status LED as before
  if (bmeAvailable) {
    if (now - previousMillis >= blinkInterval) {
      previousMillis = now;
      ledState = !ledState;
      digitalWrite(STATUS_LED_PIN, ledState);
    }

    // time to read fresh BME values?
    if (now - lastRead >= READ_INTERVAL_MS) {
      lastRead = now;

      lastTemperature = bme.readTemperature();
      lastHumidity    = bme.readHumidity();
      lastVPD         = calcVPD(lastTemperature, offsetLeafTemperature, lastHumidity);

      // log every 60s if valid
      if ((now - lastLog >= LOG_INTERVAL_MS) && !isnan(lastTemperature) && !isnan(lastHumidity) && !isnan(lastVPD)) {
        appendLog(now, lastTemperature, lastHumidity, lastVPD);
        lastLog = now;
        logPrint("[LITTLEFS] Logged data to " + String(LOG_PATH));
      }

      // compact hourly
      static unsigned long lastCompact = 0;
      if (now - lastCompact >= COMPACT_EVERY_MS) {
        compactLog();  // keeps only the last 48 hours
        lastCompact = now;
        logPrint("[LITTLEFS] Compacted log file " + String(LOG_PATH));
      }

      // hier könntest du auch deine "addReading(...)" für die 1h-Mittel aufrufen
      // z.B.: addReading(lastTemperature, lastHumidity, lastVPD, DS18B20STemperature);
    }
  }

  // === JSON BUILDING (always) ===
  String json = "{\n";
  // ---- current readings ----
  if (!isnan(curPhase)) {
    json += "\"curGrowPhase\":" + String(curPhase) + ",\n";
  } else {
    json += "\"curGrowPhase\":null,\n";
  } 
  if (!isnan(lastTemperature)) {
    json += "\"curTemperature\":" + String(lastTemperature, 1) + ",\n";
  } else {
    json += "\"curTemperature\":null,\n";
  } 
  if (!isnan(DS18B20STemperature)) {
    json += "\"cur" + DS18B20Name + "\":" + String(DS18B20STemperature, 1) + ",\n";
  } else {
    json += "\"cur" + DS18B20Name + "\":null,\n";
  }
  if (!isnan(lastHumidity)) {
    json += "\"curHumidity\":" + String(lastHumidity, 0) + ",\n";
  } else {
    json += "\"curHumidity\":null,\n";
  }
  if (!isnan(lastVPD)) {
    json += "\"curVpd\":" + String(lastVPD, 1) + ",\n";
  } else {
    json += "\"curVpd\":null,\n";
  }
  if (!isnan(irrigationRuns)) {
    json += "\"curIrrigationRuns\":" + String(irrigationRuns) + ",\n";
  } else {
    json += "\"curIrrigationRuns\":null,\n";
  }
  if (!isnan(tankLevel)) {
    json += "\"curTankLevel\":" + String(tankLevel, 0) + ",\n";
  } else {
    json += "\"curTankLevel\":null,\n";
  }
  if (!isnan(tankLevelCm)) {
    json += "\"curTankLevelDistance\":" + String(tankLevelCm, 0) + ",\n";
  } else {
    json += "\"curTankLevelDistance\":null,\n";
  }
  if (wTimeLeft.length() > 0) {
    json += "\"curTimeLeftIrrigation\":\"" + String(wTimeLeft) +  "\",\n";
  } else {
    json += "\"curTimeLeftIrrigation\":null,\n";
  }
  if (!isnan(avgTemp())) {
    json += "\"avgTemperature\":" + String(avgTemp(), 1)  + ",\n";
  } else {
    json += "\"avgTemperature\":null,\n";
  }
  if (!isnan(avgWaterTemp())) {
    json += "\"avg" + DS18B20Name + "\":" + String(avgWaterTemp(), 1) + ",\n";
  } else {
    json += "\"avg" + DS18B20Name + "\":null,\n";
  }
  if (!isnan(avgHum())) {
    json += "\"avgHumidity\":" + String(avgHum(), 0) + ",\n";
  } else {
    json += "\"avgHumidity\":null,\n";
  }
  if (!isnan(avgVPD())) {
    json += "\"avgVpd\":" + String(avgVPD(), 1) + ",\n";
  } else {
    json += "\"avgVpd\":null,\n";
  }

  // ---- relays ----
  // returns e.g. "relays":[true,false,true,false]
  json += "\"relays\":[";
  for (int i = 0; i < NUM_RELAYS; i++) {
    int state = digitalRead(relayPins[i]); // depends on your wiring (LOW=on or HIGH=on)
    // here we assume HIGH=on
    bool on = (state == HIGH);
    json += (on ? "true" : "false");
    if (i < NUM_RELAYS - 1) json += ",";
  }
  json += "],\n";
  
  // ---- Shelly Main Switch ----
  if (!shMain.ok) {
    logPrint("[API] MAIN SWITCH request not ok");
    json += "\"shellyMainSwitchStatus\":false,\n";
    json += "\"shellyMainSwitchPower\":null,\n";
    json += "\"shellyMainSwitchTotalWh\":null,\n";
  } else {
    json += "\"shellyMainSwitchStatus\":" + String(shMain.isOn ? "true" : "false") + ",\n";
    if (!isnan(shMain.powerW) && !isinf(shMain.powerW)) {
      json += "\"shellyMainSwitchPower\":" + String(shMain.powerW, 2) + ",\n";
    } else {
      json += "\"shellyMainSwitchPower\":null,\n";
    }
    if (!isnan(shMain.energyWh) && !isinf(shMain.energyWh)) {
      json += "\"shellyMainSwitchTotalWh\":" + String(shMain.energyWh, 2) + ",\n";
    } else {
      json += "\"shellyMainSwitchTotalWh\":null,\n";
    }
  }

  // ---- Shelly Heater ----
  if (!shHeater.ok) {
    logPrint("[API] HEATER request not ok");
    json += "\"shellyHeaterStatus\":false,\n";
    json += "\"shellyHeaterPower\":null,\n";
    json += "\"shellyHeaterTotalWh\":null,\n";
  } else {
    json += "\"shellyHeaterStatus\":" + String(shHeater.isOn ? "true" : "false") + ",\n";
    // powerW can be NAN if parsing failed -> output null
    if (!isnan(shHeater.powerW) && !isinf(shHeater.powerW)) {
      json += "\"shellyHeaterPower\":" + String(shHeater.powerW, 2) + ",\n";
    } else {
      json += "\"shellyHeaterPower\":null,\n";
    }
    if (!isnan(shHeater.energyWh) && !isinf(shHeater.energyWh)) {
      json += "\"shellyHeaterTotalWh\":" + String(shHeater.energyWh, 2) + ",\n";
    } else {
      json += "\"shellyHeaterTotalWh\":null,\n";
    }
  }

  // ---- Shelly Humidifier ----
  if (!shHumidifier.ok) {
    logPrint("[API] HUMIDIFIER request not ok");
    json += "\"shellyHumidifierStatus\":false,\n";
    json += "\"shellyHumidifierPower\":null,\n";
    json += "\"shellyHumidifierTotalWh\":null,\n";
  } else {
    json += "\"shellyHumidifierStatus\":" + String(shHumidifier.isOn ? "true" : "false") + ",\n";
    if (!isnan(shHumidifier.powerW) && !isinf(shHumidifier.powerW)) {
      json += "\"shellyHumidifierPower\":" + String(shHumidifier.powerW, 2) + ",\n";
    } else {
      json += "\"shellyHumidifierPower\":null,\n";
    }
    if (!isnan(shHumidifier.energyWh) && !isinf(shHumidifier.energyWh)) {
      json += "\"shellyHumidifierTotalWh\":" + String(shHumidifier.energyWh, 2) + ",\n";
    } else {
      json += "\"shellyHumidifierTotalWh\":null,\n";
    }
  }

  // ---- Shelly Fan ----
  if (!shFan.ok) {
    logPrint("[SHELLY] FAN request not ok");
    json += "\"shellyFanStatus\":false,\n";
    json += "\"shellyFanPower\":null,\n";
    json += "\"shellyFanTotalWh\":null,\n";
  } else {
    json += "\"shellyFanStatus\":" + String(shFan.isOn ? "true" : "false") + ",\n";
    if (!isnan(shFan.powerW) && !isinf(shFan.powerW)) {
      json += "\"shellyFanPower\":" + String(shFan.powerW, 2) + ",\n";
    } else {
      json += "\"shellyFanPower\":null,\n";
    }
    if (!isnan(shFan.energyWh) && !isinf(shFan.energyWh)) {
      json += "\"shellyFanTotalWh\":" + String(shFan.energyWh, 2) + ",\n";
    } else {
      json += "\"shellyFanTotalWh\":null,\n";
    }
  }

  // captured time
  json += "\"captured\":\"" + String(timeStr)  + "\"\n";

  json += "}";

  return json;
}

// check HCSR04 sensor
float pingTankLevel(uint8_t trigPin, uint8_t echoPin,
                     uint32_t timeout_us = 30000,   // ~5m max, praktisch weniger
                     uint8_t samples = 3) {         // Mittelwert über N Samples
  // Pins konfigurieren (idempotent, stört nicht wenn öfter aufgerufen)
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  float sum = 0.0f;
  uint8_t ok = 0;

  for (uint8_t i = 0; i < samples; i++) {
    // Trigger-Puls (10 µs HIGH)
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Echo-Pulsdauer messen
    unsigned long duration = pulseIn(echoPin, HIGH, timeout_us);
    if (duration == 0) {
      // Timeout -> ungültig
      continue;
    }

    // Schallgeschwindigkeit ~343 m/s => 0.0343 cm/µs
    // Hin- und Rückweg => /2
    float cm = (duration * 0.0343f) / 2.0f;

    // Plausibilitätsfilter (HC-SR04 typ. 2..400cm)
    if (cm >= 2.0f && cm <= 400.0f) {
      sum += cm;
      ok++;
    }

    delay(10); // kleine Pause zwischen Messungen
  }

  if (ok == 0) return -1.0f;
  return sum / ok;
}

static void handleHistory() {
  // Query-Parameter
  uint32_t hours = 48;
  uint32_t maxPts = 1500;
  if (server.hasArg("hours"))  hours  = std::max<uint32_t>(1, server.arg("hours").toInt());
  if (server.hasArg("max"))    maxPts = std::max<uint32_t>(50, server.arg("max").toInt());

  uint32_t nowms = millis();
  uint32_t from  = (hours >= 596523) ? 0 : (nowms - hours * 3600UL * 1000UL); // guard overflow

  // 1. Datei öffnen und erst mal zählen (wie viele im Zeitraum?)
  auto f = LittleFS.open(LOG_PATH, FILE_READ);
  if (!f) { server.send(200, "application/json", "[]"); 
    logPrint(String("[LITTLEFS]: ") + LOG_PATH + " open failed!");
    return; 
  }

  // Zählen
  size_t count = 0;
  {
    String line;
    while (f.available()) {
      line = f.readStringUntil('\n');
      int c1 = line.indexOf(',');
      if (c1 <= 0) continue;
      uint32_t ts = strtoul(line.substring(0, c1).c_str(), nullptr, 10);
      if (ts >= from) count++;
    }
  }

  // stride berechnen
  size_t stride = (count > maxPts) ? (count / maxPts + ((count % maxPts) ? 1 : 0)) : 1;
  f.seek(0);

  // 2. JSON streamen
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "[");
  String chunk; chunk.reserve(64);

  size_t i = 0, kept = 0;
  bool first = true;

  String line;
  while (f.available()) {
    line = f.readStringUntil('\n');
    int c1 = line.indexOf(',');
    if (c1 <= 0) continue;
    uint32_t ts = strtoul(line.substring(0, c1).c_str(), nullptr, 10);
    if (ts < from) continue;

    if ((i++ % stride) != 0) continue; // ausdünnen

    // Felder parsen
    int c2 = line.indexOf(';', c1 + 1);
    int c3 = line.indexOf(';', c2 + 1);
    if (c2 < 0 || c3 < 0) continue;

    float t = atof(line.substring(c1 + 1, c2).c_str());
    float h = atof(line.substring(c2 + 1, c3).c_str());
    float v = atof(line.substring(c3 + 1).c_str());

    if (!first) server.sendContent(";");
    first = false;

    chunk = "{\"ts\":";
    chunk += String(ts);
    chunk += ";\"tempC\":";
    chunk += String(t, 2);
    chunk += ";\"hum\":";
    chunk += String(h, 0);
    chunk += ";\"vpd\":";
    chunk += String(v, 3);
    chunk += "}";
    server.sendContent(chunk);
    kept++;
  }
  f.close();
  server.sendContent("]");
}

static void handleDownloadHistory() {
  if (!LittleFS.exists(LOG_PATH)) { server.send(404, "text/plain", "No log file"); return; }
  File f = LittleFS.open(LOG_PATH, FILE_READ);
  if (!f) { server.send(500, "text/plain", "Open failed"); return; }

  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=envlog.csv");
  server.sendHeader("Cache-Control", "no-store");
  server.streamFile(f, "text/csv");
  f.close();
}

static void handleDeleteLog() {
  if (LittleFS.exists(LOG_PATH)) {
    LittleFS.remove(LOG_PATH);
    server.send(200, "text/html", "<html><body>Gel&ouml;scht <a href=\"/\">Back</a></body></html>");
    logPrint("[WEB] CSV deleted: " + String(LOG_PATH));
  } else {
    server.send(404, "text/html", "<html><body>No CSV found. <a href=\"/\">Back</a></body></html>");
  }
}

void handleApiLogBuffer() {
  String txt; txt.reserve(4096);
  for (const auto& line : logBuffer) {
    txt += line; txt += '\n';
  }
  server.send(200, "text/plain; charset=utf-8", txt);
}

void handleClearLog() {
  logBuffer.clear();
  server.send(204); // No Content
}

void handleDownloadLog() {
  String txt; txt.reserve(8192);
  for (const auto& line : logBuffer) { txt += line; txt += '\n'; }
  server.sendHeader("Content-Type", "text/plain; charset=utf-8");
  server.sendHeader("Content-Disposition", "attachment; filename=weblog.txt");
  server.send(200, "text/plain; charset=utf-8", txt);
}

// helper: read actual relay pin and convert to bool  
bool isRelayOn(int idx) {
  return digitalRead(relayPins[idx]) == HIGH;
}

// set relay state by index
void setRelay(int idx, bool on) {
  if (idx < 0 || idx >= NUM_RELAYS) return;
  digitalWrite(relayPins[idx], on ? HIGH : LOW); // <-- so!
  relayStates[idx] = on;
}

// toggle relay state by index and return new state as JSON
void handleRelayToggleIdx(int idx) {
  bool cur = isRelayOn(idx);
  bool next = !cur;
  setRelay(idx, next);

  String res = "{";
  res += "\"id\":" + String(idx + 1);
  res += ",\"state\":" + String(next ? "true" : "false");
  res += "}";
  server.send(200, "application/json", res);
}

void handleRelayIrrigationIdx(int idx) {
  setRelay(idx, true);
  relayOffTime[idx] = millis() + 10000;
  relayActive[idx] = true;

  String res = "{";
  res += "\"id\":" + String(idx + 1);
  res += ",\"state\":true";
  res += "}";
  server.send(200, "application/json", res);
}

void handleStartWatering() {
  
  if (irrigationRuns == 0) {
    // calculate number of irrigation runs
    float wateringSecond = irrigation / 10;
    float wateringTask = wateringSecond * timePerTask;
    irrigationRuns = wateringTask / amountOfWater;

    logPrint("[IRRIGATION] Starting watering: " + String(irrigation) + " ml in " + String(irrigationRuns) + " runs of " + String(amountOfWater) + " ml each.");

    if (language == "de") {
      sendPushover("Bewässerung startet. Dauer: " + calculateEndtimeWatering(), "Bewässerung startet.");
    } else {
      sendPushover("Irrigation started. Duration: " + calculateEndtimeWatering(), "Irrigation started.");
    }

    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    irrigationRuns > 0;
    logPrint("[IRRIGATION] No irrigation configured. Aborting watering.");
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

float calculateTankPercent(float current, float minTank, float maxTank) {
  if (maxTank == minTank) return 0;
  float percent = (current - minTank) / (maxTank - minTank) * 100;
  percent = fmax(0, fmin(100, percent));
  return round(percent);
}

void readTankLevel() {
  tankLevelCm = pingTankLevel(TRIG, ECHO);
  if (!isnan(tankLevelCm)) {
    logPrint("[TANK LEVEL] Current distance to water: " + String(tankLevelCm, 0) + " cm");
    server.sendHeader("Location", "/");
    server.send(303);
    if (maxTank == 0 || maxTank == minTank) return;
    tankLevel = calculateTankPercent(tankLevelCm, minTank, maxTank);
    logPrint("[TANK LEVEL] Current tank level: " + String(tankLevel) + " %");
  } else {
    logPrint("[TANK LEVEL] Error reading tank level.");
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

String calculateEndtimeWatering() {
  unsigned long totalIrrigationTimeMs = irrigationRuns * ((secondsToMilliseconds(timePerTask) * 3) + minutesToMilliseconds(betweenTasks));
  unsigned long totalSeconds = totalIrrigationTimeMs / 1000;
  unsigned long totalMinutes = totalSeconds / 60;
  unsigned long hours = totalMinutes / 60;
  unsigned long minutes = totalMinutes % 60;

  logPrint("[IRRIGATION] Estimated total irrigation time: " + String(hours) + " hours and " + String(minutes) + " minutes.");

  return String(hours) + ":" + String(minutes);
}

// Send notification via Pushover
bool sendPushover(const String& message, const String& title) {
  if (pushoverSent) {
    WiFiClientSecure client;
    client.setInsecure(); // simpel & schnell (ohne Zertifikatsprüfung)

    HTTPClient https;
    if (!https.begin(client, "https://api.pushover.net/1/messages.json")) {
      return false;
    }

    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body =
      "token=" + String(pushoverAppKey) +
      "&user="  + String(pushoverUserKey) +
      "&device=" + String(pushoverDevice) +
      "&title=" + title +
      "&priority=1" +
      "&message=" + message;

    int code = https.POST(body);
    String resp = https.getString();
    https.end();

    logPrint("[MESSAGE] Pushover HTTP send code: " + String(code));
    logPrint("[MESSAGE] " + resp);

    return (code == 200);
  } else {
    logPrint("[MESSAGE] Pushover notification not sent: disabled");
    return false;
  }
}

// Send notification via Gotify
bool sendGotify(const String& msg, const String& title, int priority) {
  if (gotifySent) {
    String GOTIFY_URL = String("https://") + gotifyServer + "/message?token=" + gotifyToken;
  
    // priority: -2 (lowest) to 10 (highest)
    WiFiClientSecure client;
    client.setInsecure(); // schnell & unkompliziert (ohne Zertifikatsprüfung)

    HTTPClient http;
    if (!http.begin(client, GOTIFY_URL.c_str())) return false;

    http.addHeader("Content-Type", "application/json");

    // JSON Body
    String body = "{";
    body += "\"title\":\"" + title + "\",";
    body += "\"message\":\"" + msg + "\",";
    body += "\"priority\":" + String(priority);
    body += "}";

    int code = http.POST(body);
    String resp = http.getString();
    http.end();

    logPrint("[MESSAGE] Gotify HTTP send code: " + String(code));
    logPrint("[MESSAGE] " + resp);
    return (code >= 200 && code < 300);
  } else {
    logPrint("[MESSAGE] Gotify notification not sent: disabled");
    return false;
  }
  
}

// =======================
// URL HELPERS
// =======================

// Build base URL without credentials (safe for logging)
static String makeBaseUrl(const String& host, uint16_t port) {
  String u = "http://" + host;
  if (port != 80) u += ":" + String(port);
  return u;
}

// =======================
// HASH HELPERS (Digest)
// =======================

// Compute hash (MD5/SHA-256) and return lowercase hex
static String hashHex(mbedtls_md_type_t mdType, const String& s) {
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(mdType);
  if (!info) return "";

  unsigned char out[64];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, info, 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char*)s.c_str(), s.length());
  mbedtls_md_finish(&ctx, out);
  mbedtls_md_free(&ctx);

  const size_t outLen = mbedtls_md_get_size(info);
  static const char* hex = "0123456789abcdef";
  String res;
  res.reserve(outLen * 2);
  for (size_t i = 0; i < outLen; i++) {
    res += hex[(out[i] >> 4) & 0xF];
    res += hex[out[i] & 0xF];
  }
  return res;
}

static String toLowerCopy(String s) { s.toLowerCase(); return s; }

// Extract parameter value from Digest header, supports:
// key="value"  OR  key=value
static String digestParam(const String& header, const String& key) {
  String h = header;
  h.trim();
  String hl = toLowerCopy(h);

  String kl = key;
  kl.trim();
  kl.toLowerCase();

  int p = hl.indexOf(kl + "=");
  if (p < 0) return "";

  p += kl.length() + 1; // skip "key="
  if (p >= h.length()) return "";

  // quoted value
  if (h[p] == '"') {
    p++;
    int e = h.indexOf('"', p);
    if (e < 0) return "";
    return h.substring(p, e);
  }

  // unquoted value until comma
  int e = h.indexOf(',', p);
  if (e < 0) e = h.length();
  String v = h.substring(p, e);
  v.trim();
  return v;
}

static String makeCnonce() {
  // Simple pseudo-random cnonce, good enough for LAN usage
  uint32_t r1 = (uint32_t)esp_random();
  uint32_t r2 = (uint32_t)esp_random();
  char buf[17];
  snprintf(buf, sizeof(buf), "%08lx%08lx", (unsigned long)r1, (unsigned long)r2);
  return String(buf);
}

// Build Digest Authorization header for RFC7616 (SHA-256) and MD5 (fallback)
// Supports "-sess" variants as well.
static String buildDigestAuth(const String& wwwAuth,
                              const String& user,
                              const String& pass,
                              const String& method,
                              const String& uri) {
  String realm  = digestParam(wwwAuth, "realm");
  String nonce  = digestParam(wwwAuth, "nonce");
  String qop    = digestParam(wwwAuth, "qop");
  String opaque = digestParam(wwwAuth, "opaque");
  String alg    = digestParam(wwwAuth, "algorithm"); // e.g. SHA-256, SHA-256-sess, MD5

  if (realm.length() == 0 || nonce.length() == 0) return "";

  bool sess = false;
  String algLower = toLowerCopy(alg);
  if (algLower.endsWith("-sess")) {
    sess = true;
    algLower.replace("-sess", "");
  }

  mbedtls_md_type_t mdType = MBEDTLS_MD_MD5;
  if (algLower == "sha-256") mdType = MBEDTLS_MD_SHA256;
  else if (algLower == "md5" || algLower.length() == 0) mdType = MBEDTLS_MD_MD5;

  // Prefer qop=auth if offered
  String qopUse = (toLowerCopy(qop).indexOf("auth") >= 0) ? "auth" : "";

  String nc = "00000001";
  String cnonce = makeCnonce();

  // HA1 = H(user:realm:pass)
  String ha1 = hashHex(mdType, user + ":" + realm + ":" + pass);
  // HA1-sess = H(HA1:nonce:cnonce)
  if (sess) {
    ha1 = hashHex(mdType, ha1 + ":" + nonce + ":" + cnonce);
  }

  // HA2 = H(method:uri)
  String ha2 = hashHex(mdType, method + ":" + uri);

  // response
  String resp;
  if (qopUse.length()) {
    resp = hashHex(mdType, ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qopUse + ":" + ha2);
  } else {
    resp = hashHex(mdType, ha1 + ":" + nonce + ":" + ha2);
  }

  // Build header line
  String h = "Authorization: Digest ";
  h += "username=\"" + user + "\", ";
  h += "realm=\"" + realm + "\", ";
  h += "nonce=\"" + nonce + "\", ";
  h += "uri=\"" + uri + "\", ";
  if (alg.length()) h += "algorithm=" + alg + ", ";

  if (qopUse.length()) {
    h += "response=\"" + resp + "\", ";
    h += "qop=" + qopUse + ", ";
    h += "nc=" + nc + ", ";
    h += "cnonce=\"" + cnonce + "\", ";
  } else {
    h += "response=\"" + resp + "\", ";
  }

  if (opaque.length()) h += "opaque=\"" + opaque + "\", ";

  if (h.endsWith(", ")) h.remove(h.length() - 2);
  h += "\r\n";
  return h;
}

// =======================
// HTTP HELPERS
// =======================

// Fetch WWW-Authenticate header using HTTPClient (no auth).
// This is used to discover whether the device requires Digest or Basic.
static bool fetchWwwAuthenticate(const String& url, String& outWwwAuth, int& outCode) {
  outWwwAuth = "";
  outCode = -1;

  HTTPClient http;
  http.setTimeout(4000);
  http.useHTTP10(true);

  const char* keys[] = {"WWW-Authenticate"};
  http.collectHeaders(keys, 1);

  if (!http.begin(url)) return false;

  int code = http.GET();
  outCode = code;

  if (code == 401) {
    outWwwAuth = http.header("WWW-Authenticate");
  }

  http.end();
  return true;
}

// Read the complete HTTP response (status line + headers + body)
static bool readAllFromClient(WiFiClient& client, String& outRaw) {
  outRaw = "";
  unsigned long start = millis();

  // Wait for first bytes (up to 2s)
  while (!client.available() && client.connected() && (millis() - start) < 2000) {
    delay(5);
  }

  // Read until connection closes or timeout (4s max)
  start = millis();
  while ((client.connected() || client.available()) && (millis() - start) < 4000) {
    while (client.available()) {
      outRaw += client.readString();
      start = millis(); // reset timeout while data arrives
    }
    delay(5);
  }

  return outRaw.length() > 0;
}

// Parse HTTP status code and body from a raw response string
static bool parseHttpResponse(const String& raw, int& outCode, String& outBody) {
  outCode = -1;
  outBody = "";

  int sep = raw.indexOf("\r\n\r\n");
  if (sep < 0) sep = raw.indexOf("\n\n");
  if (sep < 0) {
    // No headers found, treat everything as body
    outBody = raw;
    return true;
  }

  String head = raw.substring(0, sep);
  outBody = raw.substring(sep + ((raw[sep] == '\r') ? 4 : 2));

  // Status line is first header line
  int lineEnd = head.indexOf("\r\n");
  if (lineEnd < 0) lineEnd = head.indexOf('\n');
  String statusLine = (lineEnd >= 0) ? head.substring(0, lineEnd) : head;
  statusLine.trim();

  int sp1 = statusLine.indexOf(' ');
  int sp2 = statusLine.indexOf(' ', sp1 + 1);
  if (sp1 > 0 && sp2 > sp1) {
    outCode = statusLine.substring(sp1 + 1, sp2).toInt();
  }

  return true;
}

// Low-level GET using WiFiClient, with optional Authorization header line.
static bool rawHttpGet(const String& host, uint16_t port, const String& path,
                       const String& authHeaderLine,
                       int& outCode, String& outBody) {
  outCode = -1;
  outBody = "";

  WiFiClient client;
  client.setTimeout(4); // seconds

  if (!client.connect(host.c_str(), port)) {
    return false;
  }

  String p = path;
  if (!p.startsWith("/")) p = "/" + p;

  // Use HTTP/1.0 to avoid chunked transfer encoding
  String req =
    "GET " + p + " HTTP/1.0\r\n"
    "Host: " + host + "\r\n" +
    authHeaderLine +
    "Connection: close\r\n\r\n";

  client.print(req);

  String raw;
  bool ok = readAllFromClient(client, raw);
  client.stop();
  if (!ok) return false;

  parseHttpResponse(raw, outCode, outBody);
  return true;
}

// Auto-auth GET: first fetch WWW-Authenticate, then perform Digest/BASIC accordingly
static bool httpGetWithDigestAutoAuth(const String& host, uint16_t port, const String& path,
                                      const String& user, const String& pass,
                                      int& outCode, String& outBody) {
  outCode = -1;
  outBody = "";

  // Discover auth scheme (no credentials sent here)
  String url = makeBaseUrl(host, port) + path;
  String www;
  int firstCode = -1;

  if (!fetchWwwAuthenticate(url, www, firstCode)) {
    return false;
  }

  // If endpoint is open (200), just request without auth
  if (firstCode == 200) {
    return rawHttpGet(host, port, path, "", outCode, outBody);
  }

  // If unauthorized, decide scheme
  if (firstCode != 401) {
    // Unexpected (404, 500, etc.). Still try without auth to get body.
    return rawHttpGet(host, port, path, "", outCode, outBody);
  }

  String wwwLower = toLowerCopy(www);

  if (wwwLower.startsWith("digest")) {
    // Build Digest Authorization header
    String digestLine = buildDigestAuth(www, user, pass, "GET", path);
    if (digestLine.length() == 0) return false;
    return rawHttpGet(host, port, path, digestLine, outCode, outBody);
  }

  // Basic auth (rare with Shelly Gen2/3 but supported sometimes)
  if (wwwLower.startsWith("basic")) {
    // We'll avoid putting credentials in logs; we only build the header
    // NOTE: If you need Basic, you can implement base64 header here.
    // Many Shelly Gen2/3 use Digest, so this branch is typically not needed.
    return false;
  }

  return false;
}

// =======================
// MAIN: GET VALUES
// =======================
ShellyValues getShellyValues(const String& host, uint8_t gen, uint8_t switchId = 0, uint16_t port = 80) {
  ShellyValues v;

  if (WiFi.status() != WL_CONNECTED) {
    logPrint("[SHELLY] WiFi not connected");
    return v;
  }

  // Decide path for Gen1 vs Gen2/3
  String path = (gen == 1)
    ? "/status"
    : ("/rpc/Switch.GetStatus?id=" + String(switchId));

  logPrint("[SHELLY] GET " + host + ":" + String(port) + " " + path);

  int code = -1;
  String body;

  // Gen1 often uses Basic or simple auth; Gen2/3 typically uses Digest
  bool ok = httpGetWithDigestAutoAuth(host, port, path, shUser, shPass, code, body);
  if (!ok) {
    logPrint("[SHELLY] request failed");
    return v;
  }

  logPrint("[SHELLY] HTTP=" + String(code) + " bodyLen=" + String(body.length()));

  if (code != 200 || body.length() == 0) {
    logPrint("[SHELLY] Invalid response");
    return v;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    logPrint("[SHELLY] JSON parse error: " + String(err.c_str()));
    logPrint("[SHELLY] Body (first 200): " + body.substring(0, 200));
    return v;
  }

  logPrint("[SHELLY] output=" + String((bool)(doc["output"] | false)));
  logPrint("[SHELLY] apower=" + String((float)(doc["apower"] | NAN)));
  logPrint("[SHELLY] voltage=" + String((float)(doc["voltage"] | NAN)));
  logPrint("[SHELLY] current=" + String((float)(doc["current"] | NAN)));
  logPrint("[SHELLY] aenergy.total=" + String((float)(doc["aenergy"]["total"] | NAN)));

  // Parse values
  if (gen == 1) {
    v.isOn     = doc["relays"][switchId]["ison"] | false;
    v.powerW   = doc["meters"][switchId]["power"] | NAN;
    v.energyWh = doc["meters"][switchId]["total"] | NAN;
  } else {
    v.isOn     = doc["output"] | false;
    v.powerW   = doc["apower"] | NAN;
    v.voltageV = doc["voltage"] | NAN;
    v.currentA = doc["current"] | NAN;
    v.energyWh = doc["aenergy"]["total"] | NAN;
  }

  v.ok = true;
  return v;
}

// =======================
// Uses the same Digest auto-auth mechanism.
// =======================
static bool shellySwitchSet(const String& host, uint8_t gen, bool on, uint8_t switchId = 0, uint16_t port = 80) {
  if (WiFi.status() != WL_CONNECTED) return false;

  String path;
  if (gen == 1) {
    path = "/relay/" + String(switchId) + "?turn=" + String(on ? "on" : "off");
  } else {
    path = "/rpc/Switch.Set?id=" + String(switchId) + "&on=" + String(on ? "true" : "false");
  }

  logPrint("[SHELLY] SET " + host + ":" + String(port) + " " + path);

  int code = -1;
  String body;

  bool ok = httpGetWithDigestAutoAuth(host, port, path, shUser, shPass, code, body);
  logPrint("[SHELLY] HTTP=" + String(code) + " bodyLen=" + String(body.length()));
  return ok && (code == 200);
}

static bool shellySwitchOn(const String& host, uint8_t gen, uint8_t switchId = 0, uint16_t port = 80) {
  return shellySwitchSet(host, gen, true, switchId, port);
}

static bool shellySwitchOff(const String& host, uint8_t gen, uint8_t switchId = 0, uint16_t port = 80) {
  return shellySwitchSet(host, gen, false, switchId, port);
}