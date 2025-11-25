#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "023ef5e7dc384a739b05409b387ae2f0.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "user11";
const char* mqtt_pass = "user11ABC";

WiFiClientSecure espClient;
PubSubClient client(espClient);

const int pinLock = 32;  // Relay ควบคุมระบบล็อค
const int pinDoor = 33;  // Relay ควบคุมกลอนประตู

#define SS_PIN  5
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);
byte authorizedUID[4] = {0x24, 0xA5, 0x9A, 0x2B}; // รหัสบัตรที่อนุญาต

const char* t_cmd_lock  = "home/security/lock/cmd";
const char* t_stat_lock = "home/security/lock/status";
const char* t_cmd_door  = "home/security/door/cm1";
const char* t_stat_door = "home/security/door/status";

bool isLocked = true;

void setup_wifi();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
bool checkUID(byte *uid);
void openDoorSequence(String source); 

void setup() {
  Serial.begin(115200);

  pinMode(pinLock, OUTPUT);
  pinMode(pinDoor, OUTPUT);
  
  digitalWrite(pinLock, LOW);
  digitalWrite(pinDoor, LOW);

  SPI.begin();
  rfid.PCD_Init();

  espClient.setInsecure();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) reconnect();
  client.loop();

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
  
    Serial.print("Card UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();

    if (checkUID(rfid.uid.uidByte)) {
      openDoorSequence("RFID Card");
    } else {
      Serial.println("Access Denied!");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

void openDoorSequence(String source) {
    Serial.println(">>> ACCESS GRANTED by: " + source);
    
    if (isLocked) {
        isLocked = false;
        digitalWrite(pinLock, HIGH);
        client.publish(t_stat_lock, "UNLOCK");
        delay(200); 
    }

    Serial.println("Opening Door...");
    digitalWrite(pinDoor, HIGH);
    client.publish(t_stat_door, "OPEN");
    
    Serial.println("Waiting 5 seconds...");
    delay(5000); 

    Serial.println("Closing Door...");
    digitalWrite(pinDoor, LOW);
    client.publish(t_stat_door, "CLOSE");
}

bool checkUID(byte *uid) {
  for (int i = 0; i < 4; i++) {
    if (uid[i] != authorizedUID[i]) return false;
  }
  return true;
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  String strTopic = String(topic);

  Serial.print("MQTT CMD: "); Serial.println(msg);

  if (strTopic == t_cmd_lock) {
      if (msg == "UNLOCK") {
          isLocked = false;
          digitalWrite(pinLock, HIGH);
          client.publish(t_stat_lock, "UNLOCK");
      }
      else if (msg == "LOCK") {
          isLocked = true;
          digitalWrite(pinLock, LOW);
          digitalWrite(pinDoor, LOW);
          client.publish(t_stat_door, "CLOSE");
          client.publish(t_stat_lock, "LOCK");
          Serial.println("LOCKED via MQTT");
      }
  }
  else if (strTopic == t_cmd_door) {
      if (isLocked) {
          Serial.println("Error: System is Locked!");
          return;
      }
      if (msg == "OPEN") {
          digitalWrite(pinDoor, HIGH);
          client.publish(t_stat_door, "OPEN");
      } else if (msg == "CLOSE") {
          digitalWrite(pinDoor, LOW);
          client.publish(t_stat_door, "CLOSE");
      }
  }
}

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("Connected");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to HiveMQ...");
    if (client.connect("ESP32_ID", mqtt_user, mqtt_pass)) {
      Serial.println("Connected!");
      client.subscribe(t_cmd_lock);
      client.subscribe(t_cmd_door);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}