#include <WiFi.h>
#include <PubSubClient.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_SSD1306.h>

// === Configuration WiFi & MQTT ===
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "mqtt.eu.thingsboard.cloud";
const int mqtt_port = 1883;
const char* access_token = "****"; //copy the valid access token

WiFiClient espClient;
PubSubClient client(espClient);

// === Variables PIR ===
unsigned long lastMotionTime = 0;
const int alarmDuration = 3000; // ms
bool pirReady = false;
unsigned long pirActivationDelay = 10000; // ms

// === LCD + Clavier (Entrée) ===
LiquidCrystal_I2C lcd(0x27, 16, 2);
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'}, {'4','5','6','B'},
  {'7','8','9','C'}, {'*','0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {27, 14, 12, 13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// === Pins ===
const int servoPin = 17;
const int pirPinEntree = 5;
const int ledPinEntree = 16;
const int buzzerPinEntree = 23;

#define DHTPIN_CUISINE 15
#define DHTTYPE DHT22
#define MQ2PIN_CUISINE 34
#define LEDPIN_CUISINE 0
#define BUZZERPIN_CUISINE 13
#define SERVOPIN_CUISINE 4
DHT dhtCuisine(DHTPIN_CUISINE, DHTTYPE);
Servo ventilo;
Servo servo;

#define PIR_PIN_CHAMBRE 19
#define DHT_PIN_CHAMBRE 18
#define LED_PIN_CHAMBRE 27
#define MQ2_PIN_CHAMBRE 35
DHT dhtChambre(DHT_PIN_CHAMBRE, DHTTYPE);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// === Etat global ===
String input_password = "";
const String correct_password = "0000";
bool door_open = false;
bool systemReady = false;
unsigned long startupTime;
float lastTempCuisine = NAN;
int lastGasCuisine = -1;
float lastTempChambre = NAN;
float lastHumidityChambre = NAN;
int lastSmokeChambre = -1;

// === Fonctions MQTT ===
void sendTelemetry(const char* key, const char* value) {
  if (!pirReady) return;
  char payload[100];
  snprintf(payload, sizeof(payload), "{\"%s\":\"%s\"}", key, value);
  client.publish("v1/devices/me/telemetry", payload);
}

void sendTelemetryFloat(const char* key, float value) {
  if (!pirReady) return;
  char payload[100];
  snprintf(payload, sizeof(payload), "{\"%s\":%.2f}", key, value);
  client.publish("v1/devices/me/telemetry", payload);
}

void sendTelemetryInt(const char* key, int value) {
  if (!pirReady) return;
  char payload[100];
  snprintf(payload, sizeof(payload), "{\"%s\":%d}", key, value);
  client.publish("v1/devices/me/telemetry", payload);
}

void setup_wifi() {
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password, 6, NULL, true);
  while (WiFi.status() != WL_CONNECTED) delay(500);
}

void reconnect() {
  while (!client.connected()) {
    client.connect("ESP32-Door", access_token, NULL);
    delay(2000);
  }
}

void openDoor() {
  servo.write(90);
  door_open = true;
  lcd.clear(); lcd.print("Porte ouverte!");
  sendTelemetry("door", "open");
  delay(5000);
  servo.write(0);
  door_open = false;
  sendTelemetry("door", "closed");
  lcd.clear(); lcd.print("Entrez code:");
}

void gestionCuisine() {
  float temperature = dhtCuisine.readTemperature();
  int gasLevel = analogRead(MQ2PIN_CUISINE);

  if (!isnan(temperature) && abs(temperature - lastTempCuisine) > 0.5) {
    sendTelemetryFloat("cuisine_temperature", temperature);
    lastTempCuisine = temperature;
  }

  if (abs(gasLevel - lastGasCuisine) > 50) {
    sendTelemetryInt("cuisine_gaz", gasLevel);
    lastGasCuisine = gasLevel;
  }

  if (temperature > 30) {
    ventilo.write(180);
    sendTelemetry("cuisine_ventilo", "on");
  } else {
    ventilo.write(0);
    sendTelemetry("cuisine_ventilo", "off");
  }

  if (gasLevel > 1600) {
    digitalWrite(LEDPIN_CUISINE, HIGH);
    digitalWrite(BUZZERPIN_CUISINE, HIGH);
    sendTelemetry("cuisine_alert", "gas_detected");
  } else {
    digitalWrite(LEDPIN_CUISINE, LOW);
    digitalWrite(BUZZERPIN_CUISINE, LOW);
  }
}

