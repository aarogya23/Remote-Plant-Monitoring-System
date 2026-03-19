#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>

// ================= ESP32 HOTSPOT (ACCESS POINT MODE) =================
const char* ap_ssid     = "PlantOS";        // Hotspot name shown on your phone
const char* ap_password = "plant1234";      // Min 8 chars, leave "" for open network
IPAddress ap_ip(192, 168, 4, 1);            // Fixed IP — open http://192.168.4.1

// ================= PIN SETUP =================
#define PH_PIN        34
#define SOIL_PIN      35
#define DHT_PIN       4
#define DHT_TYPE      DHT11
#define BUZZER_PIN    19
#define LED_PIN       2

// ================= OBJECTS =================
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

// ================= SETTINGS =================
const unsigned long READ_INTERVAL = 2000;
const unsigned long LCD_PAGE_TIME = 3000;
const byte SAMPLE_COUNT = 30;

float ph_offset = 0.0;
float ph_slope  = 3.5;

int SOIL_DRY = 4095;
int SOIL_WET = 3000;

unsigned long previousMillis = 0;
unsigned long lcdMillis      = 0;
byte lcdPage = 0;

float  g_pH = 0, g_temp = 0, g_humidity = 0;
int    g_soilPercent = 0, g_soilRaw = 0;
String g_soilStatus = "", g_phStatus = "", g_tempStatus = "";

// ================= AVERAGE READ =================
int readAverage(byte pin) {
  long total = 0;
  for (byte i = 0; i < SAMPLE_COUNT; i++) {
    total += analogRead(pin);
    delay(5);
  }
  return total / SAMPLE_COUNT;
}

// ================= BUZZER =================
void beep(int freq, int duration) {
  tone(BUZZER_PIN, freq);
  delay(duration);
  noTone(BUZZER_PIN);
}

void dryAlert() {
  for (int i = 0; i < 3; i++) {
    beep(2000, 200);
    delay(100);
  }
}

// ================= LCD DISPLAY =================
void showLCDPage(byte page) {
  lcd.clear();
  if (page == 0) {
    lcd.setCursor(0,0);
    lcd.print("Soil:");
    lcd.print(g_soilPercent);
    lcd.print("% ");
    lcd.print(g_soilStatus.substring(0,8));
    lcd.setCursor(0,1);
    lcd.print("pH:");
    lcd.print(g_pH,2);
    lcd.print(" ");
    lcd.print(g_phStatus.substring(0,8));
  } else if (page == 1) {
    lcd.setCursor(0,0);
    lcd.print("Temp:");
    lcd.print(g_temp,1);
    lcd.print("C ");
    lcd.print(g_tempStatus.substring(0,6));
    lcd.setCursor(0,1);
    lcd.print("Humid:");
    lcd.print(g_humidity,1);
    lcd.print("%");
  } else if (page == 2) {
    lcd.setCursor(0,0);
    lcd.print("IP:");
    lcd.print(WiFi.localIP());
    lcd.setCursor(0,1);
    if(g_soilPercent < 30){
      lcd.print("!! WATER PLANT !!");
    } else {
      lcd.print("Plant is OK :)  ");
    }
  }
}

// ================= HEALTH SCORE =================
int calcHealth() {
  int score = 100;
  if (g_soilPercent < 20) score -= 30;
  else if (g_soilPercent < 40) score -= 15;
  else if (g_soilPercent > 85) score -= 10;
  if (g_pH < 5.0 || g_pH > 8.0) score -= 25;
  else if (g_pH < 5.5 || g_pH > 7.5) score -= 10;
  if (g_temp < 10 || g_temp > 40) score -= 25;
  else if (g_temp < 15 || g_temp > 35) score -= 10;
  if (g_humidity < 20 || g_humidity > 90) score -= 20;
  else if (g_humidity < 30 || g_humidity > 80) score -= 8;
  if (score < 0) score = 0;
  return score;
}

