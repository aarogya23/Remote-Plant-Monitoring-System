#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= WIFI HOTSPOT =================
const char* ssid = "AgroSystem_ESP32";
const char* password = "12345678";
String serverName = "http://192.168.4.2:8080/api/sensor";

// ================= SENSOR PINS =================
#define SOIL_PIN 35
#define PH_PIN   34
#define LDR_PIN  33
#define DHT_PIN  4
#define DHT_TYPE DHT11
#define BUZZER_PIN 19

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);

// ================= TIMERS =================
unsigned long previousMillis = 0;
unsigned long lcdMillis = 0;

const long sendInterval = 10000;
const long lcdInterval  = 3000;

byte lcdPage = 0;

// ================= THRESHOLDS =================
int SOIL_MIN = 30;
int TEMP_MAX = 35;
int LIGHT_MIN = 20;

// ================= VALUES =================
int soilPercent = 0;
float ph = 0;
float temperature = 0;
float humidity = 0;
int ldrPercent = 0;

// ================= SETUP =================
void setup() {

  Serial.begin(115200);
  dht.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin();
  lcd.init();
  lcd.backlight();

  lcd.setCursor(2, 0);
  lcd.print("AGRO SYSTEM");
  lcd.setCursor(3, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println("ESP32 Hotspot Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

// ================= LOOP =================
void loop() {

  if (millis() - previousMillis >= sendInterval) {
    previousMillis = millis();

    readSensors();
    checkAlerts();
    sendToServer();
  }

  if (millis() - lcdMillis >= lcdInterval) {
    lcdMillis = millis();
    lcdPage = (lcdPage + 1) % 4;
    showLCD();
  }
}

// ================= READ SENSORS =================
void readSensors() {

  // Soil
  int soilRaw = analogRead(SOIL_PIN);
  soilPercent = map(soilRaw, 4095, 3000, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  // pH
  int phRaw = analogRead(PH_PIN);
  float voltage = phRaw * (3.3 / 4095.0);
  ph = constrain(3.5 * voltage, 0, 14);

  // LDR
  int ldrRaw = analogRead(LDR_PIN);
  ldrPercent = map(ldrRaw, 0, 4095, 0, 100);
  ldrPercent = constrain(ldrPercent, 0, 100);

  // DHT
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT ERROR!");
    return;
  }

  Serial.println("===== SENSOR DATA =====");
  Serial.println("Soil: " + String(soilPercent) + "%");
  Serial.println("pH: " + String(ph,2));
  Serial.println("Temp: " + String(temperature));
  Serial.println("Humidity: " + String(humidity));
  Serial.println("Light: " + String(ldrPercent) + "%");
  Serial.println("=======================");
}

// ================= ALERT SYSTEM =================
void checkAlerts() {

  if (soilPercent < SOIL_MIN ||
      temperature > TEMP_MAX ||
      ldrPercent < LIGHT_MIN) {

    Serial.println("⚠ ALERT TRIGGERED!");

    for(int i=0; i<3; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
    }
  }
}

// ================= LCD DISPLAY =================
void showLCD() {

  lcd.clear();

  if (lcdPage == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Soil:");
    lcd.print(soilPercent);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("pH:");
    lcd.print(ph,2);
  }

  else if (lcdPage == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(temperature,1);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Hum:");
    lcd.print(humidity,1);
    lcd.print("%");
  }

  else if (lcdPage == 2) {
    lcd.setCursor(0, 0);
    lcd.print("Light:");
    lcd.print(ldrPercent);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Monitoring...");
  }

  else if (lcdPage == 3) {
    lcd.setCursor(0, 0);
    lcd.print("Hotspot ON");

    lcd.setCursor(0, 1);
    lcd.print(WiFi.softAPIP());
  }
}

// ================= SEND TO SERVER =================
void sendToServer() {

  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName);
  http.addHeader("Content-Type", "application/json");

  String jsonData = "{";
  jsonData += "\"soil\":" + String(soilPercent) + ",";
  jsonData += "\"ph\":" + String(ph,2) + ",";
  jsonData += "\"temperature\":" + String(temperature,1) + ",";
  jsonData += "\"humidity\":" + String(humidity,1) + ",";
  jsonData += "\"ldr\":" + String(ldrPercent);
  jsonData += "}";

  int httpResponseCode = http.POST(jsonData);

  Serial.print("HTTP Response: ");
  Serial.println(httpResponseCode);

  // Short confirmation beep
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  http.end();
}
