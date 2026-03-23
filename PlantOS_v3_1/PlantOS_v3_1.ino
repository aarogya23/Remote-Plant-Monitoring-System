// ================================================================
//  PlantOS v3 — ESP32 Hotspot Dashboard
//  Sensors : Capacitive Soil | BH1750 Light | DHT22 | pH | 2×18650
//  Output  : LCD 16×2 I2C | Buzzer | LED | Web Dashboard (AJAX)
// ================================================================

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BH1750.h>          // "BH1750" by Christopher Laws
#include <WiFi.h>
#include <WebServer.h>

// ================================================================
//  HOTSPOT CONFIG
// ================================================================
const char* ap_ssid     = "PlantOS";
const char* ap_password = "plant1234";
IPAddress   ap_ip(192, 168, 4, 1);

// ================================================================
//  PIN DEFINITIONS
// ================================================================
#define SOIL_PIN     2    // Capacitive soil AOUT  → GPIO2
#define PH_PIN      34    // pH sensor AOUT        → GPIO34 (ADC1, input-only)
#define DHT_PIN      4    // DHT22 data pin
#define DHT_TYPE  DHT22
#define BUZZER_PIN  19
#define LED_PIN     18    // LED moved from GPIO2  → GPIO18
#define BAT_PIN     33    // Voltage divider        → GPIO33 (ADC1)
// BH1750 → SDA=GPIO21, SCL=GPIO22  (I2C 0x23, ADDR pin → GND)
// LCD    → SDA=GPIO21, SCL=GPIO22  (I2C 0x27, shared bus)

// ================================================================
//  OBJECTS
// ================================================================
DHT               dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
BH1750            lightMeter;
WebServer         server(80);

// ================================================================
//  CALIBRATION & CONSTANTS
// ================================================================

// ---- Capacitive soil (HIGH=dry, LOW=wet — inverse of resistive) ----
int SOIL_DRY = 3200;    // raw ADC in open air   — tune after calibration
int SOIL_WET = 1500;    // raw ADC in water       — tune after calibration

// ---- pH sensor ----
// pH = ph_slope × Voltage + ph_offset
// Default slope ≈ -5.70 for most cheap analog pH modules (inverse: higher V = lower pH)
// Calibrate with pH 4 and pH 7 buffer solutions and adjust below
float PH_SLOPE  = -5.70;
float PH_OFFSET =  21.34;   // tune: PH_OFFSET = pH_expected - PH_SLOPE × Vmeasured

// ---- 2S 18650 battery (8.4V full, 6.0V empty) ----
// Voltage divider: R1=100kΩ (BAT+→pin), R2=47kΩ (pin→GND)
// Vpin = Vbat × R2/(R1+R2)  →  Vbat = Vpin × (R1+R2)/R2
const float R1          = 100000.0;
const float R2          =  47000.0;
const float VREF        =       3.3;
const float BAT_FULL_V  =       8.4;
const float BAT_EMPTY_V =       6.0;

// ================================================================
//  GLOBAL SENSOR STATE
// ================================================================
float  g_temp = 0, g_humidity = 0, g_lux = 0, g_pH = 0;
int    g_soilPercent = 0, g_soilRaw = 0;
float  g_batVoltage  = 0;
int    g_batPercent  = 0;

String g_soilStatus  = "---";
String g_tempStatus  = "---";
String g_lightStatus = "---";
String g_phStatus    = "---";
String g_batStatus   = "---";

unsigned long previousMillis = 0;
unsigned long lcdMillis      = 0;
byte          lcdPage        = 0;

const unsigned long READ_INTERVAL = 2000;
const unsigned long LCD_PAGE_TIME = 3000;
const byte          SAMPLE_COUNT  = 30;

// ================================================================
//  HELPERS
// ================================================================
int readAverage(byte pin) {
  long total = 0;
  for (byte i = 0; i < SAMPLE_COUNT; i++) {
    total += analogRead(pin);
    delay(5);
  }
  return total / SAMPLE_COUNT;
}

void beep(int freq, int ms) { tone(BUZZER_PIN, freq); delay(ms); noTone(BUZZER_PIN); }
void dryAlert()  { for (int i=0;i<3;i++){ beep(2000,200); delay(100); } }
void batAlert()  { beep(800,500); delay(200); beep(800,500); }
void phAlert()   { beep(1200,300); delay(150); beep(1200,300); }

// ================================================================
//  READ BATTERY
// ================================================================
void readBattery() {
  int   raw    = readAverage(BAT_PIN);
  float vPin   = raw * (VREF / 4095.0);
  g_batVoltage = vPin * ((R1 + R2) / R2);
  g_batPercent = (int)((g_batVoltage - BAT_EMPTY_V) /
                       (BAT_FULL_V   - BAT_EMPTY_V) * 100.0);
  g_batPercent = constrain(g_batPercent, 0, 100);

  if      (g_batPercent > 70) g_batStatus = "Good";
  else if (g_batPercent > 30) g_batStatus = "Low";
  else                         g_batStatus = "Critical";
}

