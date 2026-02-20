// ESP32 Soil Moisture Sensor Code

const int soilPin = 34;   // GPIO34 (ADC pin)
int soilValue = 0;

void setup() {
  Serial.begin(115200);   // ESP32 uses 115200 baud rate
  delay(1000);
  Serial.println("Soil Moisture Monitoring with ESP32");
}

void loop() {

  soilValue = analogRead(soilPin);   // Read analog value (0–4095)

  Serial.print("Soil Moisture Value: ");
  Serial.println(soilValue);

  // Moisture condition
  if (soilValue > 3000) {
    Serial.println("Status: Dry Soil");
  }
  else if (soilValue > 1500) {
    Serial.println("Status: Normal Moisture");
  }
  else {
    Serial.println("Status: Wet Soil");
  }

  Serial.println("-------------------------");

  delay(1000);
}
