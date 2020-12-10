#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 0
#define SS_PIN  15

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(74880);
  while (!Serial);

	SPI.begin();
	mfrc522.PCD_Init();
	delay(40);
	mfrc522.PCD_DumpVersionToSerial();
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void loop() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}