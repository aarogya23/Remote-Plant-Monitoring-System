#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>

// ================= ESP32 ACCESS POINT =================
const char* ssid = "SmartAgroSystem";
const char* password = "12345678";

// ================= PIN SETUP =================
#define PH_PIN        34
#define SOIL_PIN      35
#define DHT_PIN       4
#define DHT_TYPE      DHT11
#define BUZZER_PIN    19
#define LED_PIN       2
#define LDR_PIN       33   // Digital LDR: LOW = Light, HIGH = Dark
#define VIB_PIN       32   // Digital Vibration sensor (SW-420): HIGH = vibrating

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
bool   g_ldrDark = false;       // true = dark/night, false = light/day
bool   g_vibrating = false;     // true = vibration detected
String g_soilStatus = "", g_phStatus = "", g_tempStatus = "", g_ldrStatus = "", g_vibStatus = "";

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
    lcd.print("Light:");
    lcd.print(g_ldrStatus);
    lcd.setCursor(0,1);
    lcd.print("IP:");
    lcd.print(WiFi.softAPIP());
  } else if (page == 3) {
    lcd.setCursor(0,0);
    lcd.print("IP:");
    lcd.print(WiFi.softAPIP());
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
  // Soil penalty
  if (g_soilPercent < 20) score -= 30;
  else if (g_soilPercent < 40) score -= 15;
  else if (g_soilPercent > 85) score -= 10;
  // pH penalty
  if (g_pH < 5.0 || g_pH > 8.0) score -= 25;
  else if (g_pH < 5.5 || g_pH > 7.5) score -= 10;
  // Temp penalty
  if (g_temp < 10 || g_temp > 40) score -= 25;
  else if (g_temp < 15 || g_temp > 35) score -= 10;
  // Humidity penalty
  if (g_humidity < 20 || g_humidity > 90) score -= 20;
  else if (g_humidity < 30 || g_humidity > 80) score -= 8;
  // LDR penalty (plants need light)
  if (g_ldrDark) score -= 15;
  // Vibration penalty (plant being disturbed)
  if (g_vibrating) score -= 10;
  if (score < 0) score = 0;
  return score;
}

// ================= WEBPAGE =================
void handleRoot() {

  int health = calcHealth();
  String healthColor = health >= 75 ? "#22c55e" : health >= 50 ? "#f59e0b" : "#ef4444";
  String alertBg     = (g_soilPercent < 30) ? "#fef2f2" : "#f0fdf4";
  String alertBorder = (g_soilPercent < 30) ? "#ef4444" : "#22c55e";
  String alertText   = (g_soilPercent < 30) ? "#dc2626" : "#16a34a";
  String alertMsg    = (g_soilPercent < 30) ? "&#9888; WATER YOUR PLANT NOW!" : "&#10003; Plant is healthy &amp; happy";
  String alertIcon   = (g_soilPercent < 30) ? "&#128167;" : "&#127807;";

  // Soil bar color
  String soilColor = (g_soilPercent < 30) ? "#ef4444" : (g_soilPercent < 40) ? "#f59e0b" : "#22c55e";
  // pH bar color
  String phColor   = (g_pH < 5.5 || g_pH > 7.5) ? "#f59e0b" : "#22c55e";
  // Temp bar color
  String tempColor = (g_temp < 15 || g_temp > 35) ? "#ef4444" : "#22c55e";
  // Humidity bar color
  String humColor  = (g_humidity < 30 || g_humidity > 80) ? "#f59e0b" : "#3b82f6";
  // LDR
  String ldrColor  = g_ldrDark ? "#6366f1" : "#f59e0b";
  String ldrBar    = g_ldrDark ? "20" : "85";
  // Vibration
  String vibColor  = g_vibrating ? "#ef4444" : "#22c55e";
  String vibBar    = g_vibrating ? "90" : "5";

  // Bar widths (as percent of 16px scale)
  int soilBar = g_soilPercent;
  int phBar   = (int)((g_pH / 14.0) * 100.0);
  int tempBar = (int)((g_temp / 50.0) * 100.0); if(tempBar>100)tempBar=100;
  int humBar  = (int)g_humidity;

  // Status labels
  String soilLabel = g_soilStatus;
  String phLabel   = g_phStatus;
  String tempLabel = g_tempStatus;

  // Circumference for SVG ring: r=54 => circ=339.3
  float circ   = 339.3;
  float ringOff = circ - (health / 100.0) * circ;

  String html = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="refresh" content="3">
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

@keyframes shake {
  0%, 100% { transform: translateX(0); }
  15%       { transform: translateX(-4px) rotate(-1deg); }
  30%       { transform: translateX(4px) rotate(1deg); }
  45%       { transform: translateX(-3px); }
  60%       { transform: translateX(3px); }
  75%       { transform: translateX(-2px); }
  90%       { transform: translateX(2px); }
}

.card-shake {
  animation: shake 0.6s ease infinite;
  border-color: #fca5a5 !important;
  box-shadow: 0 0 0 3px #fee2e2 !important;
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
  border: 1.5px solid )rawliteral" + alertBorder + R"rawliteral(;
  background: )rawliteral" + alertBg + R"rawliteral(;
  color: )rawliteral" + alertText + R"rawliteral(;
  font-size: 14px;
  font-weight: 600;
  display: flex;
  align-items: center;
  gap: 10px;
}

/* ---- SENSOR CARDS ---- */
.cards-row {
  display: grid;
  grid-template-columns: repeat(6, 1fr);
  gap: 14px;
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
  transition: width 0.5s ease;
}

.status-dot {
  width: 7px; height: 7px;
  border-radius: 50%;
  flex-shrink: 0;
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
  gap: 0;
}

/* SVG sparkline */
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
}

