#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

void setup() {
  Serial.begin(9600);
  
  Wire.begin(); // Nano uses A4=SDA, A5=SCL automatically
  
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 Ready!");
  } else {
    Serial.println("BH1750 not found! Check wiring.");
  }
}

void loop() {
  float lux = lightMeter.readLightLevel();
  
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  
  delay(500);
}
