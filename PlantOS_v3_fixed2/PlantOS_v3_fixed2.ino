// ================================================================
//  PlantOS v3 — ESP32 Hotspot Dashboard  [FIXED v2]
//  Sensors : Capacitive Soil | BH1750 Light | DHT22 | pH | 2×18650
//  Output  : LCD 16×2 I2C | Buzzer | LED | Web Dashboard (AJAX)
//
//  FIXES IN THIS VERSION:
//  1. PH_OFFSET corrected to 14.16 (water now reads ~7.0)
//  2. Buzzer cooldown timers — no more non-stop ringing
//     - Dry alert  : max once per 30 seconds
//     - pH alert   : max once per 60 seconds
//     - Bat alert  : max once per 2 minutes
//  3. pH voltage printed in Serial for easy future recalibration
//
//  HOW TO FINE-TUNE pH IF STILL OFF:
//    Open Serial Monitor @ 115200 baud
//    Dip probe in plain water, note "rawV" value
//    PH_OFFSET = 7.0 + (5.70 × rawV)
//    Plug that number in below and re-upload
//
//  PIN ASSIGNMENTS:
//  SOIL_PIN → GPIO34  (ADC1_CH6, input-only)
//  PH_PIN   → GPIO35  (ADC1_CH7, input-only)
//  BAT_PIN  → GPIO33  (ADC1_CH5)
//  DHT_PIN  → GPIO4
//  BUZZER   → GPIO19
//  LED      → GPIO18
//  BH1750   → SDA=GPIO21, SCL=GPIO22  (I2C 0x23)
//  LCD      → SDA=GPIO21, SCL=GPIO22  (I2C 0x27)
// ================================================================

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "BluetoothSerial.h"

// Check if Bluetooth configurations are enabled in the compiler
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// ================================================================
//  BLUETOOTH CONFIG (EDIT THIS)
// ================================================================

// This is the name that will appear when you scan for Bluetooth devices on your PC
const char* BLUETOOTH_NAME = "PlantOS-ESP32";

// ================================================================
//  PIN DEFINITIONS
// ================================================================
#define SOIL_PIN     34
#define PH_PIN       35
#define DHT_PIN       4
#define DHT_TYPE  DHT22
#define BUZZER_PIN   19
#define LED_PIN      18
#define BAT_PIN      33

// ================================================================
//  OBJECTS
// ================================================================
DHT               dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
BluetoothSerial SerialBT;

// ================================================================
//  CALIBRATION & CONSTANTS
// ================================================================

// ---- Capacitive soil (HIGH=dry, LOW=wet) ----
int SOIL_DRY = 3200;
int SOIL_WET = 1500;

// ---- pH sensor ----
// PH_OFFSET FIXED: was 15.90 → gave 8.74 in water
//                  now 14.16 → gives ~7.0 in water  ✅
// To recalibrate:
//   1. Open Serial Monitor @ 115200
//   2. Dip probe in plain water
//   3. Note rawV value printed
//   4. PH_OFFSET = 7.0 + (5.70 × rawV)
float PH_SLOPE  = -5.70;
float PH_OFFSET =  14.16;   // ✅ FIXED (was 15.90 → caused 8.74 reading)

// ---- 2S 18650 battery (8.4V full, 6.0V empty) ----
// Voltage divider: R1=100kΩ, R2=47kΩ
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
const bool HAS_LIGHT_SENSOR = false; // BH1750 removed for now

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
//  BUZZER COOLDOWN TIMERS
// ================================================================
unsigned long lastDryAlertTime = 0;
unsigned long lastPhAlertTime  = 0;
unsigned long lastBatAlertTime = 0;

const unsigned long DRY_ALERT_INTERVAL = 30000;   // 30 seconds
const unsigned long PH_ALERT_INTERVAL  = 60000;   // 60 seconds
const unsigned long BAT_ALERT_INTERVAL = 120000;  // 2 minutes

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

