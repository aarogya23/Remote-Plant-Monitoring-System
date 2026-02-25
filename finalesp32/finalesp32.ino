#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= PIN SETUP =================
#define PH_PIN        34
#define SOIL_PIN      35
#define DHT_PIN       4
#define DHT_TYPE      DHT11
#define BUZZER_PIN    19
#define LED_PIN       2
// SDA = D21, SCL = D22 (ESP32 default)

// ================= OBJECTS =================
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // change 0x27 to 0x3F if LCD is blank

// ================= SETTINGS =================
const unsigned long READ_INTERVAL = 2000;
const unsigned long LCD_PAGE_TIME = 3000;
const byte SAMPLE_COUNT = 30;

// pH calibration
float ph_offset = 0.0;
float ph_slope  = 3.5;

// ✅ Calibrated soil values
int SOIL_DRY = 4095;
int SOIL_WET = 3000;

unsigned long previousMillis = 0;
unsigned long lcdMillis      = 0;
byte lcdPage = 0;

// Global sensor values
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
  // 3 urgent beeps when soil is dry
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
    // ── Page 3: Soil RAW & Alert ──
    lcd.setCursor(0, 0);
    lcd.print("SoilRAW:");
    lcd.print(g_soilRaw);
    lcd.print("    ");

    lcd.setCursor(0, 1);
    if (g_soilPercent < 30) {
      lcd.print("!! WATER PLANT !!");
    } else {
      lcd.print("Plant is OK :)  ");
    }
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  Wire.begin();  // SDA=D21, SCL=D22

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

  Serial.println("=============================");
  Serial.println("  ESP32 Smart Agro System");
  Serial.println("=============================");

  // Startup beep
  beep(1000, 100);
  delay(100);
  beep(1500, 100);
}

// ================= LOOP =================
void loop() {

  // ── LCD page auto switch ─────────────────────────
  if (millis() - lcdMillis >= LCD_PAGE_TIME) {
    lcdMillis = millis();
    lcdPage = (lcdPage + 1) % 3;
    showLCDPage(lcdPage);
  }

  // ── Main sensor read ─────────────────────────────
  if (millis() - previousMillis >= READ_INTERVAL) {
    previousMillis = millis();

    // ── Read Soil ─────────────────────────────────
    g_soilRaw     = readAverage(SOIL_PIN);
    g_soilPercent = map(g_soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
    g_soilPercent = constrain(g_soilPercent, 0, 100);

    // ── Read pH ───────────────────────────────────
    int phRaw     = readAverage(PH_PIN);
    float voltage = phRaw * (3.3 / 4095.0);
    g_pH          = constrain((ph_slope * voltage) + ph_offset, 0.0, 14.0);

    // ── Read DHT ──────────────────────────────────
    g_humidity = dht.readHumidity();
    g_temp     = dht.readTemperature();

    if (isnan(g_humidity) || isnan(g_temp)) {
      Serial.println("!!! DHT Sensor Error !!!");
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("DHT ERROR!");
      lcd.setCursor(1, 1);
      lcd.print("Check Wiring");
      beep(300, 500);
      return;
    }

    // ── Status Labels ─────────────────────────────
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

    // ── Dry Alert — buzzer + LED ───────────────────
    if (g_soilPercent < 30) {
      digitalWrite(LED_PIN, HIGH);  // LED ON when dry
      dryAlert();                   // buzzer beeps
      Serial.println("!!! SOIL DRY - WATER YOUR PLANT !!!");
    } else {
      digitalWrite(LED_PIN, LOW);   // LED OFF when ok
    }

    // ── pH Alert ──────────────────────────────────
    if (g_pH < 5.5 || g_pH > 8.5) {
      beep(2500, 100);
      delay(50);
      beep(2500, 100);
    }

    // ── Temp Alert ────────────────────────────────
    if (g_temp > 35) {
      beep(1800, 100);
      delay(50);
      beep(1800, 100);
    }

    // ── Serial Output ─────────────────────────────
    Serial.println("\n======= SENSOR DATA =======");
    Serial.print("Soil RAW  : "); Serial.println(g_soilRaw);
    Serial.print("Soil      : "); Serial.print(g_soilPercent); Serial.print("%  --> "); Serial.println(g_soilStatus);
    Serial.print("pH        : "); Serial.print(g_pH, 2);       Serial.print("  --> "); Serial.println(g_phStatus);
    Serial.print("Temp      : "); Serial.print(g_temp, 1);     Serial.println(" C");
    Serial.print("Humidity  : "); Serial.print(g_humidity, 1); Serial.println(" %");
    Serial.println("===========================");

    // Update LCD
    showLCDPage(lcdPage);
  }
}