/* ---- IP FOOTER ---- */
.ip-footer {
  text-align: center;
  font-size: 11px;
  color: var(--muted);
  font-family: var(--mono);
  padding-bottom: 8px;
}

/* ---- RESPONSIVE ---- */
@media (max-width: 1200px) {
  .cards-row { grid-template-columns: repeat(3, 1fr); }
}
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
        <div class="device-status">&#8226; Online</div>
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

    <!-- ALERT BANNER -->
    <div class="alert-banner">
      <span style="font-size:18px">)rawliteral" + alertIcon + R"rawliteral(</span>
      )rawliteral" + alertMsg + R"rawliteral(
    </div>

    <!-- SENSOR CARDS -->
    <div class="cards-row">

      <!-- TEMPERATURE -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#127777;</div>
          <span class="card-delta" style="color:)rawliteral" + tempColor + R"rawliteral(">)rawliteral" + g_tempStatus + R"rawliteral(</span>
        </div>
        <div class="card-label">Temperature</div>
        <div>
          <span class="card-value" style="color:)rawliteral" + tempColor + R"rawliteral(">)rawliteral" + String(g_temp,1) + R"rawliteral(</span>
          <span class="card-unit">&deg;C</span>
        </div>
        <div class="card-bar-track">
          <div class="card-bar-fill" style="width:)rawliteral" + String(tempBar) + R"rawliteral(%;background:)rawliteral" + tempColor + R"rawliteral("></div>
        </div>
        <div class="card-sub">
          <div class="status-dot" style="background:)rawliteral" + tempColor + R"rawliteral("></div>
          <span style="color:)rawliteral" + tempColor + R"rawliteral(">)rawliteral" + g_tempStatus + R"rawliteral(</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;Optimal 18&ndash;25&deg;C</span>
        </div>
      </div>

      <!-- HUMIDITY -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#128167;</div>
          <span class="card-delta" style="color:)rawliteral" + humColor + R"rawliteral(">)rawliteral" + String(g_humidity,1) + R"rawliteral(%</span>
        </div>
        <div class="card-label">Humidity</div>
        <div>
          <span class="card-value" style="color:)rawliteral" + humColor + R"rawliteral(">)rawliteral" + String(g_humidity,1) + R"rawliteral(</span>
          <span class="card-unit">%</span>
        </div>
        <div class="card-bar-track">
          <div class="card-bar-fill" style="width:)rawliteral" + String(humBar) + R"rawliteral(%;background:)rawliteral" + humColor + R"rawliteral("></div>
        </div>
        <div class="card-sub">
          <div class="status-dot" style="background:)rawliteral" + humColor + R"rawliteral("></div>
          <span style="color:var(--muted);font-weight:400">Optimal 40&ndash;70%</span>
        </div>
      </div>

      <!-- SOIL MOISTURE -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#127807;</div>
          <span class="card-delta" style="color:)rawliteral" + soilColor + R"rawliteral(">)rawliteral" + g_soilStatus + R"rawliteral(</span>
        </div>
        <div class="card-label">Soil Moisture</div>
        <div>
          <span class="card-value" style="color:)rawliteral" + soilColor + R"rawliteral(">)rawliteral" + String(g_soilPercent) + R"rawliteral(</span>
          <span class="card-unit">%</span>
        </div>
        <div class="card-bar-track">
          <div class="card-bar-fill" style="width:)rawliteral" + String(soilBar) + R"rawliteral(%;background:)rawliteral" + soilColor + R"rawliteral("></div>
        </div>
        <div class="card-sub">
          <div class="status-dot" style="background:)rawliteral" + soilColor + R"rawliteral("></div>
          <span style="color:)rawliteral" + soilColor + R"rawliteral(">)rawliteral" + g_soilStatus + R"rawliteral(</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;Optimal 35&ndash;65%</span>
        </div>
      </div>

      <!-- PH -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#9878;</div>
          <span class="card-delta" style="color:)rawliteral" + phColor + R"rawliteral(">)rawliteral" + g_phStatus + R"rawliteral(</span>
        </div>
        <div class="card-label">Soil pH</div>
        <div>
          <span class="card-value" style="color:)rawliteral" + phColor + R"rawliteral(">)rawliteral" + String(g_pH,2) + R"rawliteral(</span>
          <span class="card-unit">pH</span>
        </div>
        <div class="card-bar-track">
          <div class="card-bar-fill" style="width:)rawliteral" + String(phBar) + R"rawliteral(%;background:)rawliteral" + phColor + R"rawliteral("></div>
        </div>
        <div class="card-sub">
          <div class="status-dot" style="background:)rawliteral" + phColor + R"rawliteral("></div>
          <span style="color:)rawliteral" + phColor + R"rawliteral(">)rawliteral" + g_phStatus + R"rawliteral(</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;Optimal 5.5&ndash;7.5</span>
        </div>
      </div>

      <!-- LDR LIGHT SENSOR -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#9728;</div>
          <span class="card-delta" style="color:)rawliteral" + ldrColor + R"rawliteral(">)rawliteral" + g_ldrStatus + R"rawliteral(</span>
        </div>
        <div class="card-label">Light Level</div>
        <div style="margin-top:6px;">
          <span style="font-size:28px; font-weight:800; font-family:'DM Mono',monospace; color:)rawliteral" + ldrColor + R"rawliteral(">)rawliteral" + (g_ldrDark ? "DARK" : "BRIGHT") + R"rawliteral(</span>
        </div>
        <div class="card-bar-track">
          <div class="card-bar-fill" style="width:)rawliteral" + ldrBar + R"rawliteral(%;background:)rawliteral" + ldrColor + R"rawliteral("></div>
        </div>
        <div class="card-sub">
          <div class="status-dot" style="background:)rawliteral" + ldrColor + R"rawliteral("></div>
          <span style="color:)rawliteral" + ldrColor + R"rawliteral(">)rawliteral" + (g_ldrDark ? "No sunlight" : "Good light") + R"rawliteral(</span>
        </div>
      </div>

      <!-- VIBRATION SENSOR -->
      <div class="card )rawliteral" + (g_vibrating ? "card-shake" : "") + R"rawliteral(">
        <div class="card-header">
          <div class="card-icon">&#128246;</div>
          <span class="card-delta" style="color:)rawliteral" + vibColor + R"rawliteral(">)rawliteral" + g_vibStatus + R"rawliteral(</span>
        </div>
        <div class="card-label">Vibration</div>
        <div style="margin-top:6px; display:flex; align-items:center; gap:8px;">
          <span style="font-size:28px; font-weight:800; font-family:'DM Mono',monospace; color:)rawliteral" + vibColor + R"rawliteral(">)rawliteral" + (g_vibrating ? "ACTIVE" : "CALM") + R"rawliteral(</span>
          )rawliteral" + (g_vibrating ? R"rawliteral(<span style="font-size:20px; animation:shake 0.4s infinite;">&#128246;</span>)rawliteral" : "") + R"rawliteral(
        </div>
        <div class="card-bar-track">
          <div class="card-bar-fill" style="width:)rawliteral" + vibBar + R"rawliteral(%;background:)rawliteral" + vibColor + R"rawliteral(; transition:width 0.2s;"></div>
        </div>
        <div class="card-sub">
          <div class="status-dot" style="background:)rawliteral" + vibColor + R"rawliteral(; )rawliteral" + (g_vibrating ? "animation:pulse 0.3s infinite;" : "") + R"rawliteral("></div>
          <span style="color:)rawliteral" + vibColor + R"rawliteral(">)rawliteral" + (g_vibrating ? "Movement detected!" : "No movement") + R"rawliteral(</span>
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
            <!-- Gridlines -->
            <line x1="0" y1="33"  x2="600" y2="33"  stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="66"  x2="600" y2="66"  stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="100" x2="600" y2="100" stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="133" x2="600" y2="133" stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="166" x2="600" y2="166" stroke="#f1f5f9" stroke-width="1"/>
            <!-- Temp line (scale: 0-50°C → 200-0px) -->
            <polyline id="tempLine" fill="none" stroke="#ef4444" stroke-width="2" stroke-linejoin="round"/>
            <!-- Humidity line -->
            <polyline id="humLine"  fill="none" stroke="#3b82f6" stroke-width="2" stroke-linejoin="round"/>
            <!-- Soil line -->
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
            <circle cx="75" cy="75" r="54"
              fill="none"
              stroke=")rawliteral" + healthColor + R"rawliteral("
              stroke-width="11"
              stroke-linecap="round"
              stroke-dasharray=")rawliteral" + String(circ,1) + R"rawliteral("
              stroke-dashoffset=")rawliteral" + String(ringOff,1) + R"rawliteral("
              transform="rotate(-90 75 75)"/>
            <text x="75" y="69" text-anchor="middle"
              font-family="'DM Mono',monospace"
              font-size="26" font-weight="700"
              fill=")rawliteral" + healthColor + R"rawliteral(">)rawliteral" + String(health) + R"rawliteral(%</text>
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
              <div class="metric-bar-fill" style="width:)rawliteral" + String(tempBar) + R"rawliteral(%;background:)rawliteral" + tempColor + R"rawliteral("></div>
            </div>
            <span class="metric-val">)rawliteral" + String(g_temp,1) + R"rawliteral(&deg;C</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#128167;</span>
            <span class="metric-name">Humidity</span>
            <div class="metric-bar">
              <div class="metric-bar-fill" style="width:)rawliteral" + String(humBar) + R"rawliteral(%;background:)rawliteral" + humColor + R"rawliteral("></div>
            </div>
            <span class="metric-val">)rawliteral" + String(g_humidity,1) + R"rawliteral(%</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#127807;</span>
            <span class="metric-name">Soil</span>
            <div class="metric-bar">
              <div class="metric-bar-fill" style="width:)rawliteral" + String(soilBar) + R"rawliteral(%;background:)rawliteral" + soilColor + R"rawliteral("></div>
            </div>
            <span class="metric-val">)rawliteral" + String(g_soilPercent) + R"rawliteral(%</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#9878;</span>
            <span class="metric-name">pH</span>
            <div class="metric-bar">
              <div class="metric-bar-fill" style="width:)rawliteral" + String(phBar) + R"rawliteral(%;background:)rawliteral" + phColor + R"rawliteral("></div>
            </div>
            <span class="metric-val">)rawliteral" + String(g_pH,2) + R"rawliteral(</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#9728;</span>
            <span class="metric-name">Light</span>
            <div class="metric-bar">
              <div class="metric-bar-fill" style="width:)rawliteral" + ldrBar + R"rawliteral(%;background:)rawliteral" + ldrColor + R"rawliteral("></div>
            </div>
            <span class="metric-val" style="color:)rawliteral" + ldrColor + R"rawliteral(">)rawliteral" + g_ldrStatus + R"rawliteral(</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#128246;</span>
            <span class="metric-name">Vibration</span>
            <div class="metric-bar">
              <div class="metric-bar-fill" style="width:)rawliteral" + vibBar + R"rawliteral(%;background:)rawliteral" + vibColor + R"rawliteral("></div>
            </div>
            <span class="metric-val" style="color:)rawliteral" + vibColor + R"rawliteral(">)rawliteral" + g_vibStatus + R"rawliteral(</span>
          </div>
        </div>

      </div><!-- end health-card -->
    </div><!-- end bottom-row -->

    <div class="ip-footer">ESP32 AP &nbsp;|&nbsp; )rawliteral" + WiFi.softAPIP().toString() + R"rawliteral( &nbsp;|&nbsp; SSID: SmartAgroSystem</div>

  </div><!-- end content -->
</div><!-- end main -->

<script>
// ---- Clock ----
function updateClock() {
  var now = new Date();
  var h = String(now.getHours()).padStart(2,'0');
  var m = String(now.getMinutes()).padStart(2,'0');
  var s = String(now.getSeconds()).padStart(2,'0');
  var t = h+':'+m+':'+s;
  document.getElementById('clockDisp').textContent = t;
  document.getElementById('lastRead').textContent = 'Last reading: ' + t;
}
updateClock();
setInterval(updateClock, 1000);

// ---- Rolling sparkline chart ----
var MAX_POINTS = 40;
var tempData = [], humData = [], soilData = [];

// Seed with current values
var curTemp = )rawliteral" + String(g_temp, 1) + R"rawliteral(;
var curHum  = )rawliteral" + String(g_humidity, 1) + R"rawliteral(;
var curSoil = )rawliteral" + String(g_soilPercent) + R"rawliteral(;

for(var i=0; i<MAX_POINTS; i++) {
  tempData.push(curTemp + (Math.random()-0.5)*0.5);
  humData.push(curHum   + (Math.random()-0.5)*0.5);
  soilData.push(curSoil + (Math.random()-0.5)*0.5);
}

function toSVGPoints(data, minVal, maxVal) {
  var pts = '';
  for(var i=0; i<data.length; i++) {
    var x = (i / (MAX_POINTS-1)) * 600;
    var y = 200 - ((data[i] - minVal) / (maxVal - minVal)) * 200;
    y = Math.max(2, Math.min(198, y));
    pts += x.toFixed(1)+','+y.toFixed(1)+' ';
  }
  return pts.trim();
}

function updateChart() {
  // Gently drift toward current real values
  var lt = tempData[tempData.length-1];
  var lh = humData[humData.length-1];
  var ls = soilData[soilData.length-1];
  tempData.push(lt + (curTemp - lt)*0.3 + (Math.random()-0.5)*0.3);
  humData.push( lh + (curHum  - lh)*0.3 + (Math.random()-0.5)*0.3);
  soilData.push(ls + (curSoil - ls)*0.3 + (Math.random()-0.5)*0.3);
  if(tempData.length > MAX_POINTS) { tempData.shift(); humData.shift(); soilData.shift(); }

  document.getElementById('tempLine').setAttribute('points', toSVGPoints(tempData, 0, 60));
  document.getElementById('humLine').setAttribute('points',  toSVGPoints(humData,  0, 100));
  document.getElementById('soilLine').setAttribute('points', toSVGPoints(soilData, 0, 100));
}

// Draw immediately and keep animating
updateChart();
setInterval(updateChart, 800);

// ---- Water Low Sound Alert ----
var soilPercent = )rawliteral" + String(g_soilPercent) + R"rawliteral(;
var waterIsLow  = soilPercent < 30;

// Unlock AudioContext on first user tap (browser policy)
var audioCtx = null;
function getAudioCtx() {
  if (!audioCtx) {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  }
  return audioCtx;
}
document.addEventListener('click', function() { getAudioCtx(); }, { once: true });

function playNote(ctx, freq, startTime, duration, vol, type) {
  var osc  = ctx.createOscillator();
  var gain = ctx.createGain();
  osc.connect(gain);
  gain.connect(ctx.destination);
  osc.type = type || 'sine';
  osc.frequency.value = freq;
  gain.gain.setValueAtTime(0.001, ctx.currentTime + startTime);
  gain.gain.linearRampToValueAtTime(vol, ctx.currentTime + startTime + 0.02);
  gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + startTime + duration);
  osc.start(ctx.currentTime + startTime);
  osc.stop(ctx.currentTime + startTime + duration + 0.05);
}

