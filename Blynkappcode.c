// yogox36131@avulas.com
// Qwerty@123
#define BLYNK_TEMPLATE_ID "TMPL29j8U0wFt"
#define BLYNK_TEMPLATE_NAME "auto resource fault detection"
#define BLYNK_AUTH_TOKEN "B_rL-ughE_AybJaShlm4Cmg52dgKOOUV"

#include <DHT.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#define BLYNK_PRINT Serial

// WiFi credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "IDEAZONE4G";
char pass[] = "8074744190";

// Pin Definitions
#define DHTPIN D4       // DHT11 Sensor
#define DHTTYPE DHT11
#define MQ2_PIN D7      // MQ2 Smoke Sensor
#define FLAME_PIN D5    // Flame Sensor
#define BUZZER D6       // Buzzer
#define RELAY_PIN D3    // Relay Module (active LOW)
#define LDR_PIN A0      // LDR Sensor (light sensor)
#define RAIN_SENSOR_PIN D2  // Rain Sensor (Digital Output)

// Sensor instances
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

// Variables for LDR rapid fluctuation detection
int lastLdrValue = 0;
unsigned long fluctuationStartTime = 0;
bool rapidChangeDetected = false;
bool fluctuationStatusSent = false;

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(MQ2_PIN, INPUT);
  pinMode(FLAME_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);

  digitalWrite(BUZZER, LOW);
  digitalWrite(RELAY_PIN, HIGH); // Relay initially ON (active LOW)

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Connect to Blynk
  Blynk.begin(auth, ssid, pass);
}

BLYNK_WRITE(V7) { // Relay Control using Virtual Pin V7
  int relayState = param.asInt();
  if (relayState == 1) {
    digitalWrite(RELAY_PIN, LOW);  // Turn relay ON (active LOW)
  } else {
    digitalWrite(RELAY_PIN, HIGH); // Turn relay OFF
  }
}

void loop() {
  Blynk.run();

  // Read Sensors
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  int smokeStatus = digitalRead(MQ2_PIN);
  int fireStatus = digitalRead(FLAME_PIN);
  int ldrValue = analogRead(LDR_PIN);
  int rainStatus = digitalRead(RAIN_SENSOR_PIN);

  // Generate random voltage (220V to 240V) and current (20A to 30A)
  float voltage = random(220, 241);
  float current = random(20, 31);

  // Serial Debug
  Serial.print("Temp: "); Serial.print(temp); Serial.print("°C | ");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.print("% | ");
  Serial.print("Smoke: "); Serial.print(smokeStatus ? "No Smoke" : "Smoke Detected!"); Serial.print(" | ");
  Serial.print("Fire: "); Serial.print(fireStatus ? "No Fire" : "Fire Detected!"); Serial.print(" | ");
  Serial.print("LDR: "); Serial.print(ldrValue); Serial.print(" | ");
  Serial.print("Rain: "); Serial.println(rainStatus ? "No Rain" : "Rain Detected!");

  // Send to Blynk
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V8, humidity);
  Blynk.virtualWrite(V1, smokeStatus ? "No Smoke" : "Smoke Detected!");
  Blynk.virtualWrite(V2, fireStatus ? "No Fire" : "Fire Detected!");
  Blynk.virtualWrite(V5, voltage);
  Blynk.virtualWrite(V6, current);

  // Rain Detection
  if (rainStatus == LOW) {  // LOW means rain detected
    Blynk.virtualWrite(V3, "leak detected");
    Blynk.logEvent("water_pressure");
    digitalWrite(RELAY_PIN, LOW);
  } else {
    Blynk.virtualWrite(V3, "No leakage");
    digitalWrite(RELAY_PIN, HIGH);
  }

  // Rapid LDR fluctuation detection
  if (abs(ldrValue - lastLdrValue) > 30) {  // Change threshold if needed
    if (!rapidChangeDetected) {
      fluctuationStartTime = millis();
      rapidChangeDetected = true;
      fluctuationStatusSent = false;
    } else {
      if (millis() - fluctuationStartTime > 3000 && !fluctuationStatusSent) { // 3 seconds continuous
        Blynk.virtualWrite(V4, "Fluctuation Detected");
        Blynk.logEvent("rapid_light_fluctuation");
        Serial.println("Rapid LDR fluctuation detected for 3 seconds!");
        fluctuationStatusSent = true;
      }
    }
  } else {
    rapidChangeDetected = false;
    if (!fluctuationStatusSent) {
      Blynk.virtualWrite(V4, "No Fluctuation Detected");
      fluctuationStatusSent = true;
    }
  }

  lastLdrValue = ldrValue;  // Update last value

  // Buzzer alerts
  if (smokeStatus == 0) {
    Blynk.logEvent("smoke_detection");
    digitalWrite(BUZZER, HIGH);
    delay(2000);
    digitalWrite(BUZZER, LOW);
  }
  if (fireStatus == 0) {
    Blynk.logEvent("fire_detection");
    digitalWrite(BUZZER, HIGH);
    delay(2000);
    digitalWrite(BUZZER, LOW);
  }
}
