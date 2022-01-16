// Neopixel
#include "Adafruit_NeoPixel.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "ArduinoOTA.h"
#include "ArduinoJson.h"
#include "EEPROM.h"

// eeprom set
//int addr = 0;
#define EEPROM_SIZE 100

// MQTT Subscribes
const char* SubscribeTo = "esp/1/out";
const char* SubscribeFrom = "esp/1/in";

// WiFi & MQTT Server
const char* ssid = "YourWlan";
const char* password = "YourWlanPassword";
const char* mqtt_server = "YourMqttServer";
const char* mqttUser = "YourMqttUser";
const char* mqttPassword = "YourMqttPassword";
const String ver = "1.0";

String AcceePointName = "iZer0_Led_v:" + ver;

WiFiClient espClient;
PubSubClient pubClient(espClient);
long lastMsg = 0;
char msg[50];
String message = "#000000";
String lastMessage = "#000000";
String color = "#000001";
int brightness = 30;

// Neopixel Config
#define NeoPIN 13
#define NUM_LEDS 80

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NeoPIN, NEO_RGB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);

  // ##############
  // NeoPixel start
  //Serial.println();
  readEEPROM();
  setLed(color);
  //strip.setBrightness(brightness);
  strip.begin();
  //strip.show();
  //setNeoColor("#f3903c");
  delay(100);

  // WiFi
  setup_wifi();

  // MQTT
  Serial.print("Connecting to MQTT...");
  // connecting to the mqtt server
  pubClient.setServer(mqtt_server, 1883);
  pubClient.setCallback(callback);

  ArduinoOTA.setHostname(AcceePointName.c_str());

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("End OTA");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("done!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  color = "";

  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    if (i >= 0) {
      color += String((char)payload[i]);
    }
  }

  Serial.print(color);
  writeEEPROM();
  //readEEPROM();
  setLed(color);

}

void writeEEPROM() {
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("failed to init EEPROM");
    delay(1000);
  }
  int str_len = color.length() + 1;
  char newColor[str_len];
  color.toCharArray(newColor, str_len);
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, newColor[i]);
  }
  EEPROM.commit();
  Serial.println();
  Serial.print("Write eeprom ");
  Serial.println(color);
  Serial.println();
}

void readEEPROM() {
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("failed to init EEPROM");
    delay(1000);
  }
  String tmpColor = "";
  for (int i = 0; i < EEPROM_SIZE; i++) {
    byte readValue = EEPROM.read(i);

    if (readValue == 0) {
      break;
    }
    tmpColor += String((char)readValue);
  }
  if (tmpColor != "") {
    color = tmpColor;
  }
  Serial.println();
  Serial.print("Read eeprom ");
  Serial.println(tmpColor);
  Serial.println();
}

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  ArduinoOTA.handle();
  if (!pubClient.connected()) {
    delay(100);
    reconnect();
  }
  pubClient.loop();

  long now = millis();
  if (now - lastMsg > 60000 || lastMessage.indexOf(message) < 0 ) {

    lastMsg = now;
    Serial.print("Publish message: ");
    Serial.println(message);
    char msg[7];
    message.toCharArray(msg, message.length());
    Serial.print("Publishing...");
    pubClient.publish(SubscribeTo, msg);
    Serial.println("Done!!!");
    lastMessage = message;
  }
  delay(10);
}

void setLed(String json) {
  if (json != "") {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    if (doc["status"] == 1) {
      setNeoColor(doc["color"]);
      setNeoBrightness(doc["brightness"]);
    } else {
      setNeoColor("#000000");
    }
  }
}

void setNeoBrightness(int br) {
  br = map(br, 0, 100, 0, 255);
  Serial.println(br);
  strip.setBrightness(br);
  strip.show();
}

void setNeoColor(String value) {
  Serial.println("Setting Neopixel..." + value);
  message = value;
  int number = (int) strtol( &value[1], NULL, 16);

  int r = number >> 16;
  int g = number >> 8 & 0xFF;
  int b = number & 0xFF;

  Serial.print("RGB: ");
  Serial.print(r, DEC);
  Serial.print(" ");
  Serial.print(g, DEC);
  Serial.print(" ");
  Serial.print(b, DEC);
  Serial.println(" ");

  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color( g, r, b ) );
  }
  strip.show();
    Serial.println("on.");
}

void reconnect() {
  while (!pubClient.connected()) {
    if (pubClient.connect("ESP32Client", mqttUser, mqttPassword )) {
      if (pubClient.connect("ESP8266Client")) {
        Serial.println("connected");
        pubClient.publish(SubscribeTo, "Restart");
        pubClient.subscribe(SubscribeFrom);
      } else {
        Serial.print("failed, rc=");
        Serial.print(pubClient.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
      }
    }
  }
}
