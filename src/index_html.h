// index_html.h
#pragma once

// Favicon (base64-encoded)
const char FAVICON_ICO_BASE64[] PROGMEM = 
"AAABAAEAEBAAAAEAIABoBAAAFgAAACgAAAAQAAAAIAAAAAEAIAAAAAAAAAQAAAAAAAAAAAAAAAAA"
"AAAAAAD///8Af39/AFhYWAAgICAATU1NAH5+fgBQUFAAZGRkADY2NgB9fX0ARUVFAFlZWQA/Pz8A"
"bm5uAERERABra2sAenp6AEpKSgA7OzsAioqKAFtbWwBVVVUAISEhADAwMAD+/v4AAAAAAAAAAAAA"
"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIAAAACAAIAAgACAgICAAIAAgICAgACA"
"AICAgIAAgACAgICAAIAAgICAgACAAICAgIAAgACAgICAAIAAgICAgACAAICAgIAAgACAgICAAIAA"
"gICAgACAAICAgIAAgACAgICAAIAAgICAgACAAICAgIAAgACAgICAAIAAgICAgACAAICAgIAAgAAA"
"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD//wAA"
"//8AAP//AAD//wAA//8AAP//AAD//wAA";

// ------- i18n: manifest + Sprachen (separat, eigene Routen) -------
static const char I18N_MANIFEST[] PROGMEM = R"json(
{
  "default": "de",
  "languages": [
    { "code": "de", "name": "Deutsch", "dir": "ltr", "file": "de" },
    { "code": "en", "name": "English", "dir": "ltr", "file": "en" }
  ]
}
)json";

static const char I18N_DE[] PROGMEM = R"json(
{
  "app.title": "Mein Webfrontend",
  "settings.themeLight": "Hell",
  "settings.themeDark": "Dunkel",
  "status.title": "Status",
  "status.ok": "System läuft normal ✅"
}
)json";

