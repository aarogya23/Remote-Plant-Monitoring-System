// ================================================================
//  Krishi Drishti v4 — LDR sensor on GPIO2 for light detection
//  DO=LOW = Light, DO=HIGH = Dark
// ================================================================

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ================================================================
//  WIFI & SERVER
// ================================================================
const char* WIFI_SSID     = "Redmi 13";
const char* WIFI_PASSWORD = "12345678";
const char* SERVER_URL    = "http://10.173.109.184:8080/api/data";

// ================================================================
//  PINS
// ================================================================
#define SOIL_PIN    34
#define PH_PIN      35
#define DHT_PIN      4
#define DHT_TYPE    DHT22
#define BUZZER_PIN  19
#define LED_PIN     18
#define BAT_PIN     33
#define LCD2_SDA    25
#define LCD2_SCL    26
#define IR_PIN      2

TwoWire I2C_BUS2 = TwoWire(1);

DHT               dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd1(0x27, 16, 2);

// ================================================================
//  CALIBRATION
// ================================================================
int   SOIL_DRY      = 3200;
int   SOIL_WET      = 1500;
float PH_SLOPE      = -5.70;
float PH_OFFSET     =  14.16;
const float R1          = 100000.0;
const float R2          =  47000.0;
const float VREF        =       3.3;
const float BAT_FULL_V  =       8.4;
const float BAT_EMPTY_V =       6.0;

// ================================================================
//  SENSOR STATE
// ================================================================
float  g_temp = 0, g_humidity = 0, g_pH = 0;
int    g_soilPercent = 0, g_soilRaw = 0;
float  g_batVoltage  = 0;
int    g_batPercent  = 0;
bool   g_irObstacle  = false;
String g_soilStatus  = "---";
String g_tempStatus  = "---";
String g_lightStatus = "---";
String g_lightAdvice = "---";
String g_phStatus    = "---";
String g_batStatus   = "---";

unsigned long previousMillis = 0;
unsigned long lcd2FlipMillis = 0;
byte          lcd2Page       = 0;

const unsigned long READ_INTERVAL  = 2000;
const unsigned long LCD2_FLIP_TIME = 4000;
const byte          SAMPLE_COUNT   = 30;

// ================================================================
//  BUZZER COOLDOWN
// ================================================================
unsigned long lastDryAlertTime  = 0;
unsigned long lastPhAlertTime   = 0;
unsigned long lastBatAlertTime  = 0;
unsigned long lastTempAlertTime = 0;
const unsigned long DRY_ALERT_INTERVAL  = 30000;
const unsigned long PH_ALERT_INTERVAL   = 60000;
const unsigned long BAT_ALERT_INTERVAL  = 120000;
const unsigned long TEMP_ALERT_INTERVAL = 60000;

// ================================================================
//  HELPERS
// ================================================================
int readAverage(byte pin) {
  long total = 0;
  for (byte i = 0; i < SAMPLE_COUNT; i++) { total += analogRead(pin); delay(5); }
  return total / SAMPLE_COUNT;
}
void beep(int f, int ms) { tone(BUZZER_PIN, f); delay(ms); noTone(BUZZER_PIN); }
void dryAlert()  { for (int i = 0; i < 3; i++) { beep(2000, 200); delay(100); } }
void batAlert()  { beep(800, 500); delay(200); beep(800, 500); }
void phAlert()   { beep(1200, 300); delay(150); beep(1200, 300); }
void tempAlert() { beep(1800, 300); delay(100); beep(1800, 300); delay(100); beep(1800, 300); }

// ================================================================
//  LCD2 LOW-LEVEL WRITE
// ================================================================
#define LCD_ADDR      0x27
#define LCD_BACKLIGHT 0x08
#define LCD_EN        0x04
#define LCD_RW        0x02
#define LCD_RS        0x01

