#include "vars_registry.h"
#include "globals.h"

// These are defined in main.cpp (kept there intentionally)
extern SensorReadings cur;
extern Targets target;
extern ShellySettings shelly;

// Average helpers are defined in your runtime module.
extern float avgTemp();
extern float avgHum();
extern float avgVPD();
extern float avgWaterTemp();

// -------------- small JSON helpers --------------
static String jBool(bool v) { return v ? "true" : "false"; }

static String jNumOrNull(float v, uint8_t dec = 2) {
  if (isnan(v) || isinf(v)) return "null";
  return String((double)v, (unsigned int)dec);
}

static String jInt(int v) { return String(v); }
static String jUInt(uint32_t v) { return String(v); }

static String jStr(const String& s) {
  String out = "\"";
  out.reserve(s.length() + 2);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') out += '\\';
    out += c;
  }
  out += "\"";
  return out;
}

static String jMasked() { return jStr("********"); }

static String g_buildTag() {
    String tag = "\"BUILD_";
    tag += __DATE__;  // z.B. Jan 27 2026
    tag += "_";
    tag += __TIME__;  // z.B. 14:52:10
    tag += "\"";
    return tag;
}


// -------------- getters --------------

static String g_uptime() { return jUInt((uint32_t)(millis() / 1000UL)); }
static String g_heap() { return jUInt((uint32_t)ESP.getFreeHeap()); }
static String g_minheap() { return jUInt((uint32_t)ESP.getMinFreeHeap()); }
static String g_cpumhz() { return jUInt((uint32_t)ESP.getCpuFreqMHz()); }

static String g_wifiReady() { return jBool(wifiReady); }
static String g_espMode() { return jBool(espMode); }
static String g_ssid() { return jStr(ssidName); }

static String g_temp() { return jNumOrNull(cur.temperatureC, 1); }
static String g_hum() { return jNumOrNull(cur.humidityPct, 1); }
static String g_extTemp() { return jNumOrNull(cur.extTempC, 1); }
static String g_vpd() { return jNumOrNull(cur.vpdKpa, 2); }

static String g_tgtTemp() { return jNumOrNull(target.targetTempC, 1); }
static String g_tgtHum() { return jNumOrNull(target.targetHumPct, 1); }
static String g_tgtVpd() { return jNumOrNull(target.targetVpdKpa, 2); }

static String g_tankPct() { return jNumOrNull(tankLevel, 1); }
static String g_tankCm() { return jNumOrNull(tankLevelCm, 1); }
static String g_tankMin() { return jNumOrNull(minTank, 1); }
static String g_tankMax() { return jNumOrNull(maxTank, 1); }

static String g_avgTemp() { return jNumOrNull(avgTemp(), 1); }
static String g_avgHum() { return jNumOrNull(avgHum(), 1); }
static String g_avgVpd() { return jNumOrNull(avgVPD(), 2); }
static String g_avgWater() { return jNumOrNull(avgWaterTemp(), 1); }

// Settings shortcuts
static String g_ui_box() { return jStr(settings.ui.boxName); }
static String g_ui_lang() { return jStr(settings.ui.language); }
static String g_ui_theme() { return jStr(settings.ui.theme); }
static String g_ui_unit() { return jStr(settings.ui.unit); }
static String g_ui_timeFmt() { return jStr(settings.ui.timeFormat); }
static String g_ui_ntp() { return jStr(settings.ui.ntpServer); }
static String g_ui_tz() { return jStr(settings.ui.tzInfo); }

static String g_grow_phase() { return jInt(settings.grow.currentPhase); }
static String g_grow_tTemp() { return jNumOrNull(settings.grow.targetTemperature, 1); }
static String g_grow_leafOff() { return jNumOrNull(settings.grow.offsetLeafTemperature, 1); }
static String g_grow_tVpd() { return jNumOrNull(settings.grow.targetVPD, 2); }
static String g_grow_amtWater() { return jInt(settings.grow.amountOfWater); }
static String g_grow_irrig() { return jInt(settings.grow.irrigation); }
static String g_grow_timeTask() { return jInt(settings.grow.timePerTask); }
static String g_grow_between() { return jInt(settings.grow.betweenTasks); }

// Notifications (masked tokens)
static String g_notify_pushoverEnabled() { return jBool(settings.notify.pushoverEnabled); }
static String g_notify_pushoverAppKey() { return jMasked(); }
static String g_notify_pushoverUserKey() { return jMasked(); }
static String g_notify_pushoverDevice() { return jStr(settings.notify.pushoverDevice); }
static String g_notify_gotifyEnabled() { return jBool(settings.notify.gotifyEnabled); }
static String g_notify_gotifyServer() { return jStr(settings.notify.gotifyServer); }
static String g_notify_gotifyToken() { return jMasked(); }

