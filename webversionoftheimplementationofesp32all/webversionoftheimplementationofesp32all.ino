#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>

// ================= WiFi =================
char ssid[] = "Virinchi College 2";
char pass[] = "virinchi@2025";

// ================= STATIC IP =================
IPAddress localIP(192, 168, 1, 200);   // ESP32 fixed IP
IPAddress gateway(192, 168, 1, 1);     // your router IP
IPAddress subnet(255, 255, 255, 0);

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

// ================= LCD PAGES =================
void showLCDPage(byte page) {
  lcd.clear();

  if (page == 0) {
    // ── Page 1: Soil & pH ──
    lcd.setCursor(0, 0);
    lcd.print("Soil:");
    lcd.print(g_soilPercent);
    lcd.print("% ");
    lcd.print(g_soilStatus.substring(0, 8));

    lcd.setCursor(0, 1);
    lcd.print("pH:");
    lcd.print(g_pH, 2);
    lcd.print(" ");
    lcd.print(g_phStatus.substring(0, 8));

  } else if (page == 1) {
    // ── Page 2: Temp & Humidity ──
    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(g_temp, 1);
    lcd.print("C ");
    lcd.print(g_tempStatus.substring(0, 6));

    lcd.setCursor(0, 1);
    lcd.print("Humid:");
    lcd.print(g_humidity, 1);
    lcd.print("%");

  } else if (page == 2) {
    // ── Page 3: IP Address & Alert ──
    lcd.setCursor(0, 0);
    lcd.print("IP:");
    lcd.print(WiFi.localIP());

    lcd.setCursor(0, 1);
    if (g_soilPercent < 30) {
      lcd.print("!! WATER PLANT !!");
    } else {
      lcd.print("Plant is OK :)  ");
    }
  }
}

// ================= HTML PAGE =================
void handleRoot() {
  String alertColor = (g_soilPercent < 30) ? "#e74c3c" : "#2ecc71";
  String alertMsg   = (g_soilPercent < 30) ? "!! WATER YOUR PLANT !!" : "Plant is OK :)";

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta http-equiv="refresh" content="3">
  <title>Smart Agro System</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: Arial, sans-serif;
      background: #1a1a2e;
      color: white;
      min-height: 100vh;
      padding: 20px;
    }
    h1 {
      text-align: center;
      color: #00d4aa;
      font-size: 24px;
      margin-bottom: 20px;
      letter-spacing: 2px;
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 15px;
      max-width: 600px;
      margin: 0 auto;
    }
    .card {
      background: #16213e;
      border-radius: 15px;
      padding: 20px;
      text-align: center;
      border: 1px solid #0f3460;
    }
    .card .icon { font-size: 36px; margin-bottom: 8px; }
    .card .label {
      font-size: 13px;
      color: #aaa;
      margin-bottom: 6px;
      text-transform: uppercase;
      letter-spacing: 1px;
    }
    .card .value {
      font-size: 32px;
      font-weight: bold;
      color: #00d4aa;
    }
    .card .unit { font-size: 14px; color: #aaa; }
    .card .status {
      margin-top: 6px;
      font-size: 13px;
      color: #f39c12;
    }
    .alert-box {
      max-width: 600px;
      margin: 15px auto;
      padding: 15px;
      border-radius: 12px;
      text-align: center;
      font-size: 18px;
      font-weight: bold;
      background: )rawliteral" + alertColor + R"rawliteral(;
    }
    .ip-box {
      max-width: 600px;
      margin: 10px auto;
      padding: 8px;
      border-radius: 8px;
      text-align: center;
      font-size: 13px;
      color: #aaa;
      background: #16213e;
      border: 1px solid #0f3460;
    }
    .footer {
      text-align: center;
      margin-top: 20px;
      font-size: 12px;
      color: #555;
    }
  </style>
</head>
<body>

  <h1>🌱 SMART AGRO SYSTEM</h1>

  <div class="alert-box">)rawliteral" + alertMsg + R"rawliteral(</div>

  <div class="grid">

    <div class="card">
      <div class="icon">💧</div>
      <div class="label">Soil Moisture</div>
      <div class="value">)rawliteral" + String(g_soilPercent) + R"rawliteral(<span class="unit">%</span></div>
      <div class="status">)rawliteral" + g_soilStatus + R"rawliteral(</div>
    </div>

    <div class="card">
      <div class="icon">🧪</div>
      <div class="label">pH Level</div>
      <div class="value">)rawliteral" + String(g_pH, 2) + R"rawliteral(</div>
      <div class="status">)rawliteral" + g_phStatus + R"rawliteral(</div>
    </div>

    <div class="card">
      <div class="icon">🌡️</div>
      <div class="label">Temperature</div>
      <div class="value">)rawliteral" + String(g_temp, 1) + R"rawliteral(<span class="unit">°C</span></div>
      <div class="status">)rawliteral" + g_tempStatus + R"rawliteral(</div>
    </div>

    <div class="card">
      <div class="icon">💦</div>
      <div class="label">Humidity</div>
      <div class="value">)rawliteral" + String(g_humidity, 1) + R"rawliteral(<span class="unit">%</span></div>
    </div>

  </div>

  <div class="ip-box">
    📡 ESP32 IP: )rawliteral" + WiFi.localIP().toString() + R"rawliteral( | Auto refresh every 3s
  </div>

  <div class="footer">ESP32 Smart Agro System</div>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Wire.begin();
  dht.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN,    LOW);

  // LCD startup
  lcd.init();
  lcd.backlight();
  lcd.setCursor(2, 0);
  lcd.print("AGRO SYSTEM");
  lcd.setCursor(3, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  // ── Static IP + Connect WiFi ─────────────────
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  Serial.print("Connecting to WiFi...");

  WiFi.config(localIP, gateway, subnet);
  WiFi.begin(ssid, pass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts > 40) {
      Serial.println("\nWiFi Failed!");
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("WiFi Failed!");
      lcd.setCursor(1, 1);
      lcd.print("Check Creds");
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("Open browser: http://");
    Serial.println(WiFi.localIP());

    // Show IP on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(4000);
    lcd.clear();
  }

  // Start web server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web Server Started!");

  beep(1000, 100);
  delay(100);
  beep(1500, 100);
}

