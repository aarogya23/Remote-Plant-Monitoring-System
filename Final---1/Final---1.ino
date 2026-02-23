#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ================= PIN SETUP =================
#define PH_PIN       A1
#define SOIL_PIN     A0
#define DHT_PIN      7
#define DHT_TYPE     DHT11
#define RELAY_PIN    8   // Pump control

// ================= OBJECTS =================
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);

// ================= SETTINGS =================
const unsigned long READ_INTERVAL = 2000;
const byte SAMPLE_COUNT = 15;

// ===== pH Calibration =====
float ph_offset = 0.0;
float ph_slope  = 3.5;

// ===== Soil Calibration =====
int SOIL_DRY = 850;   // Adjust after testing
int SOIL_WET = 400;   // Adjust after testing

unsigned long previousMillis = 0;

// ==================================================
// Averaging Function
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
  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);   // Relay OFF (Active LOW module)

  lcd.setCursor(0,0);
  lcd.print("Smart Agro Sys");
  delay(2000);
  lcd.clear();

  Serial.println("System Ready");
}

// ==================================================
void loop() {

  if (millis() - previousMillis >= READ_INTERVAL) {
    previousMillis = millis();

    // ===== Read pH =====
    int phRaw = readAverage(PH_PIN);
    float voltage = phRaw * (5.0 / 1023.0);
    float pH = (ph_slope * voltage) + ph_offset;

    // ===== Read Soil =====
    int soilRaw = readAverage(SOIL_PIN);
    int soilPercent = map(soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    // ===== Read DHT11 (Serial Only) =====
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // ===== Status Logic =====
    String phStatus = (pH < 6.0) ? "Acidic" :
                      (pH > 7.5) ? "Alkaline" : "Neutral";

    String soilStatus = (soilPercent < 30) ? "Dry" :
                        (soilPercent < 70) ? "Moist" : "Wet";

    // ==================================================
    // 🚨 AUTOMATIC PUMP CONTROL
    if (soilPercent <= 5) {
      digitalWrite(RELAY_PIN, LOW);   // Pump ON
    } else {
      digitalWrite(RELAY_PIN, HIGH);  // Pump OFF
    }
    // ==================================================

    // ===== SERIAL OUTPUT =====
    Serial.println("------ SENSOR DATA ------");

    Serial.print("pH: ");
    Serial.print(pH,2);
    Serial.print(" (");
    Serial.print(phStatus);
    Serial.println(")");

    Serial.print("Soil: ");
    Serial.print(soilPercent);
    Serial.print("% (");
    Serial.print(soilStatus);
    Serial.println(")");

    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
\
    Serial.println("--------------------------");

    // ===== LCD DISPLAY =====
    lcd.clear();

    if (soilPercent <= 5) {

      lcd.setCursor(0,0);
      lcd.print("Soil: 0% DRY");

      lcd.setCursor(0,1);
      lcd.print("Water The Plant");

    } 
    else {

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
}