// Shelly settings (mask password)
static String g_sh_main_ip() { return jStr(settings.shelly.main.ip); }
static String g_sh_main_gen() { return jInt(settings.shelly.main.gen); }
static String g_sh_heat_ip() { return jStr(settings.shelly.heat.ip); }
static String g_sh_heat_gen() { return jInt(settings.shelly.heat.gen); }
static String g_sh_hum_ip() { return jStr(settings.shelly.hum.ip); }
static String g_sh_hum_gen() { return jInt(settings.shelly.hum.gen); }
static String g_sh_fan_ip() { return jStr(settings.shelly.fan.ip); }
static String g_sh_fan_gen() { return jInt(settings.shelly.fan.gen); }
static String g_sh_user() { return jStr(settings.shelly.username); }
static String g_sh_pass() { return jMasked(); }

// Relay getters (state + name)
#define RELAY_GETTERS(i) \
  static String g_relay_state_##i() { return jBool(relayStates[i]); } \
  static String g_relay_name_##i() { return jStr(settings.relay.name[i]); } \
  static String g_relay_schen_##i() { return jBool(settings.relay.schedule[i].enabled); } \
  static String g_relay_schst_##i() { return jInt(settings.relay.schedule[i].start); } \
  static String g_relay_schen_##i##_end() { return jInt(settings.relay.schedule[i].end); }

RELAY_GETTERS(0)
RELAY_GETTERS(1)
RELAY_GETTERS(2)
RELAY_GETTERS(3)
RELAY_GETTERS(4)
RELAY_GETTERS(5)
RELAY_GETTERS(6)
RELAY_GETTERS(7)

