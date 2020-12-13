#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>     // https://github.com/miguelbalboa/rfid
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#define RST_PIN 0
#define SS_PIN  15

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(74880);
  while (!Serial);

  Serial.println(F("Initialize WiFiManager"));
  WiFiManager wifiManager;
  wifiManager.autoConnect();

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
  readCard();
  delay(100);
}