void beep(int freq, int ms) {
  tone(BUZZER_PIN, freq);
  delay(ms);
  noTone(BUZZER_PIN);
}

void dryAlert() {
  for (int i = 0; i < 3; i++) {
    beep(2000, 200);
    delay(100);
  }
}

void batAlert() {
  beep(800, 500);
  delay(200);
  beep(800, 500);
}

void phAlert() {
  beep(1200, 300);
  delay(150);
  beep(1200, 300);
}

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
//  LCD PAGES
// ================================================================
void showLCDPage(byte page) {
  lcd.clear();
  switch (page) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Soil:"); lcd.print(g_soilPercent); lcd.print("% ");
      lcd.print(g_soilStatus.substring(0, 6));
      lcd.setCursor(0, 1);
      lcd.print("T:"); lcd.print(g_temp, 1);
      lcd.print("C H:"); lcd.print((int)g_humidity); lcd.print("%");
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("pH:"); lcd.print(g_pH, 2);
      lcd.print(" "); lcd.print(g_phStatus.substring(0, 7));
      lcd.setCursor(0, 1);
      lcd.print("Light:"); lcd.print((int)g_lux); lcd.print("lx");
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Bat:"); lcd.print(g_batVoltage, 1);
      lcd.print("V "); lcd.print(g_batPercent); lcd.print("%");
      lcd.setCursor(0, 1);
      lcd.print(g_batStatus); lcd.print(" ");
      if (g_soilPercent < 30) lcd.print("WATER!");
      else                     lcd.print("OK :)");
      break;
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("Bluetooth SPP   ");
      lcd.setCursor(0, 1);
      lcd.print("PlantOS-ESP32   ");
      break;
  }
}

// ================================================================
//  HEALTH SCORE  (0–100)
// ================================================================
int calcHealth() {
  int score = 100;

  if      (g_soilPercent < 20) score -= 30;
  else if (g_soilPercent < 40) score -= 15;
  else if (g_soilPercent > 85) score -= 10;

  if      (g_pH < 5.0 || g_pH > 8.0) score -= 25;
  else if (g_pH < 5.5 || g_pH > 7.5) score -= 10;

  if      (g_temp < 10 || g_temp > 40) score -= 25;
  else if (g_temp < 15 || g_temp > 35) score -= 10;

  if      (g_humidity < 20 || g_humidity > 90) score -= 20;
  else if (g_humidity < 30 || g_humidity > 80) score -=  8;

  if (HAS_LIGHT_SENSOR) {
    if      (g_lux <  100)  score -= 20;
    else if (g_lux <  500)  score -= 10;
    else if (g_lux > 80000) score -= 10;
  }

  if (g_batPercent < 10) score -= 15;

  return constrain(score, 0, 100);
}

String buildSensorJson() {
  int health = calcHealth();
  String j = "{";
  j += "\"temp\":"          + String(g_temp, 1)       + ",";
  j += "\"humidity\":"      + String(g_humidity, 1)   + ",";
  j += "\"soil\":"          + String(g_soilPercent)   + ",";
  j += "\"ph\":"            + String(g_pH, 2)         + ",";
  j += "\"lux\":"           + String((int)g_lux)      + ",";
  j += "\"batV\":"          + String(g_batVoltage, 2) + ",";
  j += "\"batPct\":"        + String(g_batPercent)    + ",";
  j += "\"health\":"        + String(health)          + ",";
  j += "\"soilStatus\":\""  + g_soilStatus            + "\",";
  j += "\"phStatus\":\""    + g_phStatus              + "\",";
  j += "\"tempStatus\":\""  + g_tempStatus            + "\",";
  j += "\"lightStatus\":\"" + g_lightStatus           + "\",";
  j += "\"batStatus\":\""   + g_batStatus             + "\"";
  j += "}";
  return j;
}