// ================= JSON DATA ENDPOINT =================
// This is called every 2s by the browser — no page reload needed
void handleData() {
  int health = calcHealth();
  String json = "{";
  json += "\"temp\":"        + String(g_temp, 1)      + ",";
  json += "\"humidity\":"    + String(g_humidity, 1)  + ",";
  json += "\"soil\":"        + String(g_soilPercent)  + ",";
  json += "\"ph\":"          + String(g_pH, 2)        + ",";
  json += "\"health\":"      + String(health)         + ",";
  json += "\"soilStatus\":\"" + g_soilStatus           + "\",";
  json += "\"phStatus\":\""   + g_phStatus             + "\",";
  json += "\"tempStatus\":\"" + g_tempStatus           + "\"";
  json += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// ================= WEBPAGE =================
void handleRoot() {
  String ipStr = ap_ip.toString();   // Always 192.168.4.1 in AP mode

  String html = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<!-- NO meta refresh - we use AJAX instead -->
<title>PlantOS &mdash; Dashboard</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=DM+Sans:wght@400;500;600;700&family=DM+Mono:wght@400;500&display=swap" rel="stylesheet">

<style>
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

:root {
  --bg:        #f4f6f9;
  --sidebar:   #ffffff;
  --card:      #ffffff;
  --border:    #e8ecf0;
  --text:      #1a2332;
  --muted:     #7a8694;
  --accent:    #22c55e;
  --font:      'DM Sans', sans-serif;
  --mono:      'DM Mono', monospace;
}

body {
  font-family: var(--font);
  background: var(--bg);
  color: var(--text);
  display: flex;
  min-height: 100vh;
}

/* ---- SIDEBAR ---- */
.sidebar {
  width: 220px;
  background: var(--sidebar);
  border-right: 1px solid var(--border);
  display: flex;
  flex-direction: column;
  padding: 0;
  position: fixed;
  top: 0; left: 0;
  height: 100vh;
  z-index: 10;
}

.logo {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 22px 20px 18px;
  border-bottom: 1px solid var(--border);
}

.logo-icon {
  width: 36px; height: 36px;
  background: #dcfce7;
  border-radius: 10px;
  display: flex; align-items: center; justify-content: center;
  font-size: 18px;
}

.logo-name {
  font-weight: 700;
  font-size: 17px;
  color: var(--text);
  letter-spacing: -0.3px;
}

.nav-section {
  padding: 20px 12px 8px;
}

.nav-label {
  font-size: 10px;
  font-weight: 700;
  color: var(--muted);
  letter-spacing: 0.08em;
  text-transform: uppercase;
  padding: 0 8px;
  margin-bottom: 6px;
}

.nav-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 9px 12px;
  border-radius: 9px;
  font-size: 13.5px;
  font-weight: 500;
  color: var(--muted);
  cursor: pointer;
  transition: all 0.15s;
  text-decoration: none;
}

.nav-item:hover { background: #f1f5f9; color: var(--text); }
.nav-item.active { background: #f0fdf4; color: #16a34a; font-weight: 600; }
.nav-item .nav-icon { font-size: 15px; width: 20px; text-align: center; }

.sidebar-footer {
  margin-top: auto;
  padding: 16px 20px;
  border-top: 1px solid var(--border);
}

.device-chip {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 12px;
  background: #f8fafc;
  border-radius: 10px;
  border: 1px solid var(--border);
}

.device-dot {
  width: 8px; height: 8px;
  background: #22c55e;
  border-radius: 50%;
  box-shadow: 0 0 0 3px #dcfce7;
  flex-shrink: 0;
}

.device-info { flex: 1; }
.device-name { font-size: 12px; font-weight: 600; color: var(--text); }
.device-status { font-size: 11px; color: #22c55e; font-weight: 500; }

/* ---- MAIN ---- */
.main {
  margin-left: 220px;
  flex: 1;
  display: flex;
  flex-direction: column;
  min-height: 100vh;
}

/* ---- TOPBAR ---- */
.topbar {
  background: var(--card);
  border-bottom: 1px solid var(--border);
  padding: 16px 28px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  position: sticky; top: 0; z-index: 5;
}

.topbar-title h2 {
  font-size: 20px;
  font-weight: 700;
  letter-spacing: -0.4px;
}

.topbar-title p {
  font-size: 12px;
  color: var(--muted);
  margin-top: 1px;
  font-family: var(--mono);
}

.topbar-right {
  display: flex;
  align-items: center;
  gap: 12px;
}

.live-badge {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 5px 12px;
  background: #f0fdf4;
  border: 1px solid #bbf7d0;
  border-radius: 20px;
  font-size: 12px;
  font-weight: 600;
  color: #16a34a;
}

.live-dot {
  width: 7px; height: 7px;
  background: #22c55e;
  border-radius: 50%;
  animation: pulse 1.5s infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 1; transform: scale(1); }
  50% { opacity: 0.5; transform: scale(0.85); }
}

.time-display {
  font-family: var(--mono);
  font-size: 13px;
  font-weight: 500;
  color: var(--muted);
  background: #f8fafc;
  padding: 5px 12px;
  border-radius: 8px;
  border: 1px solid var(--border);
}

/* ---- CONTENT ---- */
.content {
  padding: 24px 28px;
  display: flex;
  flex-direction: column;
  gap: 20px;
}

/* ---- ALERT BANNER ---- */
.alert-banner {
  padding: 12px 18px;
  border-radius: 12px;
  border: 1.5px solid #22c55e;
  background: #f0fdf4;
  color: #16a34a;
  font-size: 14px;
  font-weight: 600;
  display: flex;
  align-items: center;
  gap: 10px;
  transition: all 0.4s ease;
}

/* ---- SENSOR CARDS ---- */
.cards-row {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16px;
}

.card {
  background: var(--card);
  border-radius: 14px;
  padding: 18px 20px 16px;
  border: 1px solid var(--border);
  box-shadow: 0 1px 4px rgba(0,0,0,0.04);
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 12px;
}

.card-icon {
  width: 40px; height: 40px;
  border-radius: 11px;
  background: #f8fafc;
  border: 1px solid var(--border);
  display: flex; align-items: center; justify-content: center;
  font-size: 18px;
}

.card-delta {
  font-size: 11.5px;
  font-weight: 700;
  font-family: var(--mono);
  transition: color 0.3s ease;
}

.card-label {
  font-size: 12px;
  color: var(--muted);
  font-weight: 500;
  margin-bottom: 2px;
}

.card-value {
  font-size: 34px;
  font-weight: 800;
  font-family: var(--mono);
  letter-spacing: -1px;
  line-height: 1;
  transition: color 0.3s ease;
}

.card-unit {
  font-size: 15px;
  font-weight: 500;
  color: var(--muted);
  margin-left: 2px;
}

.card-sub {
  margin-top: 10px;
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 11.5px;
  font-weight: 600;
}

.card-bar-track {
  height: 5px;
  background: #f1f5f9;
  border-radius: 10px;
  margin-top: 10px;
  overflow: hidden;
}

.card-bar-fill {
  height: 100%;
  border-radius: 10px;
  transition: width 0.6s ease, background 0.3s ease;
}

.status-dot {
  width: 7px; height: 7px;
  border-radius: 50%;
  flex-shrink: 0;
  transition: background 0.3s ease;
}

/* ---- BOTTOM ROW ---- */
.bottom-row {
  display: grid;
  grid-template-columns: 1fr 320px;
  gap: 16px;
}

/* ---- CHART CARD ---- */
.chart-card {
  background: var(--card);
  border-radius: 14px;
  padding: 20px 24px;
  border: 1px solid var(--border);
  box-shadow: 0 1px 4px rgba(0,0,0,0.04);
}

.chart-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 4px;
}

.chart-title { font-size: 15px; font-weight: 700; }
.chart-sub { font-size: 12px; color: var(--muted); margin-top: 2px; }

.chart-area {
  width: 100%;
  height: 220px;
  margin-top: 16px;
  position: relative;
  display: flex;
  align-items: flex-end;
}

.sparkline { width: 100%; height: 100%; }

.legend {
  display: flex;
  gap: 16px;
  margin-top: 12px;
  flex-wrap: wrap;
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  color: var(--muted);
  font-weight: 500;
}

.legend-dot {
  width: 10px; height: 10px;
  border-radius: 50%;
}

/* ---- HEALTH CARD ---- */
.health-card {
  background: var(--card);
  border-radius: 14px;
  padding: 20px 24px;
  border: 1px solid var(--border);
  box-shadow: 0 1px 4px rgba(0,0,0,0.04);
  display: flex;
  flex-direction: column;
}

.health-title { font-size: 15px; font-weight: 700; }
.health-sub { font-size: 12px; color: var(--muted); margin-top: 2px; }

.health-ring-wrap {
  display: flex;
  justify-content: center;
  align-items: center;
  margin: 18px 0 16px;
}

.health-metrics {
  display: flex;
  flex-direction: column;
  gap: 10px;
  margin-top: 4px;
}

.metric-row {
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 12.5px;
}

.metric-icon { font-size: 14px; width: 20px; text-align: center; }
.metric-name { flex: 1; font-weight: 500; color: var(--muted); }
.metric-val {
  font-family: var(--mono);
  font-size: 12.5px;
  font-weight: 600;
  color: var(--text);
  transition: color 0.3s;
}

.metric-bar {
  width: 80px;
  height: 4px;
  background: #f1f5f9;
  border-radius: 10px;
  overflow: hidden;
}

.metric-bar-fill {
  height: 100%;
  border-radius: 10px;
  transition: width 0.6s ease, background 0.3s ease;
}

/* ---- IP FOOTER ---- */
.ip-footer {
  text-align: center;
  font-size: 11px;
  color: var(--muted);
  font-family: var(--mono);
  padding-bottom: 8px;
}

/* ---- WIFI INFO BANNER ---- */
.wifi-info {
  background: #eff6ff;
  border: 1px solid #bfdbfe;
  border-radius: 10px;
  padding: 10px 16px;
  font-size: 12px;
  color: #1d4ed8;
  font-family: var(--mono);
  display: flex;
  align-items: center;
  gap: 10px;
}

/* ---- FLASH ANIMATION for value update ---- */
@keyframes flashUpdate {
  0%   { opacity: 1; }
  30%  { opacity: 0.4; }
  100% { opacity: 1; }
}
.updated {
  animation: flashUpdate 0.4s ease;
}

/* ---- RESPONSIVE ---- */
@media (max-width: 900px) {
  .sidebar { display: none; }
  .main { margin-left: 0; }
  .cards-row { grid-template-columns: repeat(2, 1fr); }
  .bottom-row { grid-template-columns: 1fr; }
}
</style>
</head>
<body>

<!-- SIDEBAR -->
<aside class="sidebar">
  <div class="logo">
    <div class="logo-icon">&#127807;</div>
    <span class="logo-name">PlantOS</span>
  </div>

  <div class="nav-section">
    <div class="nav-label">Main</div>
    <a class="nav-item active" href="#">
      <span class="nav-icon">&#9783;</span> Dashboard
    </a>
    <a class="nav-item" href="#">
      <span class="nav-icon">&#128200;</span> Reports
    </a>
    <a class="nav-item" href="#">
      <span class="nav-icon">&#128276;</span> Alerts
    </a>
  </div>

  <div class="nav-section">
    <div class="nav-label">Settings</div>
    <a class="nav-item" href="#">
      <span class="nav-icon">&#9881;</span> Config
    </a>
    <a class="nav-item" href="#">
      <span class="nav-icon">&#128268;</span> Device
    </a>
  </div>

  <div class="sidebar-footer">
    <div class="device-chip">
      <div class="device-dot"></div>
      <div class="device-info">
        <div class="device-name">ESP32-AGRO</div>
        <div class="device-status" id="connStatus">&#8226; Online</div>
      </div>
    </div>
  </div>
</aside>

<!-- MAIN -->
<div class="main">

  <!-- TOPBAR -->
  <div class="topbar">
    <div class="topbar-title">
      <h2>Plant Dashboard</h2>
      <p id="lastRead">Last reading: --:--:--</p>
    </div>
    <div class="topbar-right">
      <div class="time-display" id="clockDisp">--:--:--</div>
      <div class="live-badge">
        <div class="live-dot"></div> Live
      </div>
    </div>
  </div>

  <!-- CONTENT -->
  <div class="content">

    <!-- WIFI INFO BANNER -->
    <div class="wifi-info">
      &#128225; Connect to WiFi hotspot: <strong>PlantOS</strong> &nbsp;|&nbsp; Password: <strong>plant1234</strong> &nbsp;|&nbsp; Then open <strong>http://192.168.4.1</strong>
    </div>

    <!-- ALERT BANNER -->
    <div class="alert-banner" id="alertBanner">
      <span style="font-size:18px" id="alertIcon">&#127807;</span>
      <span id="alertMsg">Plant is healthy &amp; happy</span>
    </div>

    <!-- SENSOR CARDS -->
    <div class="cards-row">

      <!-- TEMPERATURE -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#127777;</div>
          <span class="card-delta" id="tempStatus">Normal</span>
        </div>
        <div class="card-label">Temperature</div>
        <div>
          <span class="card-value" id="tempVal">--</span>
          <span class="card-unit">&deg;C</span>
        </div>
        <div class="card-bar-track">
          <div class="card-bar-fill" id="tempBar" style="width:0%;background:#22c55e"></div>
        </div>
        <div class="card-sub">
          <div class="status-dot" id="tempDot" style="background:#22c55e"></div>
          <span id="tempSubStatus" style="color:#22c55e">Normal</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;Optimal 18&ndash;25&deg;C</span>
        </div>
      </div>

      <!-- HUMIDITY -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#128167;</div>
          <span class="card-delta" id="humDelta" style="color:#3b82f6">--%</span>
        </div>
        <div class="card-label">Humidity</div>
        <div>
          <span class="card-value" id="humVal" style="color:#3b82f6">--</span>
          <span class="card-unit">%</span>
        </div>
        <div class="card-bar-track">
          <div class="card-bar-fill" id="humBar" style="width:0%;background:#3b82f6"></div>
        </div>
        <div class="card-sub">
          <div class="status-dot" id="humDot" style="background:#3b82f6"></div>
          <span style="color:var(--muted);font-weight:400">Optimal 40&ndash;70%</span>
        </div>
      </div>

      <!-- SOIL MOISTURE -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#127807;</div>
          <span class="card-delta" id="soilStatus">Moist</span>
        </div>
        <div class="card-label">Soil Moisture</div>
        <div>
          <span class="card-value" id="soilVal">--</span>
          <span class="card-unit">%</span>
        </div>
        <div class="card-bar-track">
          <div class="card-bar-fill" id="soilBar" style="width:0%;background:#22c55e"></div>
        </div>
        <div class="card-sub">
          <div class="status-dot" id="soilDot" style="background:#22c55e"></div>
          <span id="soilSubStatus" style="color:#22c55e">Moist</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;Optimal 35&ndash;65%</span>
        </div>
      </div>

      <!-- PH -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#9878;</div>
          <span class="card-delta" id="phStatus">Neutral</span>
        </div>
        <div class="card-label">Soil pH</div>
        <div>
          <span class="card-value" id="phVal">--</span>
          <span class="card-unit">pH</span>
        </div>
        <div class="card-bar-track">
          <div class="card-bar-fill" id="phBar" style="width:0%;background:#22c55e"></div>
        </div>
        <div class="card-sub">
          <div class="status-dot" id="phDot" style="background:#22c55e"></div>
          <span id="phSubStatus" style="color:#22c55e">Neutral</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;Optimal 5.5&ndash;7.5</span>
        </div>
      </div>

    </div><!-- end cards-row -->

    <!-- BOTTOM ROW -->
    <div class="bottom-row">

      <!-- CHART -->
      <div class="chart-card">
        <div class="chart-header">
          <div>
            <div class="chart-title">Sensor Readings</div>
            <div class="chart-sub">Live rolling history</div>
          </div>
        </div>

        <div class="chart-area">
          <svg class="sparkline" id="sparkSVG" viewBox="0 0 600 200" preserveAspectRatio="none">
            <line x1="0" y1="33"  x2="600" y2="33"  stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="66"  x2="600" y2="66"  stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="100" x2="600" y2="100" stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="133" x2="600" y2="133" stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="166" x2="600" y2="166" stroke="#f1f5f9" stroke-width="1"/>
            <polyline id="tempLine" fill="none" stroke="#ef4444" stroke-width="2" stroke-linejoin="round"/>
            <polyline id="humLine"  fill="none" stroke="#3b82f6" stroke-width="2" stroke-linejoin="round"/>
            <polyline id="soilLine" fill="none" stroke="#22c55e" stroke-width="2" stroke-linejoin="round"/>
          </svg>
        </div>

        <div class="legend">
          <div class="legend-item">
            <div class="legend-dot" style="background:#ef4444"></div> Temperature (&deg;C)
          </div>
          <div class="legend-item">
            <div class="legend-dot" style="background:#3b82f6"></div> Humidity (%)
          </div>
          <div class="legend-item">
            <div class="legend-dot" style="background:#22c55e"></div> Soil (%)
          </div>
        </div>
      </div>

      <!-- HEALTH CARD -->
      <div class="health-card">
        <div class="health-title">Plant Health</div>
        <div class="health-sub">Composite score from all sensors</div>

        <div class="health-ring-wrap">
          <svg width="150" height="150" viewBox="0 0 150 150">
            <circle cx="75" cy="75" r="54"
              fill="none" stroke="#f1f5f9" stroke-width="11"/>
            <circle cx="75" cy="75" r="54" id="healthRing"
              fill="none"
              stroke="#22c55e"
              stroke-width="11"
              stroke-linecap="round"
              stroke-dasharray="339.3"
              stroke-dashoffset="339.3"
              transform="rotate(-90 75 75)"
              style="transition: stroke-dashoffset 0.8s ease, stroke 0.4s ease;"/>
            <text x="75" y="69" text-anchor="middle"
              font-family="'DM Mono',monospace"
              font-size="26" font-weight="700"
              id="healthPct" fill="#22c55e">--%</text>
            <text x="75" y="88" text-anchor="middle"
              font-family="'DM Sans',sans-serif"
              font-size="11" font-weight="500"
              fill="#94a3b8">HEALTH</text>
          </svg>
        </div>

        <div class="health-metrics">
          <div class="metric-row">
            <span class="metric-icon">&#127777;</span>
            <span class="metric-name">Temp</span>
            <div class="metric-bar">
              <div class="metric-bar-fill" id="mTempBar" style="width:0%;background:#22c55e"></div>
            </div>
            <span class="metric-val" id="mTempVal">--&deg;C</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#128167;</span>
            <span class="metric-name">Humidity</span>
            <div class="metric-bar">
              <div class="metric-bar-fill" id="mHumBar" style="width:0%;background:#3b82f6"></div>
            </div>
            <span class="metric-val" id="mHumVal">--%</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#127807;</span>
            <span class="metric-name">Soil</span>
            <div class="metric-bar">
              <div class="metric-bar-fill" id="mSoilBar" style="width:0%;background:#22c55e"></div>
            </div>
            <span class="metric-val" id="mSoilVal">--%</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#9878;</span>
            <span class="metric-name">pH</span>
            <div class="metric-bar">
              <div class="metric-bar-fill" id="mPhBar" style="width:0%;background:#22c55e"></div>
            </div>
            <span class="metric-val" id="mPhVal">--</span>
          </div>
        </div>

      </div><!-- end health-card -->
    </div><!-- end bottom-row -->

    <div class="ip-footer">
      ESP32-AGRO &nbsp;|&nbsp; College WiFi &nbsp;|&nbsp; http://)rawliteral" + ipStr + R"rawliteral(
    </div>

  </div><!-- end content -->
</div><!-- end main -->

<script>
// ============================================================
//  UTILITY
// ============================================================
function colorFor(type, val) {
  if (type === 'temp')
    return (val < 15 || val > 35) ? '#ef4444' : '#22c55e';
  if (type === 'hum')
    return (val < 30 || val > 80) ? '#f59e0b' : '#3b82f6';
  if (type === 'soil')
    return (val < 30) ? '#ef4444' : (val < 40) ? '#f59e0b' : '#22c55e';
  if (type === 'ph')
    return (val < 5.5 || val > 7.5) ? '#f59e0b' : '#22c55e';
  if (type === 'health')
    return (val >= 75) ? '#22c55e' : (val >= 50) ? '#f59e0b' : '#ef4444';
  return '#22c55e';
}

function setColor(el, color) {
  el.style.color = color;
}

function flash(el) {
  el.classList.remove('updated');
  void el.offsetWidth; // reflow trick
  el.classList.add('updated');
}

// ============================================================
//  CLOCK
// ============================================================
function updateClock() {
  var now = new Date();
  var h = String(now.getHours()).padStart(2,'0');
  var m = String(now.getMinutes()).padStart(2,'0');
  var s = String(now.getSeconds()).padStart(2,'0');
  document.getElementById('clockDisp').textContent = h+':'+m+':'+s;
}
updateClock();
setInterval(updateClock, 1000);

// ============================================================
//  SPARKLINE CHART
// ============================================================
var MAX_POINTS = 40;
var tempData = [], humData  = [], soilData = [];
var curTemp  = 0,  curHum  = 0,  curSoil  = 0;

// Pre-fill with zeros; will update once data arrives
for (var i = 0; i < MAX_POINTS; i++) {
  tempData.push(0); humData.push(0); soilData.push(0);
}

function toSVGPoints(data, minVal, maxVal) {
  var pts = '';
  for (var i = 0; i < data.length; i++) {
    var x = (i / (MAX_POINTS - 1)) * 600;
    var y = 200 - ((data[i] - minVal) / (maxVal - minVal)) * 200;
    y = Math.max(2, Math.min(198, y));
    pts += x.toFixed(1) + ',' + y.toFixed(1) + ' ';
  }
  return pts.trim();
}

function pushChart(t, h, s) {
  tempData.push(t); humData.push(h); soilData.push(s);
  if (tempData.length > MAX_POINTS) { tempData.shift(); humData.shift(); soilData.shift(); }
  document.getElementById('tempLine').setAttribute('points', toSVGPoints(tempData, 0, 60));
  document.getElementById('humLine').setAttribute('points',  toSVGPoints(humData,  0, 100));
  document.getElementById('soilLine').setAttribute('points', toSVGPoints(soilData, 0, 100));
}

// ============================================================
//  UPDATE UI FROM DATA
// ============================================================
function applyData(d) {
  var tc = colorFor('temp',  d.temp);
  var hc = colorFor('hum',   d.humidity);
  var sc = colorFor('soil',  d.soil);
  var pc = colorFor('ph',    d.ph);
  var hc2= colorFor('health',d.health);
  var circ   = 339.3;
  var ringOff = circ - (d.health / 100.0) * circ;

  // --- Temperature ---
  var tv = document.getElementById('tempVal');
  tv.textContent = d.temp;
  setColor(tv, tc);
  setColor(document.getElementById('tempStatus'), tc);
  document.getElementById('tempStatus').textContent  = d.tempStatus;
  document.getElementById('tempSubStatus').textContent = d.tempStatus;
  setColor(document.getElementById('tempSubStatus'), tc);
  document.getElementById('tempBar').style.width      = Math.min((d.temp / 50) * 100, 100) + '%';
  document.getElementById('tempBar').style.background = tc;
  document.getElementById('tempDot').style.background = tc;
  flash(tv);

  // --- Humidity ---
  var hv = document.getElementById('humVal');
  hv.textContent = d.humidity;
  setColor(hv, hc);
  document.getElementById('humDelta').textContent    = d.humidity + '%';
  setColor(document.getElementById('humDelta'), hc);
  document.getElementById('humBar').style.width      = d.humidity + '%';
  document.getElementById('humBar').style.background = hc;
  document.getElementById('humDot').style.background = hc;
  flash(hv);

  // --- Soil ---
  var sv = document.getElementById('soilVal');
  sv.textContent = d.soil;
  setColor(sv, sc);
  document.getElementById('soilStatus').textContent     = d.soilStatus;
  setColor(document.getElementById('soilStatus'), sc);
  document.getElementById('soilSubStatus').textContent  = d.soilStatus;
  setColor(document.getElementById('soilSubStatus'), sc);
  document.getElementById('soilBar').style.width        = d.soil + '%';
  document.getElementById('soilBar').style.background   = sc;
  document.getElementById('soilDot').style.background   = sc;
  flash(sv);

  // --- pH ---
  var pv = document.getElementById('phVal');
  pv.textContent = parseFloat(d.ph).toFixed(2);
  setColor(pv, pc);
  document.getElementById('phStatus').textContent    = d.phStatus;
  setColor(document.getElementById('phStatus'), pc);
  document.getElementById('phSubStatus').textContent = d.phStatus;
  setColor(document.getElementById('phSubStatus'), pc);
  document.getElementById('phBar').style.width       = ((d.ph / 14.0) * 100) + '%';
  document.getElementById('phBar').style.background  = pc;
  document.getElementById('phDot').style.background  = pc;
  flash(pv);

  // --- Health ring ---
  var ring = document.getElementById('healthRing');
  ring.setAttribute('stroke-dashoffset', ringOff.toFixed(1));
  ring.setAttribute('stroke', hc2);
  var hp = document.getElementById('healthPct');
  hp.textContent = d.health + '%';
  hp.setAttribute('fill', hc2);

  // --- Health mini-bars ---
  document.getElementById('mTempBar').style.width      = Math.min((d.temp / 50) * 100, 100) + '%';
  document.getElementById('mTempBar').style.background = tc;
  document.getElementById('mTempVal').textContent      = d.temp + '\u00B0C';

  document.getElementById('mHumBar').style.width       = d.humidity + '%';
  document.getElementById('mHumBar').style.background  = hc;
  document.getElementById('mHumVal').textContent       = d.humidity + '%';

  document.getElementById('mSoilBar').style.width      = d.soil + '%';
  document.getElementById('mSoilBar').style.background = sc;
  document.getElementById('mSoilVal').textContent      = d.soil + '%';

  document.getElementById('mPhBar').style.width        = ((d.ph / 14.0) * 100) + '%';
  document.getElementById('mPhBar').style.background   = pc;
  document.getElementById('mPhVal').textContent        = parseFloat(d.ph).toFixed(2);

  // --- Alert banner ---
  var banner = document.getElementById('alertBanner');
  var alertIcon = document.getElementById('alertIcon');
  var alertMsg  = document.getElementById('alertMsg');
  if (d.soil < 30) {
    banner.style.borderColor = '#ef4444';
    banner.style.background  = '#fef2f2';
    banner.style.color       = '#dc2626';
    alertIcon.textContent    = '\u26A0\uFE0F';
    alertMsg.textContent     = 'WATER YOUR PLANT NOW!';
  } else {
    banner.style.borderColor = '#22c55e';
    banner.style.background  = '#f0fdf4';
    banner.style.color       = '#16a34a';
    alertIcon.textContent    = '\u2705';
    alertMsg.innerHTML       = 'Plant is healthy &amp; happy';
  }

  // --- Chart ---
  pushChart(d.temp, d.humidity, d.soil);

  // --- Last read time ---
  var now = new Date();
  var h = String(now.getHours()).padStart(2,'0');
  var m = String(now.getMinutes()).padStart(2,'0');
  var s = String(now.getSeconds()).padStart(2,'0');
  document.getElementById('lastRead').textContent = 'Last reading: ' + h + ':' + m + ':' + s;
}

// ============================================================
//  AJAX FETCH — replaces meta refresh completely
// ============================================================
var fetchFails = 0;

function fetchData() {
  fetch('/data')
    .then(function(r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return r.json();
    })
    .then(function(d) {
      fetchFails = 0;
      document.getElementById('connStatus').textContent  = '\u2022 Online';
      document.getElementById('connStatus').style.color  = '#22c55e';
      applyData(d);
    })
    .catch(function(err) {
      fetchFails++;
      console.warn('Fetch failed #' + fetchFails + ':', err);
      if (fetchFails >= 3) {
        document.getElementById('connStatus').textContent = '\u2022 Reconnecting...';
        document.getElementById('connStatus').style.color = '#f59e0b';
      }
    });
}

// Fetch immediately on page load, then every 2 seconds
fetchData();
setInterval(fetchData, 2000);
</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ================= AP MODE — no reconnect needed =================
// The ESP32 hotspot stays up as long as the board is powered.
void ensureWiFiConnected() {
  // Nothing to do in AP mode — hotspot is always on
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);
  Wire.begin();
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(2,0); lcd.print("AGRO SYSTEM");
  lcd.setCursor(3,1); lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  // ---- Start ESP32 as a WiFi Hotspot (Access Point) ----
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ap_ip, ap_ip, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("Hotspot started!");
  Serial.print("SSID     : "); Serial.println(ap_ssid);
  Serial.print("Password : "); Serial.println(ap_password);
  Serial.print("Dashboard: http://"); Serial.println(ap_ip);

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Hotspot ON!");
  lcd.setCursor(0,1); lcd.print(ap_ip);
  delay(3000);

  lcd.clear();

  // Register routes
  server.on("/",     handleRoot);
  server.on("/data", handleData);   // <-- NEW: JSON endpoint for AJAX
  server.begin();
  Serial.println("Web server started.");

  beep(1000, 100); delay(100); beep(1500, 100);
}

// ================= LOOP =================
void loop(){
  ensureWiFiConnected();

  server.handleClient();

  if (millis() - lcdMillis >= LCD_PAGE_TIME) {
    lcdMillis = millis();
    lcdPage   = (lcdPage + 1) % 3;
    showLCDPage(lcdPage);
  }

  if (millis() - previousMillis >= READ_INTERVAL) {
    previousMillis = millis();

    // Soil
    g_soilRaw     = readAverage(SOIL_PIN);
    g_soilPercent = map(g_soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
    g_soilPercent = constrain(g_soilPercent, 0, 100);

    // pH
    int phRaw     = readAverage(PH_PIN);
    float voltage = phRaw * (3.3 / 4095.0);
    g_pH          = constrain((ph_slope * voltage) + ph_offset, 0, 14);

    // DHT
    g_humidity = dht.readHumidity();
    g_temp     = dht.readTemperature();
    if (isnan(g_humidity) || isnan(g_temp)) {
      Serial.println("DHT ERROR"); return;
    }

    // Status strings
    if      (g_soilPercent < 20) g_soilStatus = "VeryDry";
    else if (g_soilPercent < 40) g_soilStatus = "Dry";
    else if (g_soilPercent < 70) g_soilStatus = "Moist";
    else                          g_soilStatus = "Wet";

    if      (g_pH < 5.5)  g_phStatus = "Acidic";
    else if (g_pH <= 7.5) g_phStatus = "Neutral";
    else                   g_phStatus = "Alkaline";

    if      (g_temp < 15) g_tempStatus = "Cold";
    else if (g_temp < 35) g_tempStatus = "Normal";
    else                   g_tempStatus = "Hot";

    // Alerts
    if (g_soilPercent < 30) {
      digitalWrite(LED_PIN, HIGH);
      dryAlert();
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    showLCDPage(lcdPage);

    Serial.printf("Temp: %.1f C | Humid: %.1f%% | Soil: %d%% | pH: %.2f\n",
                  g_temp, g_humidity, g_soilPercent, g_pH);
  }
}
