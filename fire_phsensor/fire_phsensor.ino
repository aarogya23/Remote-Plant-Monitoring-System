#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= PIN SETUP =================
const byte PH_PIN = A1;
const byte FIRE_PIN = 3;

// ================= LCD SETUP =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= SETTINGS ==================
const unsigned long READ_INTERVAL = 2000;
const byte SAMPLE_COUNT = 15;

float calibration_offset = 0.00;
float calibration_slope  = 3.5;

unsigned long previousMillis = 0;

// ---------- Read Averaged pH ----------
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

  pinMode(FIRE_PIN, INPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("System Starting");
  delay(2000);
  lcd.clear();

  Serial.println("System Ready");
}

// ==================================================
void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= READ_INTERVAL) {
    previousMillis = currentMillis;

    // ===== FIRE CHECK FIRST =====
    int fireStatus = digitalRead(FIRE_PIN);

    if (fireStatus == LOW) {   // LOW = Fire detected (most modules)
      Serial.println("🔥 FIRE DETECTED !!!");

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("!!! FIRE !!!");
      lcd.setCursor(0,1);
      lcd.print("Check Area NOW");

      delay(2000);
      return;   // Skip pH reading during fire
    }

    // ===== pH SENSOR =====
    int analogValue = readPHAverage();

    if (analogValue <= 5) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Sensor Error!");
      return;
    }

    float voltage = analogValue * (5.0 / 1023.0);
    float pHValue = (calibration_slope * voltage) + calibration_offset;

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
    Serial.print("pH: ");
    Serial.println(pHValue,2);
    Serial.print("Condition: ");
    Serial.println(condition);
    Serial.println("----------------------");

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
