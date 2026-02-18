#include <DHT11.h>

// ─── Pin Definitions ─────────────────────────────────────────
#define SOIL_PIN  34   // GPIO 34 - Soil Moisture Analog Input
#define DHT_PIN   26   // GPIO 26 - DHT11 Data Pin

DHT11 dht11(DHT_PIN);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Plant Monitor Ready!");
}

void loop() {

  // ══ SOIL MOISTURE ════════════════════════════════════════
  int rawValue = analogRead(SOIL_PIN);  // 0 - 4095 (12-bit ADC)
  int moisturePercent = map(rawValue, 4095, 0, 0, 100);  // wet=low, dry=high

  Serial.println("-------------------");
  Serial.print("Raw Value   : "); Serial.println(rawValue);
  Serial.print("Moisture    : "); Serial.print(moisturePercent); Serial.println(" %");

  if (moisturePercent < 30)      Serial.println("Soil Status : DRY - Need Water!");
  else if (moisturePercent < 70) Serial.println("Soil Status : MOIST - Good!");
  else                           Serial.println("Soil Status : WET");

  // ══ DHT11 TEMPERATURE & HUMIDITY ════════════════════════
  int temperature = 0;
  int humidity    = 0;
  int result = dht11.readTemperatureHumidity(temperature, humidity);

  if (result == 0) {
    Serial.print("Temperature : "); Serial.print(temperature); Serial.println(" C");
    Serial.print("Humidity    : "); Serial.print(humidity);    Serial.println(" %");

    // ══ PLANT HEALTH ═════════════════════════════════════
    Serial.print("Plant Health: ");
    if      (moisturePercent < 30 && temperature > 35) Serial.println("HOT & DRY - Water immediately!");
    else if (moisturePercent > 70 && humidity > 80)    Serial.println("WET & HUMID - Root rot risk!");
    else if (moisturePercent < 30)                     Serial.println("LOW MOISTURE - Water the plant!");
    else if (temperature > 35)                         Serial.println("TOO HOT - Move to cooler area!");
    else if (temperature < 10)                         Serial.println("TOO COLD - Move to warmer area!");
    else                                               Serial.println("All Good!");

  } else {
    Serial.print("DHT11 Error : ");
    Serial.println(DHT11::getErrorString(result));
  }

  Serial.println("-------------------");
  delay(2000);
}
