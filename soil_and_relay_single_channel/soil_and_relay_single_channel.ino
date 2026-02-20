// ================= PIN SETUP =================
const byte SOIL_PIN  = A0;
const byte RELAY_PIN = 7;

// ================ CALIBRATION =================
// Based on your real sensor readings
const int DRY_VALUE = 680;   // Your dry average
const int WET_VALUE = 425;   // Your wet average

// Motor switching thresholds (practical values)
const int MOTOR_ON_VALUE  = 650;  // Turn ON when soil is dry
const int MOTOR_OFF_VALUE = 450;  // Turn OFF when soil is wet

// ================ SETTINGS ====================
const unsigned long READ_INTERVAL = 2000; // 2 seconds
const byte SAMPLE_COUNT = 10;             // Averaging samples

// ==============================================
bool motorState = false;
unsigned long previousMillis = 0;

// ---------- Function to Read Averaged Soil Value ----------
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

  // Relay OFF initially (Active LOW module)
  digitalWrite(RELAY_PIN, HIGH);
}

// ==========================================================
void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= READ_INTERVAL) {
    previousMillis = currentMillis;

    int soilValue = readSoilAverage();

    // Convert to moisture %
    int moisturePercent = map(soilValue, DRY_VALUE, WET_VALUE, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);

    Serial.print("Soil Raw: ");
    Serial.print(soilValue);
    Serial.print(" | Moisture: ");
    Serial.print(moisturePercent);
    Serial.println("%");

    // --------- MOTOR CONTROL LOGIC ---------

    // Turn ON when soil is dry
    if (soilValue >= MOTOR_ON_VALUE && !motorState) {
      digitalWrite(RELAY_PIN, LOW);   // ON (Active LOW)
      motorState = true;
      Serial.println("Motor ON - Soil Dry");
    }

    // Turn OFF when soil is wet
    else if (soilValue <= MOTOR_OFF_VALUE && motorState) {
      digitalWrite(RELAY_PIN, HIGH);  // OFF
      motorState = false;
      Serial.println("Motor OFF - Soil Wet");
    }
  }
}
