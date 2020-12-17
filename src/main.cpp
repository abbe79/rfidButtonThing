#include <stdint.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>      // https://github.com/miguelbalboa/rfid
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <Bounce2.h>      // https://github.com/thomasfredericks/Bounce2

constexpr byte pinButtonPlay = 16; // D0
constexpr byte pinButtonUp   = 5;  // D1

constexpr byte pinButtonDown = 4;  // D2
constexpr byte pinResetRFID  = 0;  // D3
constexpr byte pinSSRFID     = 15; // D8

constexpr byte buttonDebounceTime = 20;
constexpr const char* hostName      = "rfidButtonThing";
constexpr const char* mqttServer    = "nas.local";
constexpr const char* mqttTopicRFID = "devices/rfidButtonThing/rfid";
constexpr const char* mqttTopicCmd  = "devices/rfidButtonThing/cmd";
constexpr const char* mqttTopicWill = "devices/rfidButtonThing/state";

MFRC522 mfrc522(pinSSRFID, pinResetRFID);
WiFiClient espClient;
PubSubClient client(espClient);
Button buttonPlay = Button();
Button buttonUp   = Button();
Button buttonDown = Button();

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

  Serial.println(F("Initialize Bounce2"));
  // GPIO16 has no pullup, so do this the other way round 
  buttonPlay.attach(pinButtonPlay, INPUT_PULLDOWN_16);
  buttonPlay.interval(buttonDebounceTime);
  buttonPlay.setPressedState(HIGH);

  buttonUp.attach(pinButtonUp, INPUT_PULLUP);
  buttonUp.interval(buttonDebounceTime);
  buttonUp.setPressedState(LOW);

  buttonDown.attach(pinButtonDown, INPUT_PULLUP);
  buttonDown.interval(buttonDebounceTime);
  buttonDown.setPressedState(LOW);
  
  Serial.println(F("Ready"));
}

bool readCard(){
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String result;
    char buffer[3];
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      itoa(mfrc522.uid.uidByte[i], buffer, 16);
      result += buffer;
      if((i+1) < mfrc522.uid.size){
        result += '-';
      }
    } 
    Serial.print(F("Found card "));
    Serial.println(result);
    client.publish(mqttTopicRFID, result.c_str());
    return true;
  }
  Serial.println(F("Could not read card"));
  return false;
}

void loop() {
  if (!client.connected()) {
    mqttReconnect();
  }
  client.loop();
  buttonPlay.update();
  buttonUp.update();
  buttonDown.update();

  if(buttonPlay.pressed()){
    Serial.println(F("Button playPause pressed"));
    // check if there is a card present
    if(!readCard()){
      // if no card was found send a playPause
      client.publish(mqttTopicCmd, "playPause");
    }
  } else if(buttonUp.pressed()){
    Serial.println(F("Button up pressed"));
    client.publish(mqttTopicCmd, "up");
  } else if(buttonDown.pressed()){
    Serial.println(F("Button down pressed"));
    client.publish(mqttTopicCmd, "down");
  }

  yield();
}