void lcd2_write4bits(uint8_t value) {
  I2C_BUS2.beginTransmission(LCD_ADDR);
  I2C_BUS2.write(value | LCD_BACKLIGHT);
  I2C_BUS2.endTransmission();
  delayMicroseconds(1);
  I2C_BUS2.beginTransmission(LCD_ADDR);
  I2C_BUS2.write((value | LCD_EN) | LCD_BACKLIGHT);
  I2C_BUS2.endTransmission();
  delayMicroseconds(1);
  I2C_BUS2.beginTransmission(LCD_ADDR);
  I2C_BUS2.write((value & ~LCD_EN) | LCD_BACKLIGHT);
  I2C_BUS2.endTransmission();
  delayMicroseconds(50);
}
void lcd2_send(uint8_t value, uint8_t mode) {
  uint8_t hi = (value & 0xF0) | mode;
  uint8_t lo = ((value << 4) & 0xF0) | mode;
  lcd2_write4bits(hi);
  lcd2_write4bits(lo);
}
void lcd2_cmd(uint8_t cmd)  { lcd2_send(cmd, 0);     delayMicroseconds(2000); }
void lcd2_char(uint8_t ch)  { lcd2_send(ch, LCD_RS); delayMicroseconds(50);   }
void lcd2_init() {
  delay(50);
  lcd2_write4bits(0x30); delay(5);
  lcd2_write4bits(0x30); delayMicroseconds(150);
  lcd2_write4bits(0x30); delayMicroseconds(150);
  lcd2_write4bits(0x20); delayMicroseconds(150);
  lcd2_cmd(0x28); lcd2_cmd(0x08); lcd2_cmd(0x01);
  delay(2);
  lcd2_cmd(0x06); lcd2_cmd(0x0C);
}
void lcd2_clear()                             { lcd2_cmd(0x01); delay(2); }
void lcd2_setCursor(uint8_t col, uint8_t row) {
  uint8_t offsets[] = {0x00, 0x40};
  lcd2_cmd(0x80 | (col + offsets[row]));
}
void lcd2_print(const char* str) { while (*str) lcd2_char(*str++); }
void lcd2_print(String s)        { lcd2_print(s.c_str()); }
void lcd2_print(int n)           { lcd2_print(String(n)); }
void lcd2_print(float f, int d)  { lcd2_print(String(f, d)); }

// ================================================================
//  BATTERY
// ================================================================
void readBattery() {
  int   raw    = readAverage(BAT_PIN);
  float vPin   = raw * (VREF / 4095.0);
  g_batVoltage = vPin * ((R1 + R2) / R2);
  g_batPercent = (int)((g_batVoltage - BAT_EMPTY_V) / (BAT_FULL_V - BAT_EMPTY_V) * 100.0);
  g_batPercent = constrain(g_batPercent, 0, 100);
  if      (g_batPercent > 70) g_batStatus = "Good";
  else if (g_batPercent > 30) g_batStatus = "Low";
  else                         g_batStatus = "Critical";
}

// ================================================================
//  LIGHT STATUS FROM LDR
//  DO=LOW  = light present  → "Light"
//  DO=HIGH = dark / no light → "Dark"
//  If readings are inverted, swap LOW/HIGH below
// ================================================================
void readIRSensor() {
  int val = digitalRead(IR_PIN);
  Serial.printf("[IR] raw: %d\n", val);

  if (val == LOW) {
    g_irObstacle  = true;
    g_lightStatus = "Near";
    g_lightAdvice = "Object Detected";
  } else {
    g_irObstacle  = false;
    g_lightStatus = "Clear";
    g_lightAdvice = "All Clear";
  }
}

// ================================================================
//  HEALTH SCORE
// ================================================================
int calcHealth() {
  int s = 100;
  if      (g_soilPercent < 20) s -= 30;
  else if (g_soilPercent < 40) s -= 15;
  else if (g_soilPercent > 85) s -= 10;
  if      (g_pH < 5.0 || g_pH > 8.0)   s -= 25;
  else if (g_pH < 5.5 || g_pH > 7.5)   s -= 10;
  if      (g_temp < 5  || g_temp > 40)  s -= 35;
  else if (g_temp < 10 || g_temp > 37)  s -= 25;
  else if (g_temp < 15 || g_temp > 33)  s -= 15;
  else if (g_temp >= 22 && g_temp < 26) s +=  5;
  if      (g_humidity < 20 || g_humidity > 90) s -= 20;
  else if (g_humidity < 30 || g_humidity > 80) s -=  8;
  if (g_irObstacle)     s -= 10;
  if (g_batPercent < 10) s -= 15;
  return constrain(s, 0, 100);
}

