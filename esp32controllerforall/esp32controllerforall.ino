#define BLYNK_TEMPLATE_ID "TMPL6UlAE3dqc"
#define BLYNK_TEMPLATE_NAME "Allinoneproject"
#define BLYNK_AUTH_TOKEN "aJrtp3X2oWfDRSvk80qfQKgeN8ZOSUEM"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "Virinchi College 2";
char pass[] = "virinchi@2025";

// Virtual Pins
#define VPIN_PH          V0
#define VPIN_SOIL        V1
#define VPIN_TEMP        V2
#define VPIN_HUMIDITY    V3
#define VPIN_PUMP_STATUS V4

float pH, temperature, humidity;
int soilPercent;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();

  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();

    // Parse: pH,soil,temp,humidity
    int i1 = data.indexOf(',');
    int i2 = data.indexOf(',', i1+1);
    int i3 = data.indexOf(',', i2+1);

    if (i1 > 0 && i2 > 0 && i3 > 0) {
      pH          = data.substring(0, i1).toFloat();
      soilPercent = data.substring(i1+1, i2).toInt();
      temperature = data.substring(i2+1, i3).toFloat();
      humidity    = data.substring(i3+1).toFloat();

      // Send to Blynk
      Blynk.virtualWrite(VPIN_PH,       pH);
      Blynk.virtualWrite(VPIN_SOIL,     soilPercent);
      Blynk.virtualWrite(VPIN_TEMP,     temperature);
      Blynk.virtualWrite(VPIN_HUMIDITY, humidity);

      // Pump status
      String pumpStatus = (soilPercent <= 5) ? "ON" : "OFF";
      Blynk.virtualWrite(VPIN_PUMP_STATUS, pumpStatus);
    }
  }
}

// Optional: Control pump from Blynk app
BLYNK_WRITE(V5) {
  int val = param.asInt();
  // Send command back to Mega if needed
  Serial2.println(val == 1 ? "PUMP_ON" : "PUMP_OFF");
}