const VarItem VARS[] = {
  {"debug.buildTag", g_buildTag, false, "debug"},
  // --- system ---
  {"sys.uptimeS", g_uptime, false, "system"},
  {"sys.freeHeap", g_heap, false, "system"},
  {"sys.minFreeHeap", g_minheap, false, "system"},
  {"sys.cpuMhz", g_cpumhz, false, "system"},

  // --- wifi ---
  {"wifi.ready", g_wifiReady, false, "wifi"},
  {"wifi.apMode", g_espMode, false, "wifi"},
  {"wifi.ssid", g_ssid, false, "wifi"},

  // --- sensors ---
  {"sensors.cur.temperatureC", g_temp, false, "sensors"},
  {"sensors.cur.humidityPct", g_hum, false, "sensors"},
  {"sensors.cur.extTempC", g_extTemp, false, "sensors"},
  {"sensors.cur.vpdKpa", g_vpd, false, "sensors"},

  // --- targets ---
  {"targets.tempC", g_tgtTemp, false, "targets"},
  {"targets.humPct", g_tgtHum, false, "targets"},
  {"targets.vpdKpa", g_tgtVpd, false, "targets"},

  // --- tank ---
  {"tank.levelPct", g_tankPct, false, "tank"},
  {"tank.levelCm", g_tankCm, false, "tank"},
  {"tank.minPct", g_tankMin, false, "tank"},
  {"tank.maxPct", g_tankMax, false, "tank"},

  // --- settings.ui ---
  {"settings.ui.boxName", g_ui_box, false, "settings.ui"},
  {"settings.ui.language", g_ui_lang, false, "settings.ui"},
  {"settings.ui.theme", g_ui_theme, false, "settings.ui"},
  {"settings.ui.unit", g_ui_unit, false, "settings.ui"},
  {"settings.ui.timeFormat", g_ui_timeFmt, false, "settings.ui"},
  {"settings.ui.ntpServer", g_ui_ntp, false, "settings.ui"},
  {"settings.ui.tzInfo", g_ui_tz, false, "settings.ui"},

  // --- settings.grow ---
  {"settings.grow.currentPhase", g_grow_phase, false, "settings.grow"},
  {"settings.grow.targetTemperature", g_grow_tTemp, false, "settings.grow"},
  {"settings.grow.offsetLeafTemperature", g_grow_leafOff, false, "settings.grow"},
  {"settings.grow.targetVPD", g_grow_tVpd, false, "settings.grow"},
  {"settings.grow.amountOfWater", g_grow_amtWater, false, "settings.grow"},
  {"settings.grow.irrigation", g_grow_irrig, false, "settings.grow"},
  {"settings.grow.timePerTask", g_grow_timeTask, false, "settings.grow"},
  {"settings.grow.betweenTasks", g_grow_between, false, "settings.grow"},

  // --- notifications ---
  {"settings.notify.pushoverEnabled", g_notify_pushoverEnabled, false, "settings.notify"},
  {"settings.notify.pushoverAppKey", g_notify_pushoverAppKey, true, "settings.notify"},
  {"settings.notify.pushoverUserKey", g_notify_pushoverUserKey, true, "settings.notify"},
  {"settings.notify.pushoverDevice", g_notify_pushoverDevice, false, "settings.notify"},
  {"settings.notify.gotifyEnabled", g_notify_gotifyEnabled, false, "settings.notify"},
  {"settings.notify.gotifyServer", g_notify_gotifyServer, false, "settings.notify"},
  {"settings.notify.gotifyToken", g_notify_gotifyToken, true, "settings.notify"},

  // --- shelly ---
  {"settings.shelly.main.ip", g_sh_main_ip, false, "settings.shelly"},
  {"settings.shelly.main.gen", g_sh_main_gen, false, "settings.shelly"},
  {"settings.shelly.heat.ip", g_sh_heat_ip, false, "settings.shelly"},
  {"settings.shelly.heat.gen", g_sh_heat_gen, false, "settings.shelly"},
  {"settings.shelly.hum.ip", g_sh_hum_ip, false, "settings.shelly"},
  {"settings.shelly.hum.gen", g_sh_hum_gen, false, "settings.shelly"},
  {"settings.shelly.fan.ip", g_sh_fan_ip, false, "settings.shelly"},
  {"settings.shelly.fan.gen", g_sh_fan_gen, false, "settings.shelly"},
  {"settings.shelly.username", g_sh_user, false, "settings.shelly"},
  {"settings.shelly.password", g_sh_pass, true, "settings.shelly"},

  // --- relays (names + states + schedule) ---
  {"relays[0].name", g_relay_name_0, false, "relays"},
  {"relays[0].state", g_relay_state_0, false, "relays"},
  {"relays[0].schedule.enabled", g_relay_schen_0, false, "relays"},
  {"relays[0].schedule.start", g_relay_schst_0, false, "relays"},
  {"relays[0].schedule.end", g_relay_schen_0_end, false, "relays"},

  {"relays[1].name", g_relay_name_1, false, "relays"},
  {"relays[1].state", g_relay_state_1, false, "relays"},
  {"relays[1].schedule.enabled", g_relay_schen_1, false, "relays"},
  {"relays[1].schedule.start", g_relay_schst_1, false, "relays"},
  {"relays[1].schedule.end", g_relay_schen_1_end, false, "relays"},

  {"relays[2].name", g_relay_name_2, false, "relays"},
  {"relays[2].state", g_relay_state_2, false, "relays"},
  {"relays[2].schedule.enabled", g_relay_schen_2, false, "relays"},
  {"relays[2].schedule.start", g_relay_schst_2, false, "relays"},
  {"relays[2].schedule.end", g_relay_schen_2_end, false, "relays"},

  {"relays[3].name", g_relay_name_3, false, "relays"},
  {"relays[3].state", g_relay_state_3, false, "relays"},
  {"relays[3].schedule.enabled", g_relay_schen_3, false, "relays"},
  {"relays[3].schedule.start", g_relay_schst_3, false, "relays"},
  {"relays[3].schedule.end", g_relay_schen_3_end, false, "relays"},

  {"relays[4].name", g_relay_name_4, false, "relays"},
  {"relays[4].state", g_relay_state_4, false, "relays"},
  {"relays[4].schedule.enabled", g_relay_schen_4, false, "relays"},
  {"relays[4].schedule.start", g_relay_schst_4, false, "relays"},
  {"relays[4].schedule.end", g_relay_schen_4_end, false, "relays"},

  {"relays[5].name", g_relay_name_5, false, "relays"},
  {"relays[5].state", g_relay_state_5, false, "relays"},
  {"relays[5].schedule.enabled", g_relay_schen_5, false, "relays"},
  {"relays[5].schedule.start", g_relay_schst_5, false, "relays"},
  {"relays[5].schedule.end", g_relay_schen_5_end, false, "relays"},

  {"relays[6].name", g_relay_name_6, false, "relays"},
  {"relays[6].state", g_relay_state_6, false, "relays"},
  {"relays[6].schedule.enabled", g_relay_schen_6, false, "relays"},
  {"relays[6].schedule.start", g_relay_schst_6, false, "relays"},
  {"relays[6].schedule.end", g_relay_schen_6_end, false, "relays"},

  {"relays[7].name", g_relay_name_7, false, "relays"},
  {"relays[7].state", g_relay_state_7, false, "relays"},
  {"relays[7].schedule.enabled", g_relay_schen_7, false, "relays"},
  {"relays[7].schedule.start", g_relay_schst_7, false, "relays"},
  {"relays[7].schedule.end", g_relay_schen_7_end, false, "relays"},
};

const size_t VARS_COUNT = sizeof(VARS) / sizeof(VARS[0]);