// ================================================================
//  LCD1 — pH & Light
//  Line 1: "pH:6.82 Neutral "
//  Line 2: "Light           "
// ================================================================
void updateLCD1() {
  lcd1.clear();

  lcd1.setCursor(0, 0);
  lcd1.print("pH:");
  lcd1.print(g_pH, 2);
  lcd1.print(" ");
  lcd1.print(g_phStatus);

  lcd1.setCursor(0, 1);
  String line2 = g_lightStatus;
  while (line2.length() < 16) line2 += " ";
  lcd1.print(line2.substring(0, 16));
}

// ================================================================
//  LCD2 — Soil/Env  ↔  Light Advice
// ================================================================
void updateLCD2() {
  lcd2_clear();

  if (lcd2Page == 0) {
    lcd2_setCursor(0, 0);
    lcd2_print("Soil:");
    lcd2_print(g_soilPercent);
    lcd2_print("% ");
    lcd2_print(g_soilStatus);

    lcd2_setCursor(0, 1);
    lcd2_print("T:");
    lcd2_print(g_temp, 1);
    lcd2_print("C H:");
    lcd2_print((int)g_humidity);
    lcd2_print("%   ");

  } else {
    String line1 = g_lightAdvice;
    while (line1.length() < 16) line1 += " ";
    lcd2_setCursor(0, 0);
    lcd2_print(line1.substring(0, 16));

    String line2 = "IR: ";
    line2 += g_lightStatus;
    while (line2.length() < 16) line2 += " ";
    lcd2_setCursor(0, 1);
    lcd2_print(line2.substring(0, 16));
  }
}

// ================================================================
//  JSON
// ================================================================
String buildSensorJson() {
  String j = "{";
  j += "\"temp\":"           + String(g_temp, 1)         + ",";
  j += "\"humidity\":"       + String(g_humidity, 1)     + ",";
  j += "\"soil\":"           + String(g_soilPercent)     + ",";
  j += "\"ph\":"             + String(g_pH, 2)           + ",";
  j += "\"lux\":"            + String(g_irObstacle ? 1 : 0) + ",";
  j += "\"batV\":"           + String(g_batVoltage, 2)   + ",";
  j += "\"batPct\":"         + String(g_batPercent)      + ",";
  j += "\"health\":"         + String(calcHealth())      + ",";
  j += "\"soilStatus\":\""   + g_soilStatus              + "\",";
  j += "\"phStatus\":\""     + g_phStatus                + "\",";
  j += "\"tempStatus\":\""   + g_tempStatus              + "\",";
  j += "\"lightStatus\":\""  + g_lightStatus             + "\",";
  j += "\"lightAdvice\":\""  + g_lightAdvice             + "\",";
  j += "\"batStatus\":\""    + g_batStatus               + "\",";
  j += "\"sensorType\":\"IR\"";
  j += "}";
  return j;
}

void sendViaWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(buildSensorJson());
    if (code > 0) Serial.printf("[HTTP] POST code: %d\n", code);
    else          Serial.printf("[HTTP] POST failed: %s\n", http.errorToString(code).c_str());
    http.end();
  } else {
    Serial.println("[WIFI] Disconnected. Reconnecting...");
    WiFi.reconnect();
  }
}

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(21, 22);
  I2C_BUS2.begin(LCD2_SDA, LCD2_SCL);
  delay(200);

  dht.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);
  pinMode(IR_PIN,    INPUT);

  lcd1.init();
  lcd1.backlight();
  lcd1.setCursor(1, 0); lcd1.print("Krishi Drishti");
  lcd1.setCursor(2, 1); lcd1.print("LCD1 Ready");

  lcd2_init();
  lcd2_setCursor(1, 0); lcd2_print("Krishi Drishti");
  lcd2_setCursor(2, 1); lcd2_print("LCD2 Ready");

  delay(2000);
  lcd1.clear();
  lcd2_clear();

  lcd1.setCursor(0, 0); lcd1.print("Connecting WiFi ");
  lcd1.setCursor(0, 1); lcd1.print(WIFI_SSID);
  lcd2_setCursor(0, 0); lcd2_print("Connecting WiFi ");
  lcd2_setCursor(0, 1); lcd2_print("Please wait...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  Serial.println("\n=== Krishi Drishti v4 (LDR light sensor) ===");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  lcd1.clear();
  lcd1.setCursor(0, 0); lcd1.print("WiFi Connected!");
  lcd1.setCursor(0, 1); lcd1.print(WiFi.localIP().toString());
  lcd2_clear();
  lcd2_setCursor(0, 0); lcd2_print("WiFi Connected!");
  lcd2_setCursor(0, 1); lcd2_print(WiFi.localIP().toString());

  delay(3000);
  lcd1.clear();
  lcd2_clear();

  beep(1000, 100); delay(100); beep(1500, 100);
  Serial.println("[OK]  Device ready.");
}