void sendViaBluetooth() {
  String payload = buildSensorJson();
  
  // Send via Bluetooth
  SerialBT.println(payload);
}

// HTML and JSON Web Endpoints removed

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  Wire.begin();

  dht.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(2, 0); lcd.print("PlantOS  v3");
  lcd.setCursor(3, 1); lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  SerialBT.begin(BLUETOOTH_NAME);
  Serial.print("Bluetooth started! Pair with '");
  Serial.print(BLUETOOTH_NAME);
  Serial.println("'");

  Serial.println("=== PlantOS v3 ===");
  Serial.println("Soil  → GPIO34 | pH → GPIO35 | Bat → GPIO33");
  Serial.println("Mode    : Bluetooth Serial (SPP)");
  Serial.println("--- pH CALIBRATION TIP ---");
  Serial.println("Dip probe in water, note rawV in Serial");
  Serial.println("PH_OFFSET = 7.0 + (5.70 x rawV)");
  Serial.println("--------------------------");

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Bluetooth Ready!");
  lcd.setCursor(0, 1); lcd.print("Pair to PC      ");
  delay(3000);
  lcd.clear();

  Serial.println("[OK]  Device ready (No local web server)");

  beep(1000, 100); delay(100); beep(1500, 100);
}

// ================================================================
//  LOOP
// ================================================================
void loop() {
  // Web server client handling removed

  // LCD page flip
  if (millis() - lcdMillis >= LCD_PAGE_TIME) {
    lcdMillis = millis();
    lcdPage   = (lcdPage + 1) % 4;
    showLCDPage(lcdPage);
  }

  // Sensor read every 2 seconds
  if (millis() - previousMillis >= READ_INTERVAL) {
    previousMillis = millis();

    // ---- Capacitive Soil → GPIO34 ----
    g_soilRaw     = readAverage(SOIL_PIN);
    g_soilPercent = map(g_soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
    g_soilPercent = constrain(g_soilPercent, 0, 100);

    // ---- pH Sensor → GPIO35 ----
    int   phRaw     = readAverage(PH_PIN);
    float phVoltage = phRaw * (VREF / 4095.0);
    g_pH            = PH_SLOPE * phVoltage + PH_OFFSET;
    g_pH            = constrain(g_pH, 0.0, 14.0);

    // ---- Light sensor (BH1750 removed for now) ----
    g_lux = 0;

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

    g_lightStatus = "N/A";

    // ================================================================
    //  ALERTS WITH COOLDOWN — no buzzer spam
    // ================================================================

    // Dry soil — LED on + beep max once per 30 sec
    if (g_soilPercent < 30) {
      digitalWrite(LED_PIN, HIGH);
      if (millis() - lastDryAlertTime >= DRY_ALERT_INTERVAL) {
        dryAlert();
        lastDryAlertTime = millis();
      }
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    // pH out of range — beep max once per 60 sec
    if (g_pH < 5.0 || g_pH > 8.5) {
      if (millis() - lastPhAlertTime >= PH_ALERT_INTERVAL) {
        phAlert();
        lastPhAlertTime = millis();
      }
    }

    // Battery critical — beep max once per 2 min
    if (g_batPercent < 15) {
      if (millis() - lastBatAlertTime >= BAT_ALERT_INTERVAL) {
        batAlert();
        lastBatAlertTime = millis();
      }
    }

    showLCDPage(lcdPage);

    // Serial debug — use rawV to recalibrate pH offset
    Serial.printf(
      "Soil:%d%% raw:%d | pH:%.2f rawV:%.3fV | T:%.1fC H:%.1f%% | Lux:%.0f | Bat:%.2fV %d%%\n",
      g_soilPercent, g_soilRaw,
      g_pH, phVoltage,
      g_temp, g_humidity,
      g_lux,
      g_batVoltage, g_batPercent
    );

    // Send reading via Bluetooth
    sendViaBluetooth();
  }
}
