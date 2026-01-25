#pragma once

#include <Arduino.h>
#include <StreamString.h>
#include <WString.h>
#include <Preferences.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <function.h>
#include <config.h>

// external DallasTemperature sensor instance (defined in a single .cpp file)
extern DallasTemperature sensors;

extern WebServer server;

extern Preferences preferences;
extern ShellySettings shelly;
extern SensorReadings cur;
extern Targets target;
extern String readSensorData();
extern void calculateTimeSince(const String& dateStr, int& daysOut, int& weeksOut);
extern void logPrint(const String& msg);
extern void appendLog(unsigned long timestamp, float temperature, float humidity, float vpd);
extern void compactLog();

// forward declaration for calcVPD (defined elsewhere)
float calcVPD(float temperatureC, float leafOffset, float humidityPct);

// Forward declarations for average helper functions used by handleRoot
float avgTemp();
float avgHum();
float avgVPD();
float avgWaterTemp();

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
    html.replace("%HUMIDITY%", String(cur.humidityPct, 0));
    html.replace("%TARGETVPD%",  String(target.targetVpdKpa, 1));
    html.replace("%AVGTEMP%", String(avgTemp(), 1));
    html.replace("%AVGWATERTEMP%",  String(avgWaterTemp(), 1));
    html.replace("%AVGHUM%",  String(avgHum(), 0));
    
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

    html.replace("%SHMAINIP%", shelly.main.ip);
    if (shelly.main.gen == 1) {
      html.replace("%SHMAINSWKIND1%", "selected");
      html.replace("%SHMAINSWKIND2%", "");
      html.replace("%SHMAINSWKIND3%", "");
    } else if (shelly.main.gen == 2) {
      html.replace("%SHMAINSWKIND1%", "");
      html.replace("%SHMAINSWKIND2%", "selected");
      html.replace("%SHMAINSWKIND3%", "");
    } else if (shelly.main.gen == 3) {
      html.replace("%SHMAINSWKIND1%", "");
      html.replace("%SHMAINSWKIND2%", "");
      html.replace("%SHMAINSWKIND3%", "selected");
    } else {
      html.replace("%SHMAINSWKIND1%", "");
      html.replace("%SHMAINSWKIND2%", "");
      html.replace("%SHMAINSWKIND3%", "");
    }

    html.replace("%SHELLYHEATERIP%", shelly.heat.ip);
    if (shelly.heat.gen == 1) {
      html.replace("%SHHEATKIND1%", "selected");
      html.replace("%SHHEATKIND2%", "");
      html.replace("%SHHEATKIND3%", "");
    } else if (shelly.heat.gen == 2) {
      html.replace("%SHHEATKIND1%", "");
      html.replace("%SHHEATKIND2%", "selected");
      html.replace("%SHHEATKIND3%", "");
    } else if (shelly.heat.gen == 3) {
      html.replace("%SHHEATKIND1%", "");
      html.replace("%SHHEATKIND2%", "");
      html.replace("%SHHEATKIND3%", "selected");
    } else {
      html.replace("%SHHEATKIND1%", "");
      html.replace("%SHHEATKIND2%", "");
      html.replace("%SHHEATKIND3%", "");
    }

    html.replace("%SHELLYHUMIDIFIERIP%", shelly.hum.ip);
    if (shelly.hum.gen == 1) {
      html.replace("%SHHUMIDKIND1%", "selected");
      html.replace("%SHHUMIDKIND2%", "");
      html.replace("%SHHUMIDKIND3%", "");
    } else if (shelly.hum.gen == 2) {
      html.replace("%SHHUMIDKIND1%", "");
      html.replace("%SHHUMIDKIND2%", "selected");
      html.replace("%SHHUMIDKIND3%", "");
    } else if (shelly.hum.gen == 3) {
      html.replace("%SHHUMIDKIND1%", "");
      html.replace("%SHHUMIDKIND2%", "");
      html.replace("%SHHUMIDKIND3%", "selected");
    } else {
      html.replace("%SHHUMIDKIND1%", "");
      html.replace("%SHHUMIDKIND2%", "");
      html.replace("%SHHUMIDKIND3%", "");
    }

    html.replace("%SHELLYFANIP%", shelly.fan.ip);
    if (shelly.fan.gen == 1) {
      html.replace("%SHFANKIND1%", "selected");
      html.replace("%SHFANKIND2%", "");
      html.replace("%SHFANKIND3%", "");
    } else if (shelly.fan.gen == 2) {
      html.replace("%SHFANKIND1%", "");
      html.replace("%SHFANKIND2%", "selected");
      html.replace("%SHFANKIND3%", "");
    } else if (shelly.fan.gen == 3) {
      html.replace("%SHFANKIND1%", "");
      html.replace("%SHFANKIND2%", "");
      html.replace("%SHFANKIND3%", "selected");
    } else {
      html.replace("%SHFANKIND1%", "");
      html.replace("%SHFANKIND2%", "");
      html.replace("%SHFANKIND3%", "");
    }

    html.replace("%SHUSER%", shelly.username);
    html.replace("%SHPASSWORD%", shelly.password);

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
  shelly.main.ip   = preferences.getString(KEY_SHMAINIP, "");
  shelly.main.gen  = preferences.getInt(KEY_SHMAINGEN, 1);

  shelly.heat.ip   = preferences.getString(KEY_SHELLYHEATIP, "");
  shelly.heat.gen  = preferences.getInt(KEY_SHELLYHEATGEN, 1);

  shelly.hum.ip    = preferences.getString(KEY_SHELLYHUMIP, "");
  shelly.hum.gen   = preferences.getInt(KEY_SHELLYHUMGEN, 1);

  shelly.fan.ip    = preferences.getString(KEY_SHELLYFANIP, "");
  shelly.fan.gen   = preferences.getInt(KEY_SHELLYFANGEN, 1);

  // Shelly credentials (optional Basic Auth)
  shelly.username  = preferences.getString(KEY_SHELLYUSERNAME, "");
  shelly.password  = preferences.getString(KEY_SHELLYPASSWORD, "");

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
           " Shelly Heater Gen: " + String(shelly.heat.gen)  + " Shelly Humidifier Gen: " + String(shelly.hum.gen) + 
           " targetVPD:" + targetVPD + " curPhase:" + String(curPhase) + " Relayname1:" + relayNames[0] + 
           " Relayname2:" + relayNames[1] + " Relayname3:" + relayNames[2] + " Relayname4:" + relayNames[3] +
           " AmountOfWater:" + String(amountOfWater) + " Irrigation:" + String(irrigation));
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

