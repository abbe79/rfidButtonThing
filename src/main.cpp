#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>      // https://github.com/miguelbalboa/rfid
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient

#define RST_PIN 0
#define SS_PIN  15
#define HOST "rfidButtonThing"
#define MQTT_SRV "nas.local"
#define MQTT_USR NULL
#define MQTT_PWD NULL
#define TOPIC "devices/rfidButtonThing/command"

MFRC522 mfrc522(SS_PIN, RST_PIN);
WiFiClient espClient;
PubSubClient client(espClient);

void mqttReconnect() {
  Serial.print(F("Attempting MQTT connection..."));
  if (client.connect(HOST, MQTT_USR, MQTT_PWD, TOPIC, 1, false, "down")) {
  //if (client.connect(HOST)) {
    Serial.println(F("connected"));
    client.publish(TOPIC, "up");
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again...");
    delay(500);
  }
}

void setup() {
  Serial.begin(74880);
  while (!Serial);

  Serial.println(F("Initialize WiFiManager"));
  WiFiManager wifiManager;
  wifiManager.setTimeout(300);

  if(!wifiManager.autoConnect(HOST)) {
    Serial.println(F("Failed to connect"));
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  Serial.println(F("WiFI connected"));
  client.setServer(MQTT_SRV, 1883);
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
    client.publish(TOPIC, rfidString.c_str());
  }
  delay(100);

  yield();
}