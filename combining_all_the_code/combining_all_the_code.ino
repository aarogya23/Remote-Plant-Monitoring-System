#include <DHT.h>

// ================= PIN SETUP =================
#define SOIL_PIN   A0
#define PH_PIN     A1
#define RELAY_PIN  7
#define DHTPIN     2
#define DHTTYPE    DHT11

DHT dht(DHTPIN, DHTTYPE);

// ================ CALIBRATION =================
const int DRY_VALUE = 680;
const int WET_VALUE = 425;

const int MOTOR_ON_VALUE  = 650;  // Soil dry
const int MOTOR_OFF_VALUE = 450;  // Soil wet

float ph_slope  = 3.5;   // Adjust after calibration
float ph_offset = 0.0;

// ================ SETTINGS ====================
const unsigned long READ_INTERVAL = 3000;
const byte SAMPLE_COUNT = 10;

bool motorState = false;
unsigned long previousMillis = 0;

// ---------- Read Soil Average ----------
int readSoilAverage() {
  long total = 0;
  for (byte i = 0; i < SAMPLE_COUNT; i++) {
    total += analogRead(SOIL_PIN);
    delay(5);
  }
  return total / SAMPLE_COUNT;
}

// ---------- Read pH Average ----------
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

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF (Active LOW)

  dht.begin();

  Serial.println("SMART AGRICULTURE SYSTEM STARTED");
  delay(3000);  // Stabilize sensors
}

// ==================================================
void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= READ_INTERVAL) {
    previousMillis = currentMillis;

    // ===== SOIL =====
    int soilValue = readSoilAverage();
    int moisturePercent = map(soilValue, DRY_VALUE, WET_VALUE, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);

    // ===== DHT11 =====
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // ===== pH SENSOR =====
    int phAnalog = readPHAverage();
    float phVoltage = phAnalog * (5.0 / 1023.0);
    float pHValue = (ph_slope * phVoltage) + ph_offset;

    // ===== DISPLAY DATA =====
    Serial.println("=================================");
    Serial.print("Soil Raw: ");
    Serial.print(soilValue);
    Serial.print(" | Moisture: ");
    Serial.print(moisturePercent);
    Serial.println("%");

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C | Humidity: ");
    Serial.print(humidity);
    Serial.println("%");

    Serial.print("pH Analog: ");
    Serial.print(phAnalog);
    Serial.print(" | Voltage: ");
    Serial.print(phVoltage, 3);
    Serial.print(" V | pH: ");
    Serial.println(pHValue, 2);

    // ===== MOTOR CONTROL LOGIC =====
    // Motor ON if soil dry AND pH in good range
    if (soilValue >= MOTOR_ON_VALUE && 
        pHValue >= 5.5 && pHValue <= 7.5 && 
        !motorState) {

      digitalWrite(RELAY_PIN, LOW);  // ON
      motorState = true;
      Serial.println("Motor ON - Dry Soil & pH OK");
    }

    // Motor OFF if soil wet OR pH abnormal
    else if ((soilValue <= MOTOR_OFF_VALUE || 
             pHValue < 5.5 || pHValue > 7.5) 
             && motorState) {

      digitalWrite(RELAY_PIN, HIGH);  // OFF
      motorState = false;
      Serial.println("Motor OFF - Wet Soil or pH Issue");
    }

    Serial.println("=================================\n");
  }
}
