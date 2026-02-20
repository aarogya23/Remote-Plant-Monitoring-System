#include <DHT.h>

// Pin definitions
#define DHT_PIN 7
#define DHTTYPE DHT11
#define SOIL_PIN A0
#define RELAY_PIN 8

DHT dht(DHT_PIN, DHTTYPE);

float temperature;
float humidity;
int soilValue;
int moisturePercent;

// Soil moisture threshold for dry soil
int dryThreshold = 30; // percentage
int wetThreshold = 50; // add hysteresis

void setup() {
  Serial.begin(9600);

  dht.begin();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Pump OFF initially

  Serial.println("Smart Plant Monitoring System Started");
  Serial.println("------------------------------------");
}

void loop() {

  // --- Read DHT11 ---
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  // --- Read Soil Sensor ---
  soilValue = analogRead(SOIL_PIN);
  moisturePercent = map(soilValue, 1023, 0, 0, 100); // Convert to %

  // --- Print Temperature & Humidity ---
  if (!isnan(temperature)) {
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
  } else {
    Serial.println("Temperature read failed");
  }

  if (!isnan(humidity)) {
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
  } else {
    Serial.println("Humidity read failed");
  }

  // --- Print Soil Moisture ---
  Serial.print("Soil Moisture Value: "); Serial.println(soilValue);
  Serial.print("Soil Moisture Percentage: "); Serial.print(moisturePercent); Serial.println(" %");

  // --- Determine Soil Status with Hysteresis ---
  static bool pumpState = false; // remember previous pump state

  if (moisturePercent < dryThreshold && !pumpState) {
    Serial.println("Status: Dry Soil → Pump ON");
    digitalWrite(RELAY_PIN, LOW); // Turn pump ON
    pumpState = true;
  } 
  else if (moisturePercent > wetThreshold && pumpState) {
    Serial.println("Status: Wet/Moist Soil → Pump OFF");
    digitalWrite(RELAY_PIN, HIGH); // Turn pump OFF
    pumpState = false;
  } 
  else {
    Serial.println("Status: Soil stable → Pump unchanged");
  }

  Serial.println("------------------------------------");

  delay(2000); // Safe DHT11 reading interval
}
