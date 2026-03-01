#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= WIFI HOTSPOT =================
const char* ssid = "AgroSystem_ESP32";
const char* password = "12345678";

// Change this to your laptop IP running Spring Boot
String serverName = "http://192.168.4.2:8080/api/sensor";

// ================= SENSOR PINS =================
#define SOIL_PIN 35
#define PH_PIN   34
#define LDR_PIN  33
#define DHT_PIN  4
#define DHT_TYPE DHT11
#define BUZZER_PIN 19

// ================= OBJECTS =================
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

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

// ================= HTML PAGE =================
String getHTML() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Agro Dashboard</title>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;background:#f4f4f4;}";
  html += ".card{background:white;padding:20px;margin:15px;border-radius:10px;box-shadow:0 0 10px gray;font-size:20px;}";
  html += "h1{color:green;}";
  html += "</style></head><body>";
  html += "<h1>🌱 AGRO SYSTEM LIVE</h1>";
  html += "<div class='card'>Soil Moisture: " + String(soilPercent) + "%</div>";
  html += "<div class='card'>pH Value: " + String(ph,2) + "</div>";
  html += "<div class='card'>Temperature: " + String(temperature,1) + " °C</div>";
  html += "<div class='card'>Humidity: " + String(humidity,1) + " %</div>";
  html += "<div class='card'>Light Level: " + String(ldrPercent) + "%</div>";
  html += "<script>setTimeout(()=>{location.reload();},5000);</script>";
  html += "</body></html>";
  return html;
}

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

  Serial.println("Hotspot Started");
  Serial.println(WiFi.softAPIP());

  // Web Server
  server.on("/", []() {
    server.send(200, "text/html", getHTML());
  });

  server.begin();
}

// ================= LOOP =================
void loop() {

  server.handleClient();

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

  int soilRaw = analogRead(SOIL_PIN);
  soilPercent = map(soilRaw, 4095, 3000, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  int phRaw = analogRead(PH_PIN);
  float voltage = phRaw * (3.3 / 4095.0);
  ph = constrain(3.5 * voltage, 0, 14);

  int ldrRaw = analogRead(LDR_PIN);
  ldrPercent = map(ldrRaw, 0, 4095, 0, 100);
  ldrPercent = constrain(ldrPercent, 0, 100);

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

// ================= ALERT =================
void checkAlerts() {

  if (soilPercent < SOIL_MIN ||
      temperature > TEMP_MAX ||
      ldrPercent < LIGHT_MIN) {

    for(int i=0;i<3;i++){
      digitalWrite(BUZZER_PIN,HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN,LOW);
      delay(200);
    }
  }
}

// ================= LCD =================
void showLCD() {

  lcd.clear();

  if (lcdPage == 0) {
    lcd.print("Soil:");
    lcd.print(soilPercent);
    lcd.print("%");
  }
  else if (lcdPage == 1) {
    lcd.print("Temp:");
    lcd.print(temperature);
    lcd.print("C");
  }
  else if (lcdPage == 2) {
    lcd.print("Hum:");
    lcd.print(humidity);
    lcd.print("%");
  }
  else if (lcdPage == 3) {
    lcd.print("IP:");
    lcd.print(WiFi.softAPIP());
  }
}

// ================= SEND TO SPRING =================
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

  int response = http.POST(jsonData);
  Serial.println(response);

  digitalWrite(BUZZER_PIN,HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN,LOW);

  http.end();
}