// Helper function to save a string preference if the corresponding argument is present
void savePrefString(
  const char* argName,
  const char* prefKey,
  String& targetVar,
  bool logValue = true,
  const char* logLabel = nullptr
) {
  if (!server.hasArg(argName)) return;

  targetVar = server.arg(argName);
  preferences.putString(prefKey, targetVar);

  if (logLabel == nullptr) logLabel = prefKey;

  if (logValue) {
    logPrint("[PREFERENCES] " + String(logLabel) + " written = " + targetVar);
  } else {
    logPrint("[PREFERENCES] " + String(logLabel) + " updated (hidden)");
  }
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

      cur.temperatureC = bme.readTemperature();
      cur.humidityPct    = bme.readHumidity();
      cur.vpdKpa         = calcVPD(cur.temperatureC, offsetLeafTemperature, cur.humidityPct);

      // log every 60s if valid
      if ((now - lastLog >= LOG_INTERVAL_MS) && !isnan(cur.temperatureC) && !isnan(cur.humidityPct) && !isnan(cur.vpdKpa)) {
        appendLog(now, cur.temperatureC, cur.humidityPct, cur.vpdKpa);
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
  if (!isnan(cur.temperatureC)) {
    json += "\"curTemperature\":" + String(cur.temperatureC, 1) + ",\n";
  } else {
    json += "\"curTemperature\":null,\n";
  } 
  if (!isnan(cur.extTempC)) {
    json += "\"cur" + DS18B20Name + "\":" + String(cur.extTempC, 1) + ",\n";
  } else {
    json += "\"cur" + DS18B20Name + "\":null,\n";
  }
  if (!isnan(cur.humidityPct)) {
    json += "\"curHumidity\":" + String(cur.humidityPct, 0) + ",\n";
  } else {
    json += "\"curHumidity\":null,\n";
  }
  if (!isnan(cur.vpdKpa)) {
    json += "\"curVpd\":" + String(cur.vpdKpa, 1) + ",\n";
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
  if (!shelly.main.values.ok) {
    logPrint("[API] MAIN SWITCH request not ok");
    json += "\"shellyMainSwitchStatus\":false,\n";
    json += "\"shellyMainSwitchPower\":null,\n";
    json += "\"shellyMainSwitchTotalWh\":null,\n";
  } else {
    json += "\"shellyMainSwitchStatus\":" + String(shelly.main.values.isOn ? "true" : "false") + ",\n";
    if (!isnan(shelly.main.values.powerW) && !isinf(shelly.main.values.powerW)) {
      json += "\"shellyMainSwitchPower\":" + String(shelly.main.values.powerW, 2) + ",\n";
    } else {
      json += "\"shellyMainSwitchPower\":null,\n";
    }
    if (!isnan(shelly.main.values.energyWh) && !isinf(shelly.main.values.energyWh)) {
      json += "\"shellyMainSwitchTotalWh\":" + String(shelly.main.values.energyWh, 2) + ",\n";
    } else {
      json += "\"shellyMainSwitchTotalWh\":null,\n";
    }
  }

  // ---- Shelly Heater ----
  if (!shelly.heat.values.ok) {
    logPrint("[API] HEATER request not ok");
    json += "\"shellyHeaterStatus\":false,\n";
    json += "\"shellyHeaterPower\":null,\n";
    json += "\"shellyHeaterTotalWh\":null,\n";
  } else {
    json += "\"shellyHeaterStatus\":" + String(shelly.heat.values.isOn ? "true" : "false") + ",\n";
    // powerW can be NAN if parsing failed -> output null
    if (!isnan(shelly.heat.values.powerW) && !isinf(shelly.heat.values.powerW)) {
      json += "\"shellyHeaterPower\":" + String(shelly.heat.values.powerW, 2) + ",\n";
    } else {
      json += "\"shellyHeaterPower\":null,\n";
    }
    if (!isnan(shelly.heat.values.energyWh) && !isinf(shelly.heat.values.energyWh)) {
      json += "\"shellyHeaterTotalWh\":" + String(shelly.heat.values.energyWh, 2) + ",\n";
    } else {
      json += "\"shellyHeaterTotalWh\":null,\n";
    }
  }

  // ---- Shelly Humidifier ----
  if (!shelly.hum.values.ok) {
    logPrint("[API] HUMIDIFIER request not ok");
    json += "\"shellyHumidifierStatus\":false,\n";
    json += "\"shellyHumidifierPower\":null,\n";
    json += "\"shellyHumidifierTotalWh\":null,\n";
  } else {
    json += "\"shellyHumidifierStatus\":" + String(shelly.hum.values.isOn ? "true" : "false") + ",\n";
    if (!isnan(shelly.hum.values.powerW) && !isinf(shelly.hum.values.powerW)) {
      json += "\"shellyHumidifierPower\":" + String(shelly.hum.values.powerW, 2) + ",\n";
    } else {
      json += "\"shellyHumidifierPower\":null,\n";
    }
    if (!isnan(shelly.hum.values.energyWh) && !isinf(shelly.hum.values.energyWh)) {
      json += "\"shellyHumidifierTotalWh\":" + String(shelly.hum.values.energyWh, 2) + ",\n";
    } else {
      json += "\"shellyHumidifierTotalWh\":null,\n";
    }
  }

  // ---- Shelly Fan ----
  if (!shelly.fan.values.ok) {
    logPrint("[SHELLY] FAN request not ok");
    json += "\"shellyFanStatus\":false,\n";
    json += "\"shellyFanPower\":null,\n";
    json += "\"shellyFanTotalWh\":null,\n";
  } else {
    json += "\"shellyFanStatus\":" + String(shelly.fan.values.isOn ? "true" : "false") + ",\n";
    if (!isnan(shelly.fan.values.powerW) && !isinf(shelly.fan.values.powerW)) {
      json += "\"shellyFanPower\":" + String(shelly.fan.values.powerW, 2) + ",\n";
    } else {
      json += "\"shellyFanPower\":null,\n";
    }
    if (!isnan(shelly.fan.values.energyWh) && !isinf(shelly.fan.values.energyWh)) {
      json += "\"shellyFanTotalWh\":" + String(shelly.fan.values.energyWh, 2) + ",\n";
    } else {
      json += "\"shellyFanTotalWh\":null,\n";
    }
  }

  // captured time
  json += "\"captured\":\"" + String(timeStr)  + "\"\n";

  json += "}";

  return json;
}