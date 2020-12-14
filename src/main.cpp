#include <stdint.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>      // https://github.com/miguelbalboa/rfid
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient

constexpr byte pinResetRFID = 0;
constexpr byte pinSSRFID = 15;
constexpr const char* hostName = "rfidButtonThing";
constexpr const char* mqttServer = "nas.local";
constexpr const char* mqttTopicRFID = "devices/rfidButtonThing/rfid";
constexpr const char* mqttTopicCmd  = "devices/rfidButtonThing/cmd";
constexpr const char* mqttTopicWill = "devices/rfidButtonThing/state";

MFRC522 mfrc522(pinSSRFID, pinResetRFID);
WiFiClient espClient;
PubSubClient client(espClient);

void mqttReconnect() {
  Serial.print(F("Attempting MQTT connection..."));
  if (client.connect(hostName, NULL, NULL, mqttTopicWill, 1, false, "0")) {
    Serial.println(F("connected"));
    client.publish(mqttTopicWill, "1");
  } else {
    Serial.print(F("failed, rc="));
    Serial.print(client.state());
    Serial.println(F(" try again..."));
    delay(500);
  }
}

void setup() {
  Serial.begin(74880);
  while (!Serial);

  Serial.println(F("Initialize WiFiManager"));
  WiFiManager wifiManager;
  wifiManager.setTimeout(300);

  if(!wifiManager.autoConnect(hostName)) {
    Serial.println(F("Failed to connect"));
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  Serial.println(F("WiFI connected"));
  client.setServer(mqttServer, 1883);
  mqttReconnect();

	Serial.println(F("Initialize MFRC522"));
	SPI.begin();
	mfrc522.PCD_Init();
	delay(5);
	mfrc522.PCD_DumpVersionToSerial();
}

String readCard(){
  String result;
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    char buffer[3];
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      itoa(mfrc522.uid.uidByte[i], buffer, 16);
      result += buffer;
      if((i+1) < mfrc522.uid.size){
        result += ':';
      }
    } 
    Serial.print(F("Found card "));
    Serial.println(result);
  }
  return result;
}

void loop() {
  if (!client.connected()) {
    mqttReconnect();
  }
  client.loop();

  String rfidString = readCard();
  if(!rfidString.isEmpty()){
    client.publish(mqttTopicRFID, rfidString.c_str());
  }
  delay(100);

  yield();
}