// ================================================================
//  LCD PAGES  (4 pages covering all sensors)
// ================================================================
void showLCDPage(byte page) {
  lcd.clear();
  switch (page) {
    case 0:
      lcd.setCursor(0,0);
      lcd.print("Soil:"); lcd.print(g_soilPercent); lcd.print("% "); lcd.print(g_soilStatus.substring(0,6));
      lcd.setCursor(0,1);
      lcd.print("T:"); lcd.print(g_temp,1); lcd.print("C H:"); lcd.print((int)g_humidity); lcd.print("%");
      break;
    case 1:
      lcd.setCursor(0,0);
      lcd.print("pH:"); lcd.print(g_pH,2); lcd.print(" "); lcd.print(g_phStatus.substring(0,7));
      lcd.setCursor(0,1);
      lcd.print("Light:"); lcd.print((int)g_lux); lcd.print("lx");
      break;
    case 2:
      lcd.setCursor(0,0);
      lcd.print("Bat:"); lcd.print(g_batVoltage,1); lcd.print("V "); lcd.print(g_batPercent); lcd.print("%");
      lcd.setCursor(0,1);
      lcd.print(g_batStatus); lcd.print(" ");
      if (g_soilPercent < 30) lcd.print("WATER!");
      else lcd.print("OK :)");
      break;
    case 3:
      lcd.setCursor(0,0);
      lcd.print("IP:"); lcd.print(ap_ip);
      lcd.setCursor(0,1);
      lcd.print("Health:"); lcd.print("see app");
      break;
  }
}

// ================================================================
//  HEALTH SCORE  (0–100)
// ================================================================
int calcHealth() {
  int score = 100;

  // Soil
  if      (g_soilPercent < 20) score -= 30;
  else if (g_soilPercent < 40) score -= 15;
  else if (g_soilPercent > 85) score -= 10;

  // pH (most plants: 5.5–7.5)
  if      (g_pH < 5.0 || g_pH > 8.0) score -= 25;
  else if (g_pH < 5.5 || g_pH > 7.5) score -= 10;

  // Temperature
  if      (g_temp < 10 || g_temp > 40) score -= 25;
  else if (g_temp < 15 || g_temp > 35) score -= 10;

  // Humidity
  if      (g_humidity < 20 || g_humidity > 90) score -= 20;
  else if (g_humidity < 30 || g_humidity > 80) score -=  8;

  // Light
  if      (g_lux <  100)  score -= 20;
  else if (g_lux <  500)  score -= 10;
  else if (g_lux > 80000) score -= 10;

  // Battery
  if (g_batPercent < 10) score -= 15;

  return constrain(score, 0, 100);
}

// ================================================================
//  JSON ENDPOINT  /data
// ================================================================
void handleData() {
  int health = calcHealth();
  String j = "{";
  j += "\"temp\":"          + String(g_temp,1)       + ",";
  j += "\"humidity\":"      + String(g_humidity,1)   + ",";
  j += "\"soil\":"          + String(g_soilPercent)  + ",";
  j += "\"ph\":"            + String(g_pH,2)         + ",";
  j += "\"lux\":"           + String((int)g_lux)     + ",";
  j += "\"batV\":"          + String(g_batVoltage,2) + ",";
  j += "\"batPct\":"        + String(g_batPercent)   + ",";
  j += "\"health\":"        + String(health)         + ",";
  j += "\"soilStatus\":\""  + g_soilStatus           + "\",";
  j += "\"phStatus\":\""    + g_phStatus             + "\",";
  j += "\"tempStatus\":\""  + g_tempStatus           + "\",";
  j += "\"lightStatus\":\"" + g_lightStatus          + "\",";
  j += "\"batStatus\":\""   + g_batStatus            + "\"";
  j += "}";
  server.sendHeader("Access-Control-Allow-Origin","*");
  server.send(200,"application/json",j);
}