// ================= LOOP =================
void loop() {
  server.handleClient();

  // ── LCD page auto switch ───────────────────────
  if (millis() - lcdMillis >= LCD_PAGE_TIME) {
    lcdMillis = millis();
    lcdPage = (lcdPage + 1) % 3;
    showLCDPage(lcdPage);
  }

  // ── Main sensor read ──────────────────────────
  if (millis() - previousMillis >= READ_INTERVAL) {
    previousMillis = millis();

    // Read Soil
    g_soilRaw     = readAverage(SOIL_PIN);
    g_soilPercent = map(g_soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
    g_soilPercent = constrain(g_soilPercent, 0, 100);

    // Read pH
    int phRaw     = readAverage(PH_PIN);
    float voltage = phRaw * (3.3 / 4095.0);
    g_pH          = constrain((ph_slope * voltage) + ph_offset, 0.0, 14.0);

    // Read DHT
    g_humidity = dht.readHumidity();
    g_temp     = dht.readTemperature();

    if (isnan(g_humidity) || isnan(g_temp)) {
      Serial.println("!!! DHT Sensor Error !!!");
      beep(300, 500);
      return;
    }

    // Status Labels
    if      (g_soilPercent < 20) g_soilStatus = "VeryDry";
    else if (g_soilPercent < 40) g_soilStatus = "Dry";
    else if (g_soilPercent < 70) g_soilStatus = "Moist";
    else if (g_soilPercent < 90) g_soilStatus = "Wet";
    else                         g_soilStatus = "Waterlgd";

    if      (g_pH < 5.5)  g_phStatus = "Acidic!!";
    else if (g_pH < 6.0)  g_phStatus = "Acidic";
    else if (g_pH <= 7.5) g_phStatus = "Neutral";
    else if (g_pH <= 8.5) g_phStatus = "Alkaline";
    else                  g_phStatus = "Alkali!!";

    if      (g_temp < 15) g_tempStatus = "Cold";
    else if (g_temp < 35) g_tempStatus = "Normal";
    else                  g_tempStatus = "Hot!!";

    // Dry Alert
    if (g_soilPercent < 30) {
      digitalWrite(LED_PIN, HIGH);
      dryAlert();
      Serial.println("!!! SOIL DRY - WATER YOUR PLANT !!!");
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    // pH Alert
    if (g_pH < 5.5 || g_pH > 8.5) {
      beep(2500, 100); delay(50); beep(2500, 100);
    }

    // Temp Alert
    if (g_temp > 35) {
      beep(1800, 100); delay(50); beep(1800, 100);
    }

    // Serial Output
    Serial.println("\n======= SENSOR DATA =======");
    Serial.print("Open browser : http://"); Serial.println(WiFi.localIP());
    Serial.print("Soil RAW  : "); Serial.println(g_soilRaw);
    Serial.print("Soil      : "); Serial.print(g_soilPercent); Serial.print("%  --> "); Serial.println(g_soilStatus);
    Serial.print("pH        : "); Serial.print(g_pH, 2);       Serial.print("  --> "); Serial.println(g_phStatus);
    Serial.print("Temp      : "); Serial.print(g_temp, 1);     Serial.println(" C");
    Serial.print("Humidity  : "); Serial.print(g_humidity, 1); Serial.println(" %");
    Serial.println("===========================");

    showLCDPage(lcdPage);
  }
}