void gestionChambre() {
  float temperature = dhtChambre.readTemperature();
  float humidity = dhtChambre.readHumidity();
  int smokeLevel = analogRead(MQ2_PIN_CHAMBRE);
  bool motion = digitalRead(PIR_PIN_CHAMBRE);

  if (!isnan(temperature) && abs(temperature - lastTempChambre) > 0.5) {
    sendTelemetryFloat("chambre_temperature", temperature);
    lastTempChambre = temperature;
  }

  if (!isnan(humidity) && abs(humidity - lastHumidityChambre) > 2.0) {
    sendTelemetryFloat("chambre_humidity", humidity);
    lastHumidityChambre = humidity;
  }

  if (abs(smokeLevel - lastSmokeChambre) > 100) {
    sendTelemetryInt("chambre_smoke", smokeLevel);
    lastSmokeChambre = smokeLevel;
    if (smokeLevel > 1000) {
      sendTelemetry("chambre_alert", "smoke_detected");
    }
  }

  if (motion) sendTelemetry("chambre_motion", "detected");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print("Temp: "); display.println(temperature);
  display.print("Hum: "); display.println(humidity);
  display.print("Smoke: "); display.println(smokeLevel);
  display.print("Motion: "); display.println(motion ? "Oui" : "Non");
  display.display();

  digitalWrite(LED_PIN_CHAMBRE, (motion || smokeLevel > 1000) ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  startupTime = millis();

  Wire.begin(21, 22);
  lcd.init(); lcd.backlight(); lcd.print("Entrez code:");
  servo.attach(servoPin); servo.write(0);
  pinMode(pirPinEntree, INPUT);
  pinMode(ledPinEntree, OUTPUT);
  pinMode(buzzerPinEntree, OUTPUT);
  dhtCuisine.begin();
  ventilo.attach(SERVOPIN_CUISINE);
  pinMode(LEDPIN_CUISINE, OUTPUT);
  pinMode(BUZZERPIN_CUISINE, OUTPUT);
  pinMode(PIR_PIN_CHAMBRE, INPUT);
  pinMode(LED_PIN_CHAMBRE, OUTPUT);
  dhtChambre.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) while (true);
  display.clearDisplay();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  char key = keypad.getKey();
  if (key) {
    if (key == '*') {
      input_password = "";
      lcd.clear(); lcd.print("Entrez code:");
    }
    else if (isdigit(key)) {
      input_password += key;
      lcd.setCursor(0, 1); lcd.print("                ");
      lcd.setCursor(0, 1); lcd.print(input_password);
      if (input_password.length() == 4) {
        if (input_password == correct_password) openDoor();
        else {
          lcd.clear(); lcd.print("Code incorrect!");
          digitalWrite(buzzerPinEntree, HIGH);
          sendTelemetry("alarm", "wrong_code");
          delay(1000);
          digitalWrite(buzzerPinEntree, LOW);
          delay(1000);
          lcd.clear(); lcd.print("Entrez code:");
        }
        input_password = "";
      }
    }
  }

  if (!pirReady && millis() - startupTime >= pirActivationDelay) pirReady = true;

  if (pirReady && digitalRead(pirPinEntree) == HIGH && !door_open && millis() - lastMotionTime > alarmDuration) {
    lastMotionTime = millis();
    digitalWrite(ledPinEntree, HIGH);
    digitalWrite(buzzerPinEntree, HIGH);
    sendTelemetry("motion", "detected");
    delay(5000);
    digitalWrite(ledPinEntree, LOW);
    digitalWrite(buzzerPinEntree, LOW);
  }

  if (pirReady) {
    gestionCuisine();
    gestionChambre();
  }

  delay(200);
}