// ================================================================
//  WEB PAGE  /
// ================================================================
void handleRoot() {
  String ip = ap_ip.toString();
  String html = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>PlantOS</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=DM+Sans:wght@400;500;600;700&family=DM+Mono:wght@400;500&display=swap" rel="stylesheet">
<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
:root{--bg:#f4f6f9;--card:#fff;--border:#e8ecf0;--text:#1a2332;--muted:#7a8694;--font:'DM Sans',sans-serif;--mono:'DM Mono',monospace}
body{font-family:var(--font);background:var(--bg);color:var(--text);display:flex;min-height:100vh}
/* SIDEBAR */
.sidebar{width:220px;background:var(--card);border-right:1px solid var(--border);display:flex;flex-direction:column;position:fixed;top:0;left:0;height:100vh;z-index:10}
.logo{display:flex;align-items:center;gap:10px;padding:22px 20px 18px;border-bottom:1px solid var(--border)}
.logo-icon{width:36px;height:36px;background:#dcfce7;border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:18px}
.logo-name{font-weight:700;font-size:17px;letter-spacing:-.3px}
.nav-section{padding:20px 12px 8px}
.nav-label{font-size:10px;font-weight:700;color:var(--muted);letter-spacing:.08em;text-transform:uppercase;padding:0 8px;margin-bottom:6px}
.nav-item{display:flex;align-items:center;gap:10px;padding:9px 12px;border-radius:9px;font-size:13.5px;font-weight:500;color:var(--muted);cursor:pointer;transition:all .15s;text-decoration:none}
.nav-item:hover{background:#f1f5f9;color:var(--text)}
.nav-item.active{background:#f0fdf4;color:#16a34a;font-weight:600}
.nav-icon{font-size:15px;width:20px;text-align:center}
.sidebar-footer{margin-top:auto;padding:16px 20px;border-top:1px solid var(--border)}
.device-chip{display:flex;align-items:center;gap:8px;padding:10px 12px;background:#f8fafc;border-radius:10px;border:1px solid var(--border)}
.device-dot{width:8px;height:8px;background:#22c55e;border-radius:50%;box-shadow:0 0 0 3px #dcfce7;flex-shrink:0}
.device-name{font-size:12px;font-weight:600}
.device-status{font-size:11px;color:#22c55e;font-weight:500}
/* MAIN */
.main{margin-left:220px;flex:1;display:flex;flex-direction:column;min-height:100vh}
.topbar{background:var(--card);border-bottom:1px solid var(--border);padding:16px 28px;display:flex;align-items:center;justify-content:space-between;position:sticky;top:0;z-index:5}
.topbar-title h2{font-size:20px;font-weight:700;letter-spacing:-.4px}
.topbar-title p{font-size:12px;color:var(--muted);margin-top:1px;font-family:var(--mono)}
.topbar-right{display:flex;align-items:center;gap:12px}
.live-badge{display:flex;align-items:center;gap:6px;padding:5px 12px;background:#f0fdf4;border:1px solid #bbf7d0;border-radius:20px;font-size:12px;font-weight:600;color:#16a34a}
.live-dot{width:7px;height:7px;background:#22c55e;border-radius:50%;animation:pulse 1.5s infinite}
@keyframes pulse{0%,100%{opacity:1;transform:scale(1)}50%{opacity:.5;transform:scale(.85)}}
.time-display{font-family:var(--mono);font-size:13px;font-weight:500;color:var(--muted);background:#f8fafc;padding:5px 12px;border-radius:8px;border:1px solid var(--border)}
/* CONTENT */
.content{padding:24px 28px;display:flex;flex-direction:column;gap:20px}
.wifi-info{background:#eff6ff;border:1px solid #bfdbfe;border-radius:10px;padding:10px 16px;font-size:12px;color:#1d4ed8;font-family:var(--mono);display:flex;align-items:center;gap:10px;flex-wrap:wrap}
.alert-banner{padding:12px 18px;border-radius:12px;border:1.5px solid #22c55e;background:#f0fdf4;color:#16a34a;font-size:14px;font-weight:600;display:flex;align-items:center;gap:10px;transition:all .4s}
/* 3-column card grid */
.cards-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:16px}
.card{background:var(--card);border-radius:14px;padding:18px 20px 14px;border:1px solid var(--border);box-shadow:0 1px 4px rgba(0,0,0,.04)}
.card-header{display:flex;justify-content:space-between;align-items:flex-start;margin-bottom:12px}
.card-icon{width:40px;height:40px;border-radius:11px;background:#f8fafc;border:1px solid var(--border);display:flex;align-items:center;justify-content:center;font-size:18px}
.card-delta{font-size:11.5px;font-weight:700;font-family:var(--mono);transition:color .3s}
.card-label{font-size:12px;color:var(--muted);font-weight:500;margin-bottom:2px}
.card-value{font-size:34px;font-weight:800;font-family:var(--mono);letter-spacing:-1px;line-height:1;transition:color .3s}
.card-unit{font-size:15px;font-weight:500;color:var(--muted);margin-left:2px}
.card-sub{margin-top:10px;display:flex;align-items:center;gap:6px;font-size:11.5px;font-weight:600}
.card-bar-track{height:5px;background:#f1f5f9;border-radius:10px;margin-top:10px;overflow:hidden}
.card-bar-fill{height:100%;border-radius:10px;transition:width .6s ease,background .3s}
.status-dot{width:7px;height:7px;border-radius:50%;flex-shrink:0;transition:background .3s}
/* BOTTOM ROW */
.bottom-row{display:grid;grid-template-columns:1fr 320px;gap:16px}
.chart-card{background:var(--card);border-radius:14px;padding:20px 24px;border:1px solid var(--border);box-shadow:0 1px 4px rgba(0,0,0,.04)}
.chart-title{font-size:15px;font-weight:700}
.chart-sub{font-size:12px;color:var(--muted);margin-top:2px}
.chart-area{width:100%;height:220px;margin-top:16px}
.sparkline{width:100%;height:100%}
.legend{display:flex;gap:14px;margin-top:12px;flex-wrap:wrap}
.legend-item{display:flex;align-items:center;gap:6px;font-size:12px;color:var(--muted);font-weight:500}
.legend-dot{width:10px;height:10px;border-radius:50%}
/* HEALTH CARD */
.health-card{background:var(--card);border-radius:14px;padding:20px 24px;border:1px solid var(--border);box-shadow:0 1px 4px rgba(0,0,0,.04)}
.health-title{font-size:15px;font-weight:700}
.health-sub{font-size:12px;color:var(--muted);margin-top:2px}
.health-ring-wrap{display:flex;justify-content:center;margin:14px 0}
.health-metrics{display:flex;flex-direction:column;gap:9px}
.metric-row{display:flex;align-items:center;gap:8px;font-size:12px}
.metric-icon{font-size:13px;width:18px;text-align:center}
.metric-name{flex:1;font-weight:500;color:var(--muted)}
.metric-val{font-family:var(--mono);font-size:12px;font-weight:600;transition:color .3s}
.metric-bar{width:72px;height:4px;background:#f1f5f9;border-radius:10px;overflow:hidden}
.metric-bar-fill{height:100%;border-radius:10px;transition:width .6s ease,background .3s}
.ip-footer{text-align:center;font-size:11px;color:var(--muted);font-family:var(--mono);padding-bottom:8px}
@keyframes flashUpdate{0%{opacity:1}30%{opacity:.4}100%{opacity:1}}
.updated{animation:flashUpdate .4s ease}
@media(max-width:960px){
  .sidebar{display:none}.main{margin-left:0}
  .cards-grid{grid-template-columns:repeat(2,1fr)}
  .bottom-row{grid-template-columns:1fr}
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
    <a class="nav-item active" href="#"><span class="nav-icon">&#9783;</span> Dashboard</a>
    <a class="nav-item" href="#"><span class="nav-icon">&#128200;</span> Reports</a>
    <a class="nav-item" href="#"><span class="nav-icon">&#128276;</span> Alerts</a>
  </div>
  <div class="nav-section">
    <div class="nav-label">Settings</div>
    <a class="nav-item" href="#"><span class="nav-icon">&#9881;</span> Config</a>
    <a class="nav-item" href="#"><span class="nav-icon">&#128268;</span> Device</a>
  </div>
  <div class="sidebar-footer">
    <div class="device-chip">
      <div class="device-dot"></div>
      <div>
        <div class="device-name">ESP32-AGRO</div>
        <div class="device-status" id="connStatus">&#8226; Online</div>
      </div>
    </div>
  </div>
</aside>

<!-- MAIN -->
<div class="main">
  <div class="topbar">
    <div class="topbar-title">
      <h2>Plant Dashboard</h2>
      <p id="lastRead">Last reading: --:--:--</p>
    </div>
    <div class="topbar-right">
      <div class="time-display" id="clockDisp">--:--:--</div>
      <div class="live-badge"><div class="live-dot"></div> Live</div>
    </div>
  </div>

  <div class="content">

    <div class="wifi-info">
      &#128225; Hotspot: <strong>PlantOS</strong> &nbsp;|&nbsp; Password: <strong>plant1234</strong> &nbsp;|&nbsp; Open: <strong>http://192.168.4.1</strong>
    </div>

    <div class="alert-banner" id="alertBanner">
      <span style="font-size:18px" id="alertIcon">&#127807;</span>
      <span id="alertMsg">Plant is healthy &amp; happy</span>
    </div>

    <!-- ROW 1: Temp | Humidity | Soil -->
    <div class="cards-grid">

      <!-- TEMPERATURE -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#127777;</div>
          <span class="card-delta" id="tempStatus">Normal</span>
        </div>
        <div class="card-label">Temperature (DHT22)</div>
        <div><span class="card-value" id="tempVal">--</span><span class="card-unit">&deg;C</span></div>
        <div class="card-bar-track"><div class="card-bar-fill" id="tempBar" style="width:0%;background:#22c55e"></div></div>
        <div class="card-sub">
          <div class="status-dot" id="tempDot" style="background:#22c55e"></div>
          <span id="tempSubStatus" style="color:#22c55e">Normal</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;Optimal 15–35°C</span>
        </div>
      </div>

      <!-- HUMIDITY -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#128167;</div>
          <span class="card-delta" id="humDelta" style="color:#3b82f6">--%</span>
        </div>
        <div class="card-label">Humidity (DHT22)</div>
        <div><span class="card-value" id="humVal" style="color:#3b82f6">--</span><span class="card-unit">%</span></div>
        <div class="card-bar-track"><div class="card-bar-fill" id="humBar" style="width:0%;background:#3b82f6"></div></div>
        <div class="card-sub">
          <div class="status-dot" id="humDot" style="background:#3b82f6"></div>
          <span style="color:var(--muted);font-weight:400">Optimal 40–70%</span>
        </div>
      </div>

      <!-- SOIL MOISTURE -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#127807;</div>
          <span class="card-delta" id="soilStatus">Moist</span>
        </div>
        <div class="card-label">Soil Moisture (Cap.)</div>
        <div><span class="card-value" id="soilVal">--</span><span class="card-unit">%</span></div>
        <div class="card-bar-track"><div class="card-bar-fill" id="soilBar" style="width:0%;background:#22c55e"></div></div>
        <div class="card-sub">
          <div class="status-dot" id="soilDot" style="background:#22c55e"></div>
          <span id="soilSubStatus" style="color:#22c55e">Moist</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;Optimal 35–65%</span>
        </div>
      </div>

    </div><!-- end row 1 -->

    <!-- ROW 2: pH | Light | Battery -->
    <div class="cards-grid">

      <!-- pH -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#9878;</div>
          <span class="card-delta" id="phStatus" style="color:#22c55e">Neutral</span>
        </div>
        <div class="card-label">Soil pH</div>
        <div><span class="card-value" id="phVal" style="color:#22c55e">--</span><span class="card-unit">pH</span></div>
        <div class="card-bar-track"><div class="card-bar-fill" id="phBar" style="width:0%;background:#22c55e"></div></div>
        <div class="card-sub">
          <div class="status-dot" id="phDot" style="background:#22c55e"></div>
          <span id="phSubStatus" style="color:#22c55e">Neutral</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;Optimal 5.5–7.5</span>
        </div>
      </div>

      <!-- LIGHT -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#9728;</div>
          <span class="card-delta" id="lightStatus" style="color:#f59e0b">--</span>
        </div>
        <div class="card-label">Light (BH1750)</div>
        <div><span class="card-value" id="luxVal" style="color:#f59e0b">--</span><span class="card-unit">lx</span></div>
        <div class="card-bar-track"><div class="card-bar-fill" id="luxBar" style="width:0%;background:#f59e0b"></div></div>
        <div class="card-sub">
          <div class="status-dot" id="luxDot" style="background:#f59e0b"></div>
          <span id="luxSubStatus" style="color:#f59e0b">--</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;500–20k lx ideal</span>
        </div>
      </div>

      <!-- BATTERY -->
      <div class="card">
        <div class="card-header">
          <div class="card-icon">&#128267;</div>
          <span class="card-delta" id="batStatusLabel" style="color:#22c55e">Good</span>
        </div>
        <div class="card-label">Battery (2S 18650)</div>
        <div><span class="card-value" id="batPctVal">--</span><span class="card-unit">%</span></div>
        <div class="card-bar-track"><div class="card-bar-fill" id="batBar" style="width:0%;background:#22c55e"></div></div>
        <div class="card-sub">
          <div class="status-dot" id="batDot" style="background:#22c55e"></div>
          <span id="batVoltVal" style="color:#22c55e;font-family:var(--mono)">--V</span>
          <span style="color:var(--muted);font-weight:400">&nbsp;7.4V nominal</span>
        </div>
      </div>

    </div><!-- end row 2 -->

    <!-- BOTTOM ROW: Sparkline + Health Ring -->
    <div class="bottom-row">

      <div class="chart-card">
        <div class="chart-title">Sensor History</div>
        <div class="chart-sub">Live rolling window — last 40 readings</div>
        <div class="chart-area">
          <svg class="sparkline" id="sparkSVG" viewBox="0 0 600 200" preserveAspectRatio="none">
            <line x1="0" y1="40"  x2="600" y2="40"  stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="80"  x2="600" y2="80"  stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="120" x2="600" y2="120" stroke="#f1f5f9" stroke-width="1"/>
            <line x1="0" y1="160" x2="600" y2="160" stroke="#f1f5f9" stroke-width="1"/>
            <polyline id="tempLine" fill="none" stroke="#ef4444" stroke-width="2"   stroke-linejoin="round"/>
            <polyline id="humLine"  fill="none" stroke="#3b82f6" stroke-width="2"   stroke-linejoin="round"/>
            <polyline id="soilLine" fill="none" stroke="#22c55e" stroke-width="2"   stroke-linejoin="round"/>
            <polyline id="phLine"   fill="none" stroke="#8b5cf6" stroke-width="1.5" stroke-linejoin="round" stroke-dasharray="5 2"/>
            <polyline id="luxLine"  fill="none" stroke="#f59e0b" stroke-width="1.5" stroke-linejoin="round" stroke-dasharray="3 2"/>
          </svg>
        </div>
        <div class="legend">
          <div class="legend-item"><div class="legend-dot" style="background:#ef4444"></div> Temp (°C)</div>
          <div class="legend-item"><div class="legend-dot" style="background:#3b82f6"></div> Humidity (%)</div>
          <div class="legend-item"><div class="legend-dot" style="background:#22c55e"></div> Soil (%)</div>
          <div class="legend-item"><div class="legend-dot" style="background:#8b5cf6"></div> pH</div>
          <div class="legend-item"><div class="legend-dot" style="background:#f59e0b"></div> Light (klx)</div>
        </div>
      </div>

      <!-- HEALTH RING -->
      <div class="health-card">
        <div class="health-title">Plant Health</div>
        <div class="health-sub">Composite score — all 6 sensors</div>
        <div class="health-ring-wrap">
          <svg width="150" height="150" viewBox="0 0 150 150">
            <circle cx="75" cy="75" r="54" fill="none" stroke="#f1f5f9" stroke-width="11"/>
            <circle cx="75" cy="75" r="54" id="healthRing"
              fill="none" stroke="#22c55e" stroke-width="11"
              stroke-linecap="round" stroke-dasharray="339.3" stroke-dashoffset="339.3"
              transform="rotate(-90 75 75)"
              style="transition:stroke-dashoffset .8s ease,stroke .4s"/>
            <text x="75" y="69" text-anchor="middle"
              font-family="'DM Mono',monospace" font-size="26" font-weight="700"
              id="healthPct" fill="#22c55e">--%</text>
            <text x="75" y="88" text-anchor="middle"
              font-family="'DM Sans',sans-serif" font-size="11" font-weight="500"
              fill="#94a3b8">HEALTH</text>
          </svg>
        </div>
        <div class="health-metrics">
          <div class="metric-row">
            <span class="metric-icon">&#127777;</span>
            <span class="metric-name">Temp</span>
            <div class="metric-bar"><div class="metric-bar-fill" id="mTempBar" style="width:0%;background:#ef4444"></div></div>
            <span class="metric-val" id="mTempVal">--°C</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#128167;</span>
            <span class="metric-name">Humidity</span>
            <div class="metric-bar"><div class="metric-bar-fill" id="mHumBar" style="width:0%;background:#3b82f6"></div></div>
            <span class="metric-val" id="mHumVal">--%</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#127807;</span>
            <span class="metric-name">Soil</span>
            <div class="metric-bar"><div class="metric-bar-fill" id="mSoilBar" style="width:0%;background:#22c55e"></div></div>
            <span class="metric-val" id="mSoilVal">--%</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#9878;</span>
            <span class="metric-name">pH</span>
            <div class="metric-bar"><div class="metric-bar-fill" id="mPhBar" style="width:0%;background:#8b5cf6"></div></div>
            <span class="metric-val" id="mPhVal">--</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#9728;</span>
            <span class="metric-name">Light</span>
            <div class="metric-bar"><div class="metric-bar-fill" id="mLuxBar" style="width:0%;background:#f59e0b"></div></div>
            <span class="metric-val" id="mLuxVal">-- lx</span>
          </div>
          <div class="metric-row">
            <span class="metric-icon">&#128267;</span>
            <span class="metric-name">Battery</span>
            <div class="metric-bar"><div class="metric-bar-fill" id="mBatBar" style="width:0%;background:#22c55e"></div></div>
            <span class="metric-val" id="mBatVal">--%</span>
          </div>
        </div>
      </div>

    </div><!-- end bottom-row -->

    <div class="ip-footer">ESP32-AGRO &nbsp;|&nbsp; PlantOS v3 &nbsp;|&nbsp; http://)rawliteral" + ip + R"rawliteral(</div>

  </div><!-- content -->
</div><!-- main -->

<script>
// ---- COLOR HELPERS ----
function colorFor(type, val) {
  if (type==='temp')   return (val<15||val>35)?'#ef4444':'#22c55e';
  if (type==='hum')    return (val<30||val>80)?'#f59e0b':'#3b82f6';
  if (type==='soil')   return val<30?'#ef4444':val<40?'#f59e0b':'#22c55e';
  if (type==='ph')     return (val<5.0||val>8.0)?'#ef4444':(val<5.5||val>7.5)?'#f59e0b':'#22c55e';
  if (type==='lux')    return val<100?'#ef4444':val<500?'#f59e0b':'#22c55e';
  if (type==='bat')    return val<30?'#ef4444':val<70?'#f59e0b':'#22c55e';
  if (type==='health') return val>=75?'#22c55e':val>=50?'#f59e0b':'#ef4444';
  return '#22c55e';
}
function luxLabel(v){
  if(v<100)   return 'Very dark';
  if(v<500)   return 'Dim';
  if(v<2000)  return 'Indoor';
  if(v<10000) return 'Bright';
  if(v<50000) return 'Sunny';
  return 'Intense';
}
function healthLabel(h){
  if(h>=80) return 'Excellent';
  if(h>=60) return 'Good';
  if(h>=40) return 'Fair';
  return 'Poor';
}
function setColor(el,c){el.style.color=c;}
function flash(el){el.classList.remove('updated');void el.offsetWidth;el.classList.add('updated');}

// ---- CLOCK ----
function updateClock(){
  var n=new Date();
  document.getElementById('clockDisp').textContent=
    String(n.getHours()).padStart(2,'0')+':'+
    String(n.getMinutes()).padStart(2,'0')+':'+
    String(n.getSeconds()).padStart(2,'0');
}
updateClock(); setInterval(updateClock,1000);

// ---- SPARKLINE ----
var MAX=40, tD=[],hD=[],sD=[],pD=[],lD=[];
for(var i=0;i<MAX;i++){tD.push(0);hD.push(0);sD.push(0);pD.push(7);lD.push(0);}

function pts(data,lo,hi){
  var s='';
  for(var i=0;i<data.length;i++){
    var x=(i/(MAX-1))*600;
    var y=200-((data[i]-lo)/(hi-lo))*200;
    y=Math.max(2,Math.min(198,y));
    s+=x.toFixed(1)+','+y.toFixed(1)+' ';
  }
  return s.trim();
}
function pushChart(t,h,s,ph,lux){
  tD.push(t); hD.push(h); sD.push(s); pD.push(ph); lD.push(lux/1000);
  if(tD.length>MAX){tD.shift();hD.shift();sD.shift();pD.shift();lD.shift();}
  document.getElementById('tempLine').setAttribute('points',pts(tD,0,60));
  document.getElementById('humLine').setAttribute('points',pts(hD,0,100));
  document.getElementById('soilLine').setAttribute('points',pts(sD,0,100));
  document.getElementById('phLine').setAttribute('points',pts(pD,0,14));
  document.getElementById('luxLine').setAttribute('points',pts(lD,0,80));
}

// ---- APPLY DATA ----
function applyData(d){
  var tc=colorFor('temp',d.temp),   hc=colorFor('hum',d.humidity),
      sc=colorFor('soil',d.soil),   pc=colorFor('ph',d.ph),
      lc=colorFor('lux',d.lux),     bc=colorFor('bat',d.batPct),
      hc2=colorFor('health',d.health);
  var circ=339.3, off=circ-(d.health/100)*circ;

  // Temperature
  var tv=document.getElementById('tempVal');
  tv.textContent=d.temp; setColor(tv,tc); flash(tv);
  document.getElementById('tempStatus').textContent=d.tempStatus; setColor(document.getElementById('tempStatus'),tc);
  document.getElementById('tempSubStatus').textContent=d.tempStatus; setColor(document.getElementById('tempSubStatus'),tc);
  document.getElementById('tempBar').style.cssText='width:'+Math.min((d.temp/50)*100,100)+'%;background:'+tc;
  document.getElementById('tempDot').style.background=tc;

  // Humidity
  var hv=document.getElementById('humVal');
  hv.textContent=d.humidity; setColor(hv,hc); flash(hv);
  document.getElementById('humDelta').textContent=d.humidity+'%'; setColor(document.getElementById('humDelta'),hc);
  document.getElementById('humBar').style.cssText='width:'+d.humidity+'%;background:'+hc;
  document.getElementById('humDot').style.background=hc;

  // Soil
  var sv=document.getElementById('soilVal');
  sv.textContent=d.soil; setColor(sv,sc); flash(sv);
  document.getElementById('soilStatus').textContent=d.soilStatus; setColor(document.getElementById('soilStatus'),sc);
  document.getElementById('soilSubStatus').textContent=d.soilStatus; setColor(document.getElementById('soilSubStatus'),sc);
  document.getElementById('soilBar').style.cssText='width:'+d.soil+'%;background:'+sc;
  document.getElementById('soilDot').style.background=sc;

  // pH
  var pv=document.getElementById('phVal');
  pv.textContent=parseFloat(d.ph).toFixed(2); setColor(pv,pc); flash(pv);
  document.getElementById('phStatus').textContent=d.phStatus; setColor(document.getElementById('phStatus'),pc);
  document.getElementById('phSubStatus').textContent=d.phStatus; setColor(document.getElementById('phSubStatus'),pc);
  document.getElementById('phBar').style.cssText='width:'+((d.ph/14)*100)+'%;background:'+pc;
  document.getElementById('phDot').style.background=pc;

  // Light
  var lv=document.getElementById('luxVal');
  lv.textContent=d.lux; setColor(lv,lc); flash(lv);
  var ll=luxLabel(d.lux);
  document.getElementById('lightStatus').textContent=d.lightStatus||ll; setColor(document.getElementById('lightStatus'),lc);
  document.getElementById('luxSubStatus').textContent=ll; setColor(document.getElementById('luxSubStatus'),lc);
  document.getElementById('luxBar').style.cssText='width:'+Math.min((d.lux/80000)*100,100)+'%;background:'+lc;
  document.getElementById('luxDot').style.background=lc;

  // Battery
  var bv=document.getElementById('batPctVal');
  bv.textContent=d.batPct; setColor(bv,bc); flash(bv);
  document.getElementById('batStatusLabel').textContent=d.batStatus; setColor(document.getElementById('batStatusLabel'),bc);
  document.getElementById('batVoltVal').textContent=parseFloat(d.batV).toFixed(2)+'V'; setColor(document.getElementById('batVoltVal'),bc);
  document.getElementById('batBar').style.cssText='width:'+d.batPct+'%;background:'+bc;
  document.getElementById('batDot').style.background=bc;

  // Health ring
  var ring=document.getElementById('healthRing');
  ring.setAttribute('stroke-dashoffset',off.toFixed(1));
  ring.setAttribute('stroke',hc2);
  var hp=document.getElementById('healthPct');
  hp.textContent=d.health+'%'; hp.setAttribute('fill',hc2);

  // Metric bars
  document.getElementById('mTempBar').style.cssText='width:'+Math.min((d.temp/50)*100,100)+'%;background:'+tc;
  document.getElementById('mTempVal').textContent=d.temp+'\u00B0C';
  document.getElementById('mHumBar').style.cssText='width:'+d.humidity+'%;background:'+hc;
  document.getElementById('mHumVal').textContent=d.humidity+'%';
  document.getElementById('mSoilBar').style.cssText='width:'+d.soil+'%;background:'+sc;
  document.getElementById('mSoilVal').textContent=d.soil+'%';
  document.getElementById('mPhBar').style.cssText='width:'+((d.ph/14)*100)+'%;background:'+pc;
  document.getElementById('mPhVal').textContent=parseFloat(d.ph).toFixed(2);
  document.getElementById('mLuxBar').style.cssText='width:'+Math.min((d.lux/80000)*100,100)+'%;background:'+lc;
  document.getElementById('mLuxVal').textContent=d.lux+' lx';
  document.getElementById('mBatBar').style.cssText='width:'+d.batPct+'%;background:'+bc;
  document.getElementById('mBatVal').textContent=d.batPct+'%';

  // Alert banner — priority: soil > pH > battery
  var banner=document.getElementById('alertBanner');
  var aIcon=document.getElementById('alertIcon');
  var aMsg=document.getElementById('alertMsg');
  if(d.soil<30){
    banner.style.cssText='padding:12px 18px;border-radius:12px;border:1.5px solid #ef4444;background:#fef2f2;color:#dc2626;font-size:14px;font-weight:600;display:flex;align-items:center;gap:10px';
    aIcon.textContent='\u26A0\uFE0F'; aMsg.textContent='WATER YOUR PLANT NOW!';
  } else if(d.ph<5.0||d.ph>8.5){
    banner.style.cssText='padding:12px 18px;border-radius:12px;border:1.5px solid #f59e0b;background:#fffbeb;color:#b45309;font-size:14px;font-weight:600;display:flex;align-items:center;gap:10px';
    aIcon.textContent='\u26A0\uFE0F'; aMsg.textContent='Soil pH out of range — check fertiliser!';
  } else if(d.batPct<20){
    banner.style.cssText='padding:12px 18px;border-radius:12px;border:1.5px solid #f59e0b;background:#fffbeb;color:#b45309;font-size:14px;font-weight:600;display:flex;align-items:center;gap:10px';
    aIcon.textContent='\u26A0\uFE0F'; aMsg.textContent='Battery low \u2014 charge soon!';
  } else {
    banner.style.cssText='padding:12px 18px;border-radius:12px;border:1.5px solid #22c55e;background:#f0fdf4;color:#16a34a;font-size:14px;font-weight:600;display:flex;align-items:center;gap:10px';
    aIcon.textContent='\u2705'; aMsg.innerHTML='Plant is healthy &amp; happy';
  }

  pushChart(d.temp,d.humidity,d.soil,d.ph,d.lux);

  var n=new Date();
  document.getElementById('lastRead').textContent='Last reading: '+
    String(n.getHours()).padStart(2,'0')+':'+
    String(n.getMinutes()).padStart(2,'0')+':'+
    String(n.getSeconds()).padStart(2,'0');
}

// ---- AJAX FETCH every 2s ----
var fails=0;
function fetchData(){
  fetch('/data')
    .then(function(r){if(!r.ok)throw new Error(r.status);return r.json();})
    .then(function(d){
      fails=0;
      var cs=document.getElementById('connStatus');
      cs.textContent='\u2022 Online'; cs.style.color='#22c55e';
      applyData(d);
    })
    .catch(function(){
      fails++;
      if(fails>=3){
        var cs=document.getElementById('connStatus');
        cs.textContent='\u2022 Reconnecting...'; cs.style.color='#f59e0b';
      }
    });
}
fetchData();
setInterval(fetchData,2000);
</script>
</body>
</html>
)rawliteral";
  server.send(200,"text/html",html);
}

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  Wire.begin();        // SDA=GPIO21, SCL=GPIO22

  dht.begin();

  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("[WARN] BH1750 not found — check wiring!");
  } else {
    Serial.println("[OK]  BH1750 ready");
  }

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(2,0); lcd.print("PlantOS  v3");
  lcd.setCursor(3,1); lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ap_ip, ap_ip, IPAddress(255,255,255,0));
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("=== PlantOS v3 ===");
  Serial.print("SSID     : "); Serial.println(ap_ssid);
  Serial.print("Password : "); Serial.println(ap_password);
  Serial.print("URL      : http://"); Serial.println(ap_ip);

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Hotspot ON!");
  lcd.setCursor(0,1); lcd.print(ap_ip);
  delay(3000);
  lcd.clear();

  server.on("/",     handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("[OK]  Web server started");

  beep(1000,100); delay(100); beep(1500,100);
}

// ================================================================
//  LOOP
// ================================================================
void loop() {
  server.handleClient();

  // LCD page flip
  if (millis() - lcdMillis >= LCD_PAGE_TIME) {
    lcdMillis = millis();
    lcdPage   = (lcdPage + 1) % 4;   // 4 pages now
    showLCDPage(lcdPage);
  }

  // Sensor read
  if (millis() - previousMillis >= READ_INTERVAL) {
    previousMillis = millis();

    // ---- Capacitive Soil (HIGH=dry, LOW=wet) ----
    g_soilRaw     = readAverage(SOIL_PIN);
    g_soilPercent = map(g_soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
    g_soilPercent = constrain(g_soilPercent, 0, 100);

    // ---- pH Sensor ----
    int phRaw     = readAverage(PH_PIN);
    float voltage = phRaw * (VREF / 4095.0);
    g_pH          = constrain(PH_SLOPE * voltage + PH_OFFSET, 0.0, 14.0);

    // ---- BH1750 Light ----
    if (lightMeter.measurementReady()) {
      g_lux = lightMeter.readLightLevel();
    }

    // ---- DHT22 ----
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      g_humidity = h;
      g_temp     = t;
    } else {
      Serial.println("[WARN] DHT22 read failed");
    }

    // ---- Battery ----
    readBattery();

    // ---- Status strings ----
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

    if      (g_lux <  100)  g_lightStatus = "VeryDark";
    else if (g_lux <  500)  g_lightStatus = "Dim";
    else if (g_lux < 2000)  g_lightStatus = "Indoor";
    else if (g_lux < 50000) g_lightStatus = "Bright";
    else                     g_lightStatus = "Intense";

    // ---- Alerts ----
    if (g_soilPercent < 30) {
      digitalWrite(LED_PIN, HIGH);
      dryAlert();
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    if (g_pH < 5.0 || g_pH > 8.5) phAlert();

    if (g_batPercent < 15) batAlert();

    showLCDPage(lcdPage);

    Serial.printf(
      "T:%.1fC | H:%.1f%% | Soil:%d%% | pH:%.2f | Lux:%.0flx | Bat:%.2fV(%d%%)\n",
      g_temp, g_humidity, g_soilPercent, g_pH, g_lux, g_batVoltage, g_batPercent
    );
  }
}
