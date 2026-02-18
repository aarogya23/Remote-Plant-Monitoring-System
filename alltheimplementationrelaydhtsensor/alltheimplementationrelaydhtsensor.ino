#include <DHT11.h>

// ═══════════════════════════════════════════════════════════════
//                      PIN DEFINITIONS
// ═══════════════════════════════════════════════════════════════
#define SOIL_PIN   34   // GPIO 34 - Soil Moisture Analog Input
#define DHT_PIN    26   // GPIO 26 - DHT11 Data Pin
#define RELAY_PIN  27   // GPIO 27 - Relay Control (Pump)

// ═══════════════════════════════════════════════════════════════
//                   MOISTURE THRESHOLDS
// ═══════════════════════════════════════════════════════════════
#define DRY_THRESHOLD   30   // Below this → pump ON
#define WET_THRESHOLD   70   // Above this → pump OFF

// ═══════════════════════════════════════════════════════════════
//                   PUMP SAFETY SETTINGS
// ═══════════════════════════════════════════════════════════════
#define PUMP_ON_TIME     5000    // Pump runs for 5 seconds
#define PUMP_COOLDOWN   30000    // Wait 30 seconds before watering again

// ═══════════════════════════════════════════════════════════════
//                   GLOBAL VARIABLES
// ═══════════════════════════════════════════════════════════════
bool pumpRunning        = false;
unsigned long pumpStartTime   = 0;
unsigned long lastWaterTime   = 0;

DHT11 dht11(DHT_PIN);

// ═══════════════════════════════════════════════════════════════
//                        SETUP
// ═══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);   // Pump OFF at start (active LOW)
  delay(1000);

  Serial.println("╔═════════════════════════════╗");
  Serial.println("║   AUTO PLANT WATERING v1.0   ║");
  Serial.println("║  Soil + DHT11 + Water Pump   ║");
  Serial.println("╚═════════════════════════════╝");
  Serial.println("  Soil Pin   : GPIO 34");
  Serial.println("  DHT11 Pin  : GPIO 26");
  Serial.println("  Relay Pin  : GPIO 27");
  Serial.println("  System Ready!");
  delay(1000);
}

// ═══════════════════════════════════════════════════════════════
//                     PUMP CONTROL
// ═══════════════════════════════════════════════════════════════
void pumpON() {
  digitalWrite(RELAY_PIN, LOW);    // Active LOW = pump ON
  pumpRunning  = true;
  pumpStartTime = millis();
  Serial.println("  💧 PUMP ON  - Watering started!");
}

void pumpOFF() {
  digitalWrite(RELAY_PIN, HIGH);   // HIGH = pump OFF
  pumpRunning   = false;
  lastWaterTime = millis();
  Serial.println("  ⛔ PUMP OFF - Watering stopped!");
}

// ═══════════════════════════════════════════════════════════════
//                     AUTO WATERING LOGIC
// ═══════════════════════════════════════════════════════════════
void autoWater(int moisture, int temperature) {

  unsigned long currentTime = millis();

  // ── Safety: Stop pump after PUMP_ON_TIME ─────────────────
  if (pumpRunning && (currentTime - pumpStartTime >= PUMP_ON_TIME)) {
    pumpOFF();
    return;
  }

  // ── Safety: Stop pump if soil is now wet ─────────────────
  if (pumpRunning && moisture >= WET_THRESHOLD) {
    Serial.println("  ✔ Soil is wet enough - stopping pump early!");
    pumpOFF();
    return;
  }

  // ── Dont water if pump is still running ──────────────────
  if (pumpRunning) return;

  // ── Dont water if cooldown not finished ──────────────────
  if (currentTime - lastWaterTime < PUMP_COOLDOWN) {
    unsigned long remaining = (PUMP_COOLDOWN - (currentTime - lastWaterTime)) / 1000;
    Serial.print("  ⏳ Cooldown: ");
    Serial.print(remaining);
    Serial.println(" seconds remaining");
    return;
  }

  // ── Dont water if temperature is too cold ────────────────
  if (temperature < 10) {
    Serial.println("  ⚠ Too cold to water - skipping!");
    return;
  }

  // ── Water if soil is DRY ──────────────────────────────────
  if (moisture < DRY_THRESHOLD) {
    Serial.println("  🌱 Soil is DRY - Starting pump!");
    pumpON();
  }
}

// ═══════════════════════════════════════════════════════════════
//                       MAIN LOOP
// ═══════════════════════════════════════════════════════════════
void loop() {

  // ══ SOIL MOISTURE ════════════════════════════════════════
  int rawValue       = analogRead(SOIL_PIN);
  int moisturePercent = map(rawValue, 4095, 0, 0, 100);
  moisturePercent    = constrain(moisturePercent, 0, 100);

  // ══ DHT11 ════════════════════════════════════════════════
  int temperature = 0;
  int humidity    = 0;
  int result = dht11.readTemperatureHumidity(temperature, humidity);

  // ══ PRINT SOIL DATA ══════════════════════════════════════
  Serial.println("====================");
  Serial.print("Raw Value   : "); Serial.println(rawValue);
  Serial.print("Moisture    : "); Serial.print(moisturePercent); Serial.println(" %");

  // Visual moisture bar
  Serial.print("Level       : [");
  for (int i = 0; i < 10; i++) {
    if (i < moisturePercent / 10) Serial.print("#");
    else                          Serial.print("-");
  }
  Serial.print("] ");
  Serial.print(moisturePercent);
  Serial.println("%");

  // Soil status
  if (moisturePercent < DRY_THRESHOLD)      Serial.println("Soil Status : DRY  - Need Water!");
  else if (moisturePercent < WET_THRESHOLD) Serial.println("Soil Status : MOIST - Good!");
  else                                      Serial.println("Soil Status : WET");

  // ══ PRINT DHT11 DATA ═════════════════════════════════════
  if (result == 0) {
    Serial.print("Temperature : "); Serial.print(temperature); Serial.println(" C");
    Serial.print("Humidity    : "); Serial.print(humidity);    Serial.println(" %");

    // ══ PLANT HEALTH ═════════════════════════════════════
    Serial.print("Plant Health: ");
    if      (moisturePercent < DRY_THRESHOLD && temperature > 35) Serial.println("HOT & DRY   - Water immediately!");
    else if (moisturePercent > WET_THRESHOLD && humidity > 80)    Serial.println("WET & HUMID - Root rot risk!");
    else if (moisturePercent < DRY_THRESHOLD)                     Serial.println("LOW MOISTURE - Water the plant!");
    else if (temperature > 35)                                    Serial.println("TOO HOT     - Move to cooler area!");
    else if (temperature < 10)                                    Serial.println("TOO COLD    - Move to warmer area!");
    else                                                          Serial.println("All Good!");

    // ══ PUMP STATUS ══════════════════════════════════════
    Serial.print("Pump Status : ");
    if (pumpRunning) Serial.println("RUNNING 💧");
    else             Serial.println("IDLE ⛔");

    // ══ AUTO WATERING ════════════════════════════════════
    autoWater(moisturePercent, temperature);

  } else {
    Serial.print("DHT11 Error : ");
    Serial.println(DHT11::getErrorString(result));
    Serial.println("  ⚠ Skipping auto water - DHT11 failed!");
  }

  Serial.println("====================");
  delay(2000);
}5
