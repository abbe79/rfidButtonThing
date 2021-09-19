#include <FS.h>
#include <MFRC522.h>      // https://github.com/miguelbalboa/rfid
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <Bounce2.h>      // https://github.com/thomasfredericks/Bounce2
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson

// button config
constexpr byte pinButtonPlay = 16; // D0
constexpr byte pinButtonUp   = 5;  // D1
constexpr byte pinButtonDown = 4;  // D2
constexpr byte pinResetRFID  = 0;  // D3
constexpr byte pinSSRFID     = 15; // D8
constexpr byte buttonDebounceTime = 20;

// wifi & mqtt config
constexpr int configStringLength = 64;
char hostName[configStringLength]      = "rfidButtonThing";
char mqttServer[configStringLength]    = "192.168.1.9";
char mqttTopicRFID[configStringLength] = "devices/rfidButtonThing/rfid";
char mqttTopicCmd[configStringLength]  = "devices/rfidButtonThing/cmd";
char mqttTopicWill[configStringLength] = "devices/rfidButtonThing/state";
bool shouldSaveConfig = false;
unsigned long lastUpMsg = 0;

// lib instances
MFRC522 mfrc522(pinSSRFID, pinResetRFID);
WiFiClient espClient;
PubSubClient client(espClient);
Bounce2::Button buttonPlay = Bounce2::Button();
Bounce2::Button buttonUp   = Bounce2::Button();
Bounce2::Button buttonDown = Bounce2::Button();

// WiFiManager callback
void saveConfigCallback () {
  Serial.println("should save config");
  shouldSaveConfig = true;
}

void sendUpMsg() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpMsg >= 60 * 1000) {
    lastUpMsg = currentMillis;
    Serial.println(F("sending alive message"));
    client.publish(mqttTopicWill, "1");    
  }
}

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

void readConfig() {
  //clean FS, for testing
  // Serial.println(F("!!!!! deleting FS !!!!!"));
  // SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
          Serial.println("\nparsed json");
          strncpy(hostName,      json["hostName"],      sizeof(hostName));
          strncpy(mqttServer,    json["mqttServer"],    sizeof(mqttServer));
          strncpy(mqttTopicRFID, json["mqttTopicRFID"], sizeof(mqttTopicRFID));
          strncpy(mqttTopicCmd,  json["mqttTopicCmd"],  sizeof(mqttTopicCmd));
          strncpy(mqttTopicWill, json["mqttTopicWill"], sizeof(mqttTopicWill));
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}

void saveConfig(){
  Serial.println("saving config");
  DynamicJsonDocument json(1024);
  json["hostName"]      = hostName;
  json["mqttServer"]    = mqttServer;
  json["mqttTopicRFID"] = mqttTopicRFID;
  json["mqttTopicCmd"]  = mqttTopicCmd;
  json["mqttTopicWill"] = mqttTopicWill;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  serializeJson(json, Serial);
  serializeJson(json, configFile);
  configFile.close();
}

void setup() {
  Serial.begin(74880);
  while (!Serial);

  readConfig();

  Serial.println(F("Initialize WiFiManager"));

  WiFiManagerParameter custom_hostName(     "hostName",      "Hostname",        hostName,      configStringLength);
  WiFiManagerParameter custom_mqttServer(   "mqttServer",    "MQTT Server",     mqttServer,    configStringLength);
  WiFiManagerParameter custom_mqttTopicRFID("mqttTopicRFID", "MQTT Topic RFID", mqttTopicRFID, configStringLength);
  WiFiManagerParameter custom_mqttTopicCmd( "mqttTopicCmd",  "MQTT Topic Cmd",  mqttTopicCmd,  configStringLength);
  WiFiManagerParameter custom_mqttTopicWill("mqttTopicWill", "MQTT Topic Will", mqttTopicWill, configStringLength);

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setTimeout(300);
  wifiManager.addParameter(&custom_hostName);
  wifiManager.addParameter(&custom_mqttServer);
  wifiManager.addParameter(&custom_mqttTopicRFID);
  wifiManager.addParameter(&custom_mqttTopicCmd);
  wifiManager.addParameter(&custom_mqttTopicWill);

  //reset settings - for testing
  // wifiManager.resetSettings();

  if(!wifiManager.autoConnect(hostName)) {
    Serial.println(F("Failed to connect"));
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  Serial.println(F("WiFI connected"));
  //read updated parameters
  strncpy(hostName,      custom_hostName.getValue(),      sizeof(hostName));
  strncpy(mqttServer,    custom_mqttServer.getValue(),    sizeof(mqttServer));
  strncpy(mqttTopicRFID, custom_mqttTopicRFID.getValue(), sizeof(mqttTopicRFID));
  strncpy(mqttTopicCmd,  custom_mqttTopicCmd.getValue(),  sizeof(mqttTopicCmd));
  strncpy(mqttTopicWill, custom_mqttTopicWill.getValue(), sizeof(mqttTopicWill));

  Serial.println("updated config values are: ");
  Serial.println("\thostName : "      + String(hostName));
  Serial.println("\tmqttServer : "    + String(mqttServer));
  Serial.println("\tmqttTopicRFID : " + String(mqttTopicRFID));
  Serial.println("\tmqttTopicCmd : "  + String(mqttTopicCmd));
  Serial.println("\tmqttTopicWill : " + String(mqttTopicWill));

  if(shouldSaveConfig){
    saveConfig();
  }

  Serial.println(F("connect to MQTT"));
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

  sendUpMsg();

  yield();
}