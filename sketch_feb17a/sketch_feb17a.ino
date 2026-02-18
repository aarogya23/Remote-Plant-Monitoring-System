#include <DHT11.h>

DHT11 dht11(26);  // D4 = GPIO 26

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("DHT11 Ready!");
}

void loop() {
  int temperature = 0;
  int humidity = 0;

  int result = dht11.readTemperatureHumidity(temperature, humidity);

  if (result == 0) {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  } else {
    Serial.print("Error ");
    Serial.print(result);
    Serial.print(": ");
    Serial.println(DHT11::getErrorString(result));
  }

  Serial.println("-------------------");
  delay(2000);
}