static const char I18N_EN[] PROGMEM = R"json(
{
  "app.title": "My Web Frontend",
  "settings.themeLight": "Light",
  "settings.themeDark": "Dark",
  "status.title": "Status",
  "status.ok": "System is running ✅"
}
)json";

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<meta name="viewport" content="width=device-width, initial-scale=1">
<head>
  <title>%CONTROLLERNAME%</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <header class="header">
    <button class="hamburger" id="hamburgerBtn" data-i18n="a11y.menu" data-i18n-attr="aria-label" aria-label="Menü öffnen/schließen" aria-expanded="false" aria-controls="sidebar">☰</button>
    <div class="title" data-i18n="app.title">%CONTROLLERNAME%</div>
    <span id="unsavedHint" class="dirty-hint" hidden data-i18n="settings.unsaved"></span>
    <div class='grow-info'>%CURRENTGROW%<BR>%CURRENTPHASE%</div>
    <div id="grow-line" data-i18n-key="info.growLine"></div>
    <div class="datetime">
      <div id="headerDate"></div>
      <div id="headerTime"></div>
    </div>
  </header>
  <div class="layout">
    <nav class="sidebar" id="sidebar">
      <a class="navlink" data-page="status"   data-i18n="nav.status">Status</a>
      <a class="navlink" data-page="runsettings" data-i18n="nav.runsetting">Betriebseinstellungen</a>
      <a class="navlink" data-page="shelly" data-i18n="nav.shelly">Shelly Einstellungen</a>
      <a class="navlink" data-page="settings" data-i18n="nav.settings">Systemeinstellungen</a>
      <a class="navlink" data-page="message" data-i18n="nav.message">Push-Einstellungen</a>
      <a class="navlink" data-page="logging" data-i18n="nav.logging">Systemprotokoll</a>
      <a class="navlink" data-page="factory" data-i18n="nav.factory">Werkseinstellungen</a>
    </nav>

  <div class="overlay" id="overlay"></div>

  <main class="content" id="content">

    <!-- status section -->
    <section id="status" class="page active card">
      <h1 data-i18n="status.title">Status</h1>
      <!-- Letztes Update direkt unter dem Statustext -->
      <p class="last-update">
        <span data-i18n="status.updated">Letztes Update:</span>
        <span id="capturedSpan">--</span>
      </p>
      <p style="margin-top:10px">
        <a class="btn" href="/download/history" data-i18n="status.download">CSV herunterladen</a>
        <a class="btn" href="/deletelog" data-i18n="status.delete">CSV löschen</a>
      </p>
      
      <div class="spacer"></div>
      <h2 data-i18n="status.currentValues">aktuelle Werte</h2>
      <!-- 3 values side by side -->
      <div class="metrics-row">
        <div class="metric">
          <div class="twoinone-label">
            <div class="metric-label" data-i18n="status.lastTemperature">Temperatur</div>
            <div class="metric-value">
              <span id="tempSpan">–</span><span class="unit">°C</span>
            </div>
          </div>
          <div class="spacer"></div>
          <div class="metric-sub">
            <div class="twoinone-label">
              <span data-i18n="status.targetTemp">Soll</span>
              <div class="metric-value">
                <span id="targetTempStatus">%TARGETTEMPERATURE%</span> <span class="unit">°C</span>
              </div>
            </div>
          </div>
          <div class="spacer"></div>
          <div class="metric-submetric">
            <div class="twoinone-label">
              <div class="metric-label">%DS18B20NAME%</div>
              <div class="metric-value">
                <span id="waterTempSpan">%WATERTEMPERATURE%</span><span class="unit">°C</span>
            </div>
          </div>
        </div>
        </div>

        <div class="metric">
          <div class="twoinone-label">
            <div class="metric-label" data-i18n="status.lasthumidity">rel. Feuchte</div>
            <div class="metric-value">
              <span id="humSpan">–</span><span class="unit">%</span>
            </div>
          </div>
        </div>

        <div class="metric">
          <div class="twoinone-label">
            <div class="metric-label" data-i18n="status.lastvpd">VPD</div>
            <div class="metric-value">
              <span id="vpdSpan">–</span><span class="unit">kPa</span>
            </div>
          </div>
          <div class="spacer"></div>
          <div class="metric-sub">
            <div class="twoinone-label">
              <span data-i18n="status.targetVpd">Soll</span>
              <div class="metric-value">
                <span id="targetVpdStatus">%TARGETVPD%</span> <span class="unit">kPa</span>
              </div>
            </div>
          </div>
          <div class="spacer"></div>
          <div class="metric-sub">
            <div class="twoinone-label">
              <span data-i18n="runsetting.offsetLeafTemperature">Offset Blatttemperatur:</span>
              <div class="metric-value">
                <span id="leafTempStatus">%LEAFTEMPERATURE%</span> <span class="unit">°C</span>
              </div>
            </div>
          </div>
        </div>
      </div>
      <h2 data-i18n="status.averagesLastHour">Durchschnittswerte der letzten Stunde</h2>
      <div class="metrics-row averages-row">
        <div class="metric">
          <div class="twoinone-label">
            <div class="metric-label" data-i18n="status.avgTemperature">Ø Temperatur</div>
            <div class="metric-value">
              <span id="avgTempSpan">%AVGTEMP%</span><span class="unit">°C</span>
            </div>
          </div>
          <div class="spacer"></div>
          <div class="twoinone-label">
            <div class="metric-label">Ø</div><div class="metric-label">%DS18B20NAME%</div>
            <div class="metric-value">
              <span id="avgWaterTempSpan">%AVGWATERTEMP%</span><span class="unit">°C</span>
            </div>
          </div>
        </div>
        <div class="metric">
          <div class="twoinone-label">
            <div class="metric-label" data-i18n="status.avgHumidity">Ø rel. Feuchte</div>
            <div class="metric-value">
              <span id="avgHumSpan">%AVGHUM%</span><span class="unit">%</span>
            </div>
          </div>
        </div>
        <div class="metric">
          <div class="twoinone-label">
            <div class="metric-label" data-i18n="status.avgVpd">Ø VPD</div>
            <div class="metric-value">
              <span id="avgVpdSpan">%AVGVPD%</span><span class="unit">kPa</span>
            </div>
          </div>
        </div>
      </div>
      <div class="metrics-row averages-row">
      <!-- ... wie oben eingefügt ... -->
    </div>

    <h2 data-i18n="status.relayIrrigation">Bewässerungssteuerung</h2>
    <div class="relay-row" id="pumpRow">
      <div class="relay-card" data-relay="6">
        <div class="relay-title"  data-i18n="status.irrigationPump1">Pumpe 1</div>
        <div class="relay-status" id="relay-Status6"></div>
        <div class="spacer"></div>
        <button class="primary" data-i18n="status.relayOn" onclick="onForTenSec(6)">Toggle</button>
      </div>
      <div class="relay-card" data-relay="7">
        <div class="relay-title" data-i18n="status.irrigationPump2">Pumpe 2</div>
        <div class="relay-status" id="relay-Status7"></div>
        <div class="spacer"></div>
        <button class="primary" data-i18n="status.relayOn" onclick="onForTenSec(7)">Toggle</button>
      </div>
      <div class="relay-card" data-relay="8">
        <div class="relay-title" data-i18n="status.irrigationPump3">Pumpe 3</div>
        <div class="relay-status" id="relay-Status8"></div>
        <div class="spacer"></div>
        <button class="primary" data-i18n="status.relayOn" onclick="onForTenSec(8)">Toggle</button>
      </div>
      <div class="relay-card" data-relay="watering">
        <div class="relay-title" data-i18n="status.watering">Bewässerung</div>
        <div class="metric-value">
          <span id="irrigationSpan"  style="font-size: 16px;">-</span><span class="unit" data-i18n="status.wateringLeft"> verbleibend</span>
        </div>
        <div class="metric-value">
          <span class="unit" data-i18n="status.endIn" style="font-size: 16px;">Ende in </span><span id="irTimeLeftSpan"  style="font-size: 16px;"></span>
        </div> 
        <div class="spacermini"></div>
        <button class="primary" onclick="startWatering()">Toggle</button>
      </div>
      <div class="relay-card" data-relay="TankFilling">
        <div class="relay-title" data-i18n="status.tank">Tank Füllung</div>
        <div class="metric-value">
          <span id="tankLevelSpan" style="font-size: 16px;">–</span><span class="unit" style="font-size: 16px;">&nbsp;%</span>
        </div>
        <div class="metric-value">
          <span id="tankCMDistanceSpan" style="font-size: 16px;">–</span><span class="unit" style="font-size: 16px;">&nbsp;cm</span>
        </div>
        <div class="spacermini"></div>
        <button class="primary" data-i18n="status.pingTank" onclick="pingTank()">Ping</button>
      </div>
    </div>

    <div class="spacer"></div>
    <h2 data-i18n="status.relayControl">Relais Steuerung</h2>
    <div class="relay-row" id="relayRow">
      <div class="relay-card" data-relay="1">
        <div class="relay-title">%RELAYNAMES1%</div>
        <div class="relay-status" id="relay-Status1"></div>
        <div class="spacer"></div>
        <button class="primary" data-i18n="status.toggleRelay" onclick="toggleRelay(1)">Toggle</button>
      </div>
      <div class="relay-card" data-relay="2">
        <div class="relay-title">%RELAYNAMES2%</div>
        <div class="relay-status" id="relay-Status2"></div>
        <div class="spacer"></div>
        <button class="primary" data-i18n="status.toggleRelay" onclick="toggleRelay(2)">Toggle</button>
      </div>
      <div class="relay-card" data-relay="3">
        <div class="relay-title">%RELAYNAMES3%</div>
        <div class="relay-status" id="relay-Status3"></div>
        <div class="spacer"></div>
        <button class="primary" data-i18n="status.toggleRelay" onclick="toggleRelay(3)">Toggle</button>
      </div>
      <div class="relay-card" data-relay="4">
        <div class="relay-title">%RELAYNAMES4%</div>
        <div class="relay-status" id="relay-Status4"></div>
        <div class="spacer"></div>
        <button class="primary" data-i18n="status.toggleRelay" onclick="toggleRelay(4)">Toggle</button>
      </div>
      <div class="relay-card" data-relay="5">
        <div class="relay-title">%RELAYNAMES5%</div>
        <div class="relay-status" id="relay-Status5"></div>
        <div class="spacer"></div>
        <button class="primary" data-i18n="status.toggleRelay" onclick="toggleRelay(5)">Toggle</button>
      </div>
    </div>

    <div class="spacer"></div>
    <h2 data-i18n="status.shellyControl">Shelly Steuerung</h2>
    <div class="relay-row" id="relayRow">
      <div class="relay-card" data-relay="shellyHeater">
        <div class="relay-title" data-i18n="status.shellyHeater">Heizung</div>
        <div class="relay-status" id="shellyHeaterStatus"></div>
        <div class="info">%SHELLYHEATERIP%</div>
        <button class="primary" data-i18n="status.toggleRelay" onclick="toggleShellyHeater()">Toggle</button>
      </div>
      <div class="relay-card" data-relay="shellyHumidifier">
        <div class="relay-title" data-i18n="status.shellyHumidifier">Luftbefeuchter</div>
        <div class="relay-status" id="shellyHumidifierStatus"></div>
        <div class="info">%SHELLYHUMIDIFIERIP%</div>
        <button class="primary" data-i18n="status.toggleRelay" onclick="toggleShellyHumidifier()">Toggle</button>
      </div>
    </div>
    </section>
    
    <!-- shellysettings section -->
    <form action="/saveshellysettings" method="POST">
    <section id="shelly" class="page card">
      <h1 data-i18n="shelly.title">Shelly Einstellungen</h1>
      <h2 data-i18n="status.shellyDevices">Shelly Geräte</h2>
      <div class="form-group">
        <label for="shellyIP" data-i18n="shelly.shellyIPHeater">Shelly IP Adresse für Heizung:</label>
        <div class="twoinone-label">
          <input name="webShellyHeatIP" id="shellyIP" style="width: 300px;" type="text" value="%SHELLYHEATERIP%">
          <select name="webShellyHeaterHostKind" style="width: 80px; id="shellyHeaterHostKind">
            <option value="1" %SHELLYHEATERHOSTKIND_IPV4%>Gen1</option>
            <option value="2" %SHELLYHEATERHOSTKIND_IPV6%>Gen2</option>
            <option value="3" %SHELLYHEATERHOSTKIND_DNS%>Gen3</option>
          </select>
        </div>
      </div>

      <div class="form-group">
        <label for="shellyIPHumidity" data-i18n="shelly.shellyIPHumidity">Shelly IP Adresse für Luftbefeuchter:</label>
        <div class="twoinone-label">
          <input name="webShellyHumIP" id="shellyIPHumidity" style="width: 300px;" type="text" value="%SHELLYHUMIDIFIERIP%">
          <select name="webShellyHumidifierHostKind" style="width: 80px; id="shellyHumidifierHostKind">
            <option value="1" %SHELLYHUMIDIFIERHOSTKIND_IPV4%>Gen1</option>
            <option value="2" %SHELLYHUMIDIFIERHOSTKIND_IPV6%>Gen2</option>
            <option value="3" %SHELLYHUMIDIFIERHOSTKIND_DNS%>Gen3</option>
          </select>
        </div>
      </div>

      <h2 data-i18n="status.shellyAuth">Shelly Authentifizierung</h2>
      <div class="form-group">
        <label for="shellyUsername" data-i18n="shelly.shellyAuthUser">Shelly Benutzername:</label>
        <input name="webShellyUsername" id="shellyUsername" style="width: 200px;" type="text" value="%SHELLYUSERNAME%">
      </div>

      <div class="form-group">
        <label for="shellyPassword" data-i18n="shelly.shellyAuthPassword">Shelly Passwort:</label>
        <input name="webShellyPassword" id="shellyPassword" style="width: 200px;" type="password" value="%SHELLYPASSWORD%">
      </div>
      
      <div class="spacer"></div>
        <button class="primary" id="saveshellysettingsBtn" data-i18n="settings.save">Speichern</button>
    </section>
    </form>

    <!-- runsettings section -->
    <form action="/saverunsettings" method="POST">
    <section id="runsettings" class="page card">
      <h1 data-i18n="runsetting.title">Betriebseinstellungen</h1>

      <div class="form-group">
        <div class="tile-right-settings">
          <div class="form-group">
            <label for="webGrowStart" data-i18n="runsetting.startGrow">Start Grow Date:</label>
            <input id="webGrowStart" name="webGrowStart" type="date" style="width: 170px;" value="%GROWSTARTDATE%">
          </div>
          <div class="form-group">
            <label for="webFloweringStart" data-i18n="runsetting.startFlower">Start Flowering Date:</label>
            <input id="webFloweringStart" name="webFloweringStart" type="date" style="width: 170px;" value="%GROWFLOWERDATE%">
          </div>
          <div class="form-group">
            <label for="webDryingStart" data-i18n="runsetting.startDry">Start Drying Date:</label>
            <input id="webDryingStart" name="webDryingStart" type="date" style="width: 170px;" value="%GROWDRAYINGDATE%">
          </div>
        </div>

        <div class="form-group">
        <label for="phaseSelect" data-i18n="runsetting.phase">Phase:</label>
        <select id="phaseSelect" style="width: 170px;" name="webCurrentPhase">
          <option value="1" %PHASE1_SEL% data-i18n="runsetting.phase.grow">Wuchs</option>
          <option value="2" %PHASE2_SEL% data-i18n="runsetting.phase.flower">Blüte</option>
          <option value="3" %PHASE3_SEL% data-i18n="runsetting.phase.dry">Trocknung</option>
        </select>
        </div>

      <div class="form-group">
        <label for="targetTemp" data-i18n="runsetting.targetTemp">Soll-Temperatur:</label>
        <input name="webTargetTemp" id="webTargetTemp" style="width: 65px;" type="number" step="0.5" min="18" max="30" value="%TARGETTEMPERATURE%">&nbsp;°C
      </div>

      <div class="form-group">
        <label for="leafTemp" data-i18n="runsetting.offsetLeafTemperature">Offset Blatttemperatur:</label>
        <input name="webOffsetLeafTemp" id="webOffsetLeafTemp" style="width: 65px;" type="number" step="0.1" min="-3.0" max="0.0" value="%LEAFTEMPERATURE%">&nbsp;°C
      </div>

      <div class="form-group">
        <label for="targetVPD" data-i18n="runsetting.targetVPD">Soll-VPD:</label>
        <input name="webTargetVPD" id="webTargetVPD" style="width: 65px;" type="number" step="0.1" min="0.5" max="1.5" value="%TARGETVPD%">&nbsp;kPa
      </div>

      <h2 data-i18n="runsetting.wateringSettings">Bewässerungseinstellung</h2>

      <div class="form-group">
        <label for="timePerTask" data-i18n="runsetting.timePerTask">Bewässerungszeit pro Task:</label>
        <input name="webTimePerTask" id="webTimePerTask" style="width: 65px;" type="number" step="1" min="1" max="30" value="%TIMEPERTASK%">&nbsp;s&nbsp;(min 1s, max 30s, step 1s)
      </div>

      <div class="form-group">
        <label for="betweenTasks" data-i18n="runsetting.betweenTasks">Pause zwischen Bewässerungen:</label>
        <input name="webBetweenTasks" id="webBetweenTasks" style="width: 65px;" type="number" step="1" min="1" max="10" value="%BETWEENTASKS%">&nbsp;Min&nbsp;(min 1Min, max 10Min, step 1Min)
      </div>

      <div class="form-group">
        <label for="amountOfWater" data-i18n="runsetting.amountOfWater">Wassermenge nach 10 Sekunden:</label>
        <input name="webAmountOfWater" id="webAmountOfWater" style="width: 65px;" type="number" step="5" min="10" max="100" value="%AMOUNTOFWATER%">&nbsp;ml&nbsp;(min 10ml, max 100ml, step 5ml)
      </div>

      <div class="form-group">
        <label for="Irrigation" data-i18n="runsetting.irrigation">Gesamte Bewässerungsmenge:</label>
        <input name="webIrrigation" id="webIrrigation" style="width: 65px;" type="number" step="10" min="100" max="3000" value="%IRRIGATION%">&nbsp;ml&nbsp;(min 100ml, max 3000ml, step 10ml)
      </div>

      <div class="form-group">
        <label for="minTank">min. Tank:</label>
        <input name="webMinTank" id="webMinTank" style="width: 65px;" type="number" value="%MINTANK%">&nbsp;cm
      </div>
      
      <div class="form-group">
        <label for="maxTank">max. Tank:</label>
        <input name="webMaxTank" id="webMaxTank" style="width: 65px;" type="number" value="%MAXTANK%">&nbsp;cm
      </div>

      <h2 data-i18n="runsetting.relayScheduling">Relais Zeitsteuerung</h2>
      <div class="form-group">
        <label for="relay1">%RELAYNAMES1%</label>
        <div class="relay-scheduled-row">
          <label class="relay-scheduled-check" data-i18n="runsetting.relay.enable">Aktivieren:
            <input type="checkbox" name="r1_enabled">
          </label>
          <label class="relay-scheduled-label" data-i18n="runsetting.relay.start">Start:
            <input type="number" name="r1_start" min="0" max="55" step="5">
          </label>
          <label class="relay-scheduled-label" data-i18n="runsetting.relay.end">Ende:
            <input type="number" name="r1_end" min="0" max="55" step="5">
          </label>
        </div>
      </div>
      <div class="form-group">
        <label for="relay2">%RELAYNAMES2%</label>
        <div class="relay-scheduled-row">
          <label class="relay-scheduled-check" data-i18n="runsetting.relay.enable">Aktivieren:
            <input type="checkbox" name="r2_enabled">
          </label>
          <label class="relay-scheduled-label" data-i18n="runsetting.relay.start">Start:
            <input type="number" name="r2_start" min="0" max="55" step="5">
          </label>
          <label class="relay-scheduled-label" data-i18n="runsetting.relay.end">Ende:
            <input type="number" name="r2_end" min="0" max="55" step="5">
          </label>
        </div>
      </div>
      <div class="form-group">
        <label for="relay3">%RELAYNAMES3%</label>
        <div class="relay-scheduled-row">
          <label class="relay-scheduled-check" data-i18n="runsetting.relay.enable">Aktivieren:
            <input type="checkbox" name="r3_enabled">
          </label>
          <label class="relay-scheduled-label" data-i18n="runsetting.relay.start">Start:
            <input type="number" name="r3_start" min="0" max="55" step="5">
          </label>
          <label class="relay-scheduled-label" data-i18n="runsetting.relay.end">Ende:
            <input type="number" name="r3_end" min="0" max="55" step="5">
          </label>
        </div>
      </div>
      <div class="form-group">
        <label for="relay4">%RELAYNAMES4%</label>
        <div class="relay-scheduled-row">
          <label class="relay-scheduled-check" data-i18n="runsetting.relay.enable">Aktivieren:
            <input type="checkbox" name="r4_enabled">
          </label>
          <label class="relay-scheduled-label" data-i18n="runsetting.relay.start">Start:
            <input type="number" name="r4_start" min="0" max="59">
          </label>
          <label class="relay-scheduled-label" data-i18n="runsetting.relay.end">Ende:
            <input type="number" name="r4_end" min="0" max="59">
          </label>
        </div>
      </div>
      <div class="form-group">
        <label for="relay4">%RELAYNAMES5%</label>
        <div class="relay-scheduled-row">
          <label class="relay-scheduled-check" data-i18n="runsetting.relay.enable">Aktivieren:
            <input type="checkbox" name="r5_enabled">
          </label>
          <label class="relay-scheduled-label" data-i18n="runsetting.relay.start">Start:
            <input type="number" name="r5_start" min="0" max="59">
          </label>
          <label class="relay-scheduled-label" data-i18n="runsetting.relay.end">Ende:
            <input type="number" name="r5_end" min="0" max="59">
          </label>
        </div>
      </div>
      <div class="spacer"></div>
      <button class="primary" id="saverunsettingsBtn" data-i18n="settings.save">Speichern</button>
    </section>
    </form>

    <!-- setting section -->
    <form action="/savesettings" method="POST">
      <section id="settings" class="page card">
        <h1 data-i18n="settings.title">Systemeinstellungen</h1>

        <div class="form-group">
          <label for="webBoxName" data-i18n="settings.boxName">Boxname:</label>
          <input name="webBoxName" id="webBoxName" type="text" data-i18n="settings.boxName.ph" data-i18n-attr="placeholder" style="width: 320px;" value="%CONTROLLERNAME%">
        </div>

        <div class="form-group">
          <label for="webNTPServer" data-i18n="settings.ntpserver">NTP-Server:</label>
          <input name="webNTPServer" id="webNTPServer" type="text" data-i18n="settings.ntpserver.ph" data-i18n-attr="placeholder" style="width: 250px;" value="%NTPSERVER%">
        </div>

        <div class="form-group">
          <div class="label-inline">
            <label for="webTimeZoneInfo" data-i18n="settings.timeZoneInfo">Zeitzone:</label>
            &nbsp;<a href="https://github.com/nayarsystems/posix_tz_db/blob/master/zones.json" target="_blank" rel="noopener noreferrer">🌐</a>
          </div>
          <input name="webTimeZoneInfo" id="webTimeZoneInfo" type="text" data-i18n="settings.timeZoneInfo.ph" data-i18n-attr="placeholder" style="width: 350px;" value="%TZINFO%">
        </div>

        <div class="form-group">
          <label for="language" data-i18n="settings.language">Sprache:</label>
          <select name="webLanguage" id="language" style="width: 100px;">
            <!-- Optionen vermutlich per JS/i18n gefüllt -->
          </select>
        </div>

        <div class="form-group">
          <label for="theme" data-i18n="settings.theme">Theme:</label>
          <select name="webTheme" id="theme" style="width: 100px;">
            <option value="light" data-i18n="settings.themeLight">Hell</option>
            <option value="dark"  data-i18n="settings.themeDark">Dunkel</option>
          </select>
        </div>

        <div class="form-group">
          <label for="dateFormat" data-i18n="settings.dateFormat">Datumsformat:</label>
          <select name="webDateFormat" id="dateFormat" style="width: 140px;">
            <option value="YYYY-MM-DD" data-i18n="settings.df_ymd">YYYY-MM-DD</option>
            <option value="DD.MM.YYYY" data-i18n="settings.df_dmy">DD.MM.YYYY</option>
          </select>
        </div>

        <div class="form-group">
          <label for="timeFormat" data-i18n="settings.timeFormat">Zeitformat:</label>
          <select name="webTimeFormat" id="timeFormat" style="width: 100px;">
            <option value="24" data-i18n="settings.tf_HHmm">24h</option>
            <option value="12" data-i18n="settings.tf_hhmma">12h AM/PM</option>
          </select>
        </div>

        <div class="form-group">
          <label for="tempUnit" data-i18n="settings.tempUnit">Temperatur-Einheit:</label>
          <select name="webTempUnit" id="tempUnit" style="width: 140px;">
            <option value="C" data-i18n="settings.celsius">°C (Celsius)</option>
            <option value="F" data-i18n="settings.fahrenheit">°F (Fahrenheit)</option>
          </select>
        </div>

        <h2 data-i18n="settings.DS18B20">DS18B20 Sensor</h2>        
        <div class="form-group checkbox">
          <label class="inline-checkbox">
           <input type="checkbox" name="webDS18B20Enable" id="webDS18B20Enable" %DS18B20ENABLE%>
           <span data-i18n="settings.enabled">aktivieren</span>
          </label>
        </div>

        <div class="form-group">
          <input name="webDS18B20Name" id="webDS18B20Name" type="text" data-i18n="settings.DS18B20Name.ph" data-i18n-attr="placeholder" style="width: 250px;" maxlength="15" value="%DS18B20NAME%">
        </div>

        <h2 data-i18n="settings.relaySettings">Relais Einstellungen</h2>
        <div class="form-group">
          <label for="webRelay1" data-i18n="settings.relay1">Relay 1:</label>
          <input name="webRelayName1" id="webRelayName1" type="text" data-i18n="settings.relay1.ph" data-i18n-attr="placeholder" style="width: 120px;" maxlength="15" value="%RELAYNAMES1%">
        </div>

        <div class="form-group">
          <label for="webRelay2" data-i18n="settings.relay2">Relay 2:</label>
          <input name="webRelayName2" id="webRelayName2" type="text" data-i18n="settings.relay2.ph" data-i18n-attr="placeholder" style="width: 120px;" maxlength="15" value="%RELAYNAMES2%">
        </div>

        <div class="form-group">
          <label for="webRelay3" data-i18n="settings.relay3">Relay 3:</label>
          <input name="webRelayName3" id="webRelayName3" type="text" data-i18n="settings.relay3.ph" data-i18n-attr="placeholder" style="width: 120px;" maxlength="15" value="%RELAYNAMES3%">
        </div>

        <div class="form-group">
          <label for="webRelay4" data-i18n="settings.relay4">Relay 4:</label>
          <input name="webRelayName4" id="webRelayName4" type="text" data-i18n="settings.relay4.ph" data-i18n-attr="placeholder" style="width: 120px;" maxlength="15" value="%RELAYNAMES4%">
        </div>

        <div class="form-group">
          <label for="webRelay5" data-i18n="settings.relay5">Relay 5:</label>
          <input name="webRelayName5" id="webRelayName5" type="text" data-i18n="settings.relay5.ph" data-i18n-attr="placeholder" style="width: 120px;" maxlength="15" value="%RELAYNAMES5%">
        </div>

        <button class="primary" id="saveSettingsBtn" data-i18n="settings.save">Speichern</button>
      </section>
    </form>

    <!-- runsettings section -->
    <form action="/savemessagesettings" method="POST">
    <section id="message" class="page card">
      <h1 data-i18n="message.title">Nachrichteneinstellungen</h1>

      <h2 data-i18n="message.pushoverSettings">Pushover Einstellungen</h2>

      <div class="form-group checkbox">
        <label class="inline-checkbox">
         <input type="checkbox" name="webPushoverEnabled" id="webPushoverEnabled" %PUSHOVERENABLED%>
         <span data-i18n="message.enabled">aktivieren</span>
        </label>
      </div>

      <div class="form-group">
        <label for="webPushoverUserKey" data-i18n="message.pushoverUserKey">Pushover Benutzer:</label>
        <input name="webPushoverUserKey" id="webPushoverUserKey" type="text" data-i18n="message.pushoverUserKey.ph" data-i18n-attr="placeholder" style="width: 320px;" value="%PUSHOVERUSERKEY%">
      </div>

      <div class="form-group">
        <label for="webPushoverAppKey" data-i18n="message.pushoverAppKey">Pushover Token:</label>
        <input name="webPushoverAppKey" id="webPushoverAppKey" type="text" data-i18n="message.pushoverAppKey.ph" data-i18n-attr="placeholder" style="width: 320px;" value="%PUSHOVERAPPKEY%">
      </div>

      <div class="form-group">
        <label for="webPushoverDevice" data-i18n="message.pushoverDevice">Pushover Gerät:</label>
        <input name="webPushoverDevice" id="webPushoverDevice" type="text" data-i18n="message.pushoverDevice.ph" data-i18n-attr="placeholder" style="width: 320px;" value="%PUSHOVERDEVICE%">
      </div>

      <h2 data-i18n="message.gotifySettings">Gotify Einstellungen</h2>

      <div class="form-group checkbox">
        <label class="inline-checkbox">
         <input type="checkbox" name="webGotifyEnabled" id="webGotifyEnabled" %GOTIFYENABLED%>
         <span data-i18n="message.enabled">aktivieren</span>
        </label>
      </div>

      <div class="form-group">
        <label for="webGotifyURL" data-i18n="message.gotifyURL">Gotify URL:</label>
        <input name="webGotifyURL" id="webGotifyURL" type="text" data-i18n="message.gotifyUrl.ph" data-i18n-attr="placeholder" style="width: 320px;" value="%GOTIFYURL%">
      </div>

      <div class="form-group">
        <label for="webGotifyToken" data-i18n="message.gotifyToken">Gotify Token:</label>
        <input name="webGotifyToken" id="webGotifyToken" type="text" data-i18n="message.gotifyToken.ph" data-i18n-attr="placeholder" style="width: 320px;" value="%GOTIFYTOKEN%">
      </div>

      <button class="primary" id="saveMessageBtn" data-i18n="settings.save">Speichern</button>
    </section>
    </form>
    

    <!-- system log section -->
    <section id="logging" class="page card">
      <h1 data-i18n="logging.title">Systemprotokoll</h1>
      <div class="weblog-card">
        <div class="weblog-head">
          <strong>System-Log</strong>
          <div class="weblog-actions">
            <a class="btn" href="/download/log">CSV/TXT Download</a>
            <button class="btn" id="toggleScrollBtn" type="button">AutoScroll: ON</button>
            <button class="btn" id="clearLogBtn" type="button" title="Log löschen">Clear</button>
          </div>
        </div>
        <pre id="weblog" class="weblog" aria-live="polite" aria-label="Laufende Logausgabe">…</pre>
      </div>
    </section>

    <!-- factory reset section -->
    <section id="factory" class="page card">
      <form action="/factory-reset" method="post" id="factoryResetForm">
        <h1 data-i18n="factory.title">Werkseinstellungen</h1>
        <input type="hidden" name="confirm" value="1">
        <button class="primary" id="factoryResetBtn" type="submit" data-i18n="factory.reset">factory reset</button>
      </form>
    </section>
  </main>
  </div>
  
 <script src="/script.js"></script>