// ================================================================
//  LOOP
// ================================================================
void loop() {

  if (millis() - lcd2FlipMillis >= LCD2_FLIP_TIME) {
    lcd2FlipMillis = millis();
    lcd2Page = (lcd2Page + 1) % 2;
    updateLCD2();
  }

  if (millis() - previousMillis >= READ_INTERVAL) {
    previousMillis = millis();

    // Soil
    g_soilRaw     = readAverage(SOIL_PIN);
    g_soilPercent = map(g_soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
    g_soilPercent = constrain(g_soilPercent, 0, 100);

    // pH
    int   phRaw     = readAverage(PH_PIN);
    float phVoltage = phRaw * (VREF / 4095.0);
    g_pH            = PH_SLOPE * phVoltage + PH_OFFSET;
    g_pH            = constrain(g_pH, 0.0, 14.0);

    // DHT22
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) { g_humidity = h; g_temp = t; }
    else Serial.println("[WARN] DHT22 read failed");

    // LDR
    readIRSensor();

    // Battery
    readBattery();

    // Status strings
    if      (g_soilPercent < 20) g_soilStatus = "VeryDry";
    else if (g_soilPercent < 40) g_soilStatus = "Dry";
    else if (g_soilPercent < 70) g_soilStatus = "Moist";
    else                          g_soilStatus = "Wet";

    if      (g_pH < 5.5)  g_phStatus = "Acidic";
    else if (g_pH <= 7.5) g_phStatus = "Neutral";
    else                   g_phStatus = "Alkaline";

    if      (g_temp < 5)  g_tempStatus = "Freezing";
    else if (g_temp < 10) g_tempStatus = "Very Cold";
    else if (g_temp < 15) g_tempStatus = "Cold";
    else if (g_temp < 20) g_tempStatus = "Cool";
    else if (g_temp < 26) g_tempStatus = "Ideal";
    else if (g_temp < 30) g_tempStatus = "Warm";
    else if (g_temp < 35) g_tempStatus = "Hot";
    else                   g_tempStatus = "Very Hot";

    // Alerts
    if (g_soilPercent < 30) {
      digitalWrite(LED_PIN, HIGH);
      if (millis() - lastDryAlertTime >= DRY_ALERT_INTERVAL) { dryAlert(); lastDryAlertTime = millis(); }
    } else { digitalWrite(LED_PIN, LOW); }

    if (g_pH < 5.0 || g_pH > 8.5)
      if (millis() - lastPhAlertTime >= PH_ALERT_INTERVAL) { phAlert(); lastPhAlertTime = millis(); }

    if (g_batPercent < 15)
      if (millis() - lastBatAlertTime >= BAT_ALERT_INTERVAL) { batAlert(); lastBatAlertTime = millis(); }

    if (g_temp > 35 || g_temp < 10)
      if (millis() - lastTempAlertTime >= TEMP_ALERT_INTERVAL) { tempAlert(); lastTempAlertTime = millis(); }

    // Update LCDs
    updateLCD1();
    updateLCD2();

    // Serial output
    Serial.printf(
      "Soil:%d%% [%s] | pH:%.2f [%s] | T:%.1fC [%s] H:%.1f%% | IR:[%s] | Bat:%d%% [%s]\n",
      g_soilPercent, g_soilStatus.c_str(),
      g_pH, g_phStatus.c_str(),
      g_temp, g_tempStatus.c_str(), g_humidity,
      g_lightStatus.c_str(),
      g_batPercent, g_batStatus.c_str()
    );

    sendViaWiFi();
  }
}
