#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= PIN SETUP =================
const byte PH_PIN = A1;

// ================= LCD SETUP =================
LiquidCrystal_I2C lcd(0x27, 16, 2);   // Change to 0x3F if needed

// ================= SETTINGS ==================
const unsigned long READ_INTERVAL = 2000;
const byte SAMPLE_COUNT = 15;

// Calibration (adjust after testing)
float calibration_offset = 0.00;
float calibration_slope  = 3.5;

unsigned long previousMillis = 0;

// ---------- Read Averaged pH Value ----------
int readPHAverage() {
  long total = 0;

  for (byte i = 0; i < SAMPLE_COUNT; i++) {
    total += analogRead(PH_PIN);
    delay(10);
  }

  return total / SAMPLE_COUNT;
}

// ==================================================
void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("pH System Start");
  delay(2000);
  lcd.clear();

  Serial.println("pH Sensor System Initializing...");
  delay(3000);   // Sensor stabilize
  Serial.println("System Ready.");
}

// ==================================================
void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= READ_INTERVAL) {
    previousMillis = currentMillis;

    int analogValue = readPHAverage();

    // Check sensor error
    if (analogValue <= 5) {
      Serial.println("ERROR: No signal from pH sensor!");

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Sensor Error!");
      lcd.setCursor(0,1);
      lcd.print("Check Probe");
      return;
    }

    // Convert to voltage
    float voltage = analogValue * (5.0 / 1023.0);

    // Convert to pH
    float pHValue = (calibration_slope * voltage) + calibration_offset;

    // Determine condition
    String condition;
    String waterStatus;

    if (pHValue < 6.0) {
      condition = "Acidic";
      waterStatus = "NOT GOOD";
    }
    else if (pHValue > 7.5) {
      condition = "Alkaline";
      waterStatus = "NOT GOOD";
    }
    else {
      condition = "Neutral";
      waterStatus = "GOOD";
    }

    // ===== SERIAL OUTPUT =====
    Serial.print("Analog: ");
    Serial.print(analogValue);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 3);
    Serial.print("V | pH: ");
    Serial.println(pHValue, 2);

    Serial.print("Condition: ");
    Serial.println(condition);
    Serial.println("--------------------------------");

    // ===== LCD DISPLAY =====
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("pH:");
    lcd.print(pHValue,1);
    lcd.print(" ");
    lcd.print(condition);

    lcd.setCursor(0,1);
    lcd.print("Water ");
    lcd.print(waterStatus);
  }
}