function playWaterAlert() {
  try {
    var ctx = getAudioCtx();
    if (ctx.state === 'suspended') ctx.resume();

    // Descending "drip drip drip" water-drop melody
    // Each note: freq, start, duration, vol, type
    playNote(ctx, 1200, 0.00, 0.12, 0.5, 'sine');
    playNote(ctx, 900,  0.18, 0.12, 0.5, 'sine');
    playNote(ctx, 650,  0.36, 0.18, 0.6, 'sine');
    // pause
    playNote(ctx, 1200, 0.70, 0.12, 0.5, 'sine');
    playNote(ctx, 900,  0.88, 0.12, 0.5, 'sine');
    playNote(ctx, 650,  1.06, 0.18, 0.6, 'sine');
    // final low urgent tone
    playNote(ctx, 440,  1.40, 0.40, 0.7, 'triangle');
  } catch(e) {
    console.warn('Audio alert failed:', e);
  }
}

// Play every time page loads while water is low
// (page auto-refreshes every 3s, so alert repeats until watered)
if (waterIsLow) {
  // Small delay so page renders first, then sound plays
  setTimeout(playWaterAlert, 600);
}
</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);
  Wire.begin();
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(VIB_PIN, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(2,0); lcd.print("AGRO SYSTEM");
  lcd.setCursor(3,1); lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  WiFi.softAP(ssid, password);
  Serial.println("ESP32 WiFi Started!");
  Serial.print("Connect to: "); Serial.println(ssid);
  Serial.print("Open browser: http://"); Serial.println(WiFi.softAPIP());

  lcd.setCursor(0,0); lcd.print("WiFi Started");
  lcd.setCursor(0,1); lcd.print(WiFi.softAPIP());
  delay(3000);
  lcd.clear();

  server.on("/", handleRoot);
  server.begin();

  beep(1000, 100); delay(100); beep(1500, 100);
}

