#include <DHT.h>

// ================= PIN SETUP =================
#define SOIL_PIN   A0
#define RELAY_PIN  7
#define DHTPIN     2
#define DHTTYPE    DHT11

DHT dht(DHTPIN, DHTTYPE);

// ================ CALIBRATION =================
const int DRY_VALUE = 680;   // Your dry reading
const int WET_VALUE = 425;   // Your wet reading

// Motor control thresholds
const int MOTOR_ON_VALUE  = 650;  // Dry condition
const int MOTOR_OFF_VALUE = 450;  // Wet condition

// ================ SETTINGS ====================
const unsigned long READ_INTERVAL = 2000;
const byte SAMPLE_COUNT = 10;

// ==============================================
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

// ==========================================================
void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF initially
  dht.begin();

  Serial.println("Smart Irrigation + DHT11 Started...");
}

// ==========================================================
void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= READ_INTERVAL) {
    previousMillis = currentMillis;

    // ----- Read Soil -----
    int soilValue = readSoilAverage();
    int moisturePercent = map(soilValue, DRY_VALUE, WET_VALUE, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);

    // ----- Read DHT11 -----
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("DHT11 Error!");
      return;
    }

    // ----- Print All Data -----
    Serial.print("Soil Raw: ");
    Serial.print(soilValue);
    Serial.print(" | Moisture: ");
    Serial.print(moisturePercent);
    Serial.print("% | Temp: ");
    Serial.print(temperature);
    Serial.print("°C | Humidity: ");
    Serial.print(humidity);
    Serial.println("%");

    // ----- Motor Control -----
    if (soilValue >= MOTOR_ON_VALUE && !motorState) {
      digitalWrite(RELAY_PIN, LOW);   // ON (Active LOW)
      motorState = true;
      Serial.println("Motor ON - Soil Dry");
    }
    else if (soilValue <= MOTOR_OFF_VALUE && motorState) {
      digitalWrite(RELAY_PIN, HIGH);  // OFF
      motorState = false;
      Serial.println("Motor OFF - Soil Wet");
    }

    Serial.println("----------------------------------");
  }
}
