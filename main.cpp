#include <WiFi.h>
#include <PubSubClient.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Wire.h>

// Configuration WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Configuration MQTT
const char* mqtt_server = "mqtt.eu.thingsboard.cloud"; //thingsboard server
const int mqtt_port = 1883;
const char* access_token = "1rfse61l1zqltl5baa9t"; 

// Configuration LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Configuration clavier
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {32 , 33 , 25 , 26};
byte colPins[COLS] = {27 , 14 , 12 , 13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Configuration servo
Servo servo;
const int servoPin = 17;
const int pirPin = 6;
const int ledPin = 16;
const int buzzerPin = 23;

// Variables
String input_password = "";
const String correct_password = "0000";
bool door_open = false;

WiFiClient espClient;
PubSubClient client(espClient);

// Ajouter cette fonction debug :
void printMQTTState(int state) {
  Serial.print("État MQTT: ");
  switch(state) {
    case -4: Serial.println("Timeout"); break;
    case -3: Serial.println("Connection lost"); break;
    case -2: Serial.println("Connect failed"); break;
    case -1: Serial.println("Disconnected"); break;
    case 0: Serial.println("Connected"); break;
    case 1: Serial.println("Bad protocol"); break;
    case 2: Serial.println("Client rejected"); break;
    case 3: Serial.println("Server unavailable"); break;
    case 4: Serial.println("Bad credentials"); break;
    case 5: Serial.println("Unauthorized"); break;
    default: Serial.println("Unknown"); break;
  }
}


void setup_wifi() {
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password, 6, NULL, true); // Canal 6, désactive le NTP
  
  Serial.print("Connexion WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nIP: ");
    Serial.println(WiFi.localIP());
    Serial.print("DNS config: ");
    Serial.println(WiFi.dnsIP()); // Doit afficher 10.13.37.1
  } else {
    Serial.println("\nErreur WiFi!");
  }
}


void reconnect() {
    while (!client.connected()) {
      Serial.print("Connexion MQTT avec token: ");
      Serial.println(access_token); // Affiche le token utilisé
      
      if (client.connect("ESP32-Door", access_token, NULL)) {
        Serial.println("Connecté à ThingsBoard!");
      } else {
        Serial.print("Erreur, rc=");
        Serial.print(client.state());
        Serial.println(" - Vérifiez:");
        Serial.println("1. Token valide");
        Serial.println("2. Device actif dans ThingsBoard");
        Serial.println("3. WiFi connecté");
        delay(5000);
      }
    }
  }

void sendTelemetry(const char* key, const char* value) {
  char payload[100];
  snprintf(payload, sizeof(payload), "{\"%s\":\"%s\"}", key, value);
  client.publish("v1/devices/me/telemetry", payload);
  Serial.println(payload);
}

// ouverture/fermeture de la porte 

void openDoor() {
  servo.write(90);
  door_open = true;
  lcd.clear();
  lcd.print("Porte ouverte!");
  sendTelemetry("door", "open");
  delay(5000);
  servo.write(0);
  door_open = false;
  sendTelemetry("door", "closed");
  lcd.clear();
  lcd.print("Entrez code:");
}

void setup() {
  Serial.begin(115200);
  
  // Initialisation matérielle
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.print("Entrez code:");
  
  servo.attach(servoPin);
  servo.write(0);
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  
  // Connexion réseau
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Gestion clavier (à revoir)
  char key = keypad.getKey();
  if (key) {
    if (key == '*') {
      input_password = "";
      lcd.clear();
      lcd.print("Entrez code:");
    } 
    else if (isdigit(key)) {
      input_password += key;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(input_password);
      
      if (input_password.length() == 4) {
        if (input_password == correct_password) {
          openDoor();
        } else {
          lcd.clear();
          lcd.print("Code incorrect!");
          digitalWrite(buzzerPin, HIGH);
          sendTelemetry("alarm", "wrong_code");
          delay(1000);
          digitalWrite(buzzerPin, LOW);
          delay(1000);
          lcd.clear();
          lcd.print("Entrez code:");
        }
        input_password = "";
      }
    }
  }

  // Détection mouvement
  if (digitalRead(pirPin) == HIGH && !door_open) {
    digitalWrite(ledPin, HIGH);
    sendTelemetry("motion", "detected");
    delay(5000);
    digitalWrite(ledPin, LOW);
  }
  
  delay(100);
}