// ================= LOOP =================
void loop(){
  server.handleClient();

  if(millis()-lcdMillis >= LCD_PAGE_TIME){
    lcdMillis = millis();
    lcdPage = (lcdPage + 1) % 4;
    showLCDPage(lcdPage);
  }

  if(millis()-previousMillis >= READ_INTERVAL){
    previousMillis = millis();

    // Soil
    g_soilRaw     = readAverage(SOIL_PIN);
    g_soilPercent = map(g_soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
    g_soilPercent = constrain(g_soilPercent, 0, 100);

    // pH
    int phRaw   = readAverage(PH_PIN);
    float voltage = phRaw * (3.3 / 4095.0);
    g_pH        = constrain((ph_slope * voltage) + ph_offset, 0, 14);

    // DHT
    g_humidity = dht.readHumidity();
    g_temp     = dht.readTemperature();
    if(isnan(g_humidity) || isnan(g_temp)){
      Serial.println("DHT ERROR"); return;
    }

    // LDR (digital: LOW = light present, HIGH = dark)
    g_ldrDark   = digitalRead(LDR_PIN);
    g_ldrStatus = g_ldrDark ? "Dark" : "Bright";

    // Vibration sensor (SW-420: HIGH = vibrating, LOW = stable)
    g_vibrating  = digitalRead(VIB_PIN);
    g_vibStatus  = g_vibrating ? "Shaking" : "Stable";

    // Status strings
    if(g_soilPercent < 20)       g_soilStatus = "VeryDry";
    else if(g_soilPercent < 40)  g_soilStatus = "Dry";
    else if(g_soilPercent < 70)  g_soilStatus = "Moist";
    else                          g_soilStatus = "Wet";

    if(g_pH < 5.5)               g_phStatus = "Acidic";
    else if(g_pH <= 7.5)         g_phStatus = "Neutral";
    else                          g_phStatus = "Alkaline";

    if(g_temp < 15)              g_tempStatus = "Cold";
    else if(g_temp < 35)         g_tempStatus = "Normal";
    else                          g_tempStatus = "Hot";

    // Alerts
    if(g_soilPercent < 30){
      digitalWrite(LED_PIN, HIGH);
      dryAlert();
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    showLCDPage(lcdPage);
  }
}
