# rfidButtonThing
This is kind of a "Tonie Box" knock off with the sound stuff delegated away.

You'll need an ESP8266, a MFRC522 RFID reader and three buttons.

Detected RFID card (serial number) or button presses are sent to an MQTT broker. From there it's up to you.

In my case my home automation system does a lookup which audio file matches the RFID serial number and starts playing the file on a Sonos speaker.
Button presses are used as "volume up/down" and "play/pause".