</body>
</html>
)rawliteral";

const char* apPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>%CONTENTCONTROLLERNAME%</title>
  <meta charset="UTF-8">
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <header class="header">
    <button class="hamburger" id="hamburgerBtn" data-i18n="a11y.menu" data-i18n-attr="aria-label" aria-label="Menü öffnen/schließen" aria-expanded="false" aria-controls="sidebar">☰</button>
    <div class="title" data-i18n="app.title">%CONTENTCONTROLLERNAME%</div>
    </div>
  </header>
  <div class="layout">
    <nav class="sidebar" id="sidebar">
      <a class="navlink" data-page="settings"   data-i18n="nav.wifisettings">WIFI Setting</a>
    </nav>

  <div class="overlay" id="overlay"></div>

    <main class="content" id="content">
      <section id="status" class="page active card">
        <form action="/save" method="post">
          <h1 data-i18n="settings.title">WIFI Setting</h1>
          <label for="ssid">WIFI SSID:</label>
          <input type="text" id="ssid" name="ssid" required><br><br>
          <label for="password">WIFI Passwort:</label>
          <input type="password" id="password" name="password" required><br><br>
          <button class="primary" id="saveBtn" data-i18n="settings.save">save & reboot</button>
        </form>
      </section>
      <section id="status" class="page active card">
        <form action="/factory-reset" method="post" id="factoryResetForm">
          <h1 data-i18n="settings.title">Factory Reset</h1>
          <input type="hidden" name="confirm" value="1">
          <button class="primary" id="factoryResetBtn" type="submit" data-i18n="settings.factoryreset.button">factory reset</button>
        </form>
      </section>
    </main>
  </div>
  
 <script src="/script.js"></script>
</body>
</html>
)rawliteral";