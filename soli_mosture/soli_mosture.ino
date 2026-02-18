// Soil Moisture Sensor for ESP32
// Connect AO (Analog Out) to GPIO 34

#define SOIL_PIN 34  // GPIO 34 (Analog pin)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Soil Moisture Sensor Ready!");
}

void loop() {
  int rawValue = analogRead(SOIL_PIN);  // 0 - 4095 (12-bit ADC)

  // Convert to percentage (wet = low value, dry = high value)
  int moisturePercent = map(rawValue, 4095, 0, 0, 100);

  Serial.print("Raw Value: ");
  Serial.println(rawValue);

  Serial.print("Moisture: ");
  Serial.print(moisturePercent);
  Serial.println(" %");

  // Soil status
  if (moisturePercent < 30) {
    Serial.println("Status: DRY - Need Water!");
  } else if (moisturePercent < 70) {
    Serial.println("Status: MOIST - Good!");
  } else {
    Serial.println("Status: WET");
  }

  Serial.println("-------------------");
  delay(2000);
}
