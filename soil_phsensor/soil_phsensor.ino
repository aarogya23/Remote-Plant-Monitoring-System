#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= PIN SETUP =================
#define PH_PIN     A1
#define SOIL_PIN   A0

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= SETTINGS =================
const unsigned long READ_INTERVAL = 2000;
const byte SAMPLE_COUNT = 15;

// ===== pH Calibration (Adjust after testing) =====
float ph_offset = 0.0;
float ph_slope  = 3.5;

// ===== Soil Calibration (Adjust after testing) =====
int SOIL_DRY = 850;   // Value in air
int SOIL_WET = 400;   // Value in water

unsigned long previousMillis = 0;

// ==================================================
// Generic Averaging Function (Reusable)
int readAverage(byte pin) {
  long total = 0;
  for (byte i = 0; i < SAMPLE_COUNT; i++) {
    total += analogRead(pin);
    delay(5);
  }
  return total / SAMPLE_COUNT;
}
// ==================================================

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("System Starting");
  delay(2000);
  lcd.clear();

  Serial.println("pH + Soil Monitoring System Ready");
}

// ==================================================
void loop() {

  if (millis() - previousMillis >= READ_INTERVAL) {
    previousMillis = millis();

    // ===== Read Sensors =====
    int phRaw   = readAverage(PH_PIN);
    int soilRaw = readAverage(SOIL_PIN);

    // ===== pH Calculation =====
    float voltage = phRaw * (5.0 / 1023.0);
    float pH = (ph_slope * voltage) + ph_offset;

    // ===== Soil Moisture Percentage =====
    int soilPercent = map(soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    // ===== Soil Status =====
    String soilStatus;
    if (soilPercent < 30) soilStatus = "Dry";
    else if (soilPercent < 70) soilStatus = "Moist";
    else soilStatus = "Wet";

    // ===== pH Status =====
    String phStatus;
    if (pH < 6.0) phStatus = "Acidic";
    else if (pH > 7.5) phStatus = "Alkaline";
    else phStatus = "Neutral";

    // ===== SERIAL OUTPUT =====
    Serial.println("----- Sensor Data -----");

    Serial.print("pH: ");
    Serial.print(pH, 2);
    Serial.print(" (");
    Serial.print(phStatus);
    Serial.println(")");

    Serial.print("Soil: ");
    Serial.print(soilPercent);
    Serial.print("% (");
    Serial.print(soilStatus);
    Serial.println(")");

    Serial.println("------------------------");

    // ===== LCD DISPLAY =====
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("pH:");
    lcd.print(pH,1);
    lcd.print(" ");
    lcd.print(phStatus);

    lcd.setCursor(0,1);
    lcd.print("Soil:");
    lcd.print(soilPercent);
    lcd.print("% ");
    lcd.print(soilStatus);
  }
}
