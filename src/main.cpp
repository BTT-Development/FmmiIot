#include <Arduino.h>
#include "arduino_secrets.h"
#include <Wire.h>
#include "DHT.h"
#include "sd_read_write.h"
#include <SD_MMC.h>
#include "RTClib.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <StreamUtils.h>

// SD-card pins
#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0  40

#define DHTPIN 48
#define DHTTYPE DHT11
#define SDAPIN 19
#define SCLPIN 20

#define SCREEN_WIDTH  128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1

#define I2C_A_SDA 13 // GPIO8
#define I2C_A_SCL 14 // GPIO9

unsigned long lastIntervalSend = 0;
unsigned long lastRealtimeSend = 0;
unsigned long intervalDelay = 10000; 
unsigned long realtimeDelay = 1000;  

const char *pathTemperature = "/Data/temperature.json";
const char *pathHumidity =    "/Data/humidity.json";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

WiFiClientSecure espClient;

PubSubClient mqttClient(espClient);

RTC_DS3231 rtc;
DHT dht(DHTPIN, DHTTYPE);

bool dhtError = false;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

JsonDocument docTemperature; 
JsonDocument docHumidity;

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    String msg;
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
        msg += (char)payload[i];
    }
    Serial.println();
    Serial.println("-----------------------");
    if (String(topic) == SECRET_MQTT_TIMEINTERVAL_TOPIC)
    {
      DynamicJsonDocument doc(128);
      deserializeJson(doc, (char*) payload, length);
      realtimeDelay = doc["RealTimeinterval"];
      intervalDelay = doc["Timeinterval"];
    }
    
}

void ConnectToWifi(){
  int attempCount = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  display.clearDisplay();
  display.setTextColor(WHITE); 
  display.setCursor(0,0);
  display.print(F("Venter på WiFi."));
  display.display();
  Serial.print("Venter på WiFi.");
  while(WiFi.status() != WL_CONNECTED && attempCount < 5)
  {
    Serial.print(" .");
    display.print(" .");
    display.display();
    delay(500);
    attempCount++;
    Serial.println("Reconnecting to WiFi...");
  }
  if(WiFi.status() == WL_CONNECTED){
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("WiFi connected");
    display.println("IP address: ");
    display.println(WiFi.localIP());
    display.display();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.status());
  }
  else{
     display.clearDisplay();
     display.print("Kunne ikke oprette forbindelse til WiFi");
     display.display();
   
  }
}

void ReconnectToMqttBroker(){
  int attempCount = 0;
  while (!mqttClient.connected() && attempCount < 5) {
    String client_id = "fmmiEsp32-" + String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    byte qos = 1;
    bool retained = true;
    if (mqttClient.connect(client_id.c_str(), SECRET_MQTT_USERNAME, SECRET_MQTT_PASSWORD, SECRET_MQTT_LASTWILL_TOPIC, qos, retained, SECRET_MQTT_LASTWILLMSG)) {
        Serial.println("Public EMQX MQTT broker connected");
        mqttClient.publish(SECRET_MQTT_LASTWILL_TOPIC, "Online", true);
        mqttClient.subscribe(SECRET_MQTT_TIMEINTERVAL_TOPIC);
        
    } else {
        Serial.print("failed with state ");
        Serial.print(mqttClient.state());
        attempCount++;
        delay(500);
    }
  }
}
void PublishMqttJson(PubSubClient &client, const char *topic, JsonDocument &doc, bool retained){
  size_t length = measureJson(doc);   
  client.beginPublish(topic, length, retained);
  BufferingPrint bufferedClient(client, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  if(mqttClient.endPublish()){
    Serial.println("Data sendt");
  }
}
void WriteToSdCard(const char *path, JsonDocument doc){
  Serial.println("starter skrivning");
   DynamicJsonDocument docArray(4096); 
      File readFile = SD_MMC.open(path, FILE_READ);
      if (readFile) {
        DeserializationError error = deserializeJson(docArray, readFile);
        readFile.close();
        if (error) {
          Serial.println("Fejl ved indlæsning af eksisterende JSON, starter nyt array.");
          docArray.clear();
          docArray.to<JsonArray>(); 
        }
      }
      else{
        docArray.to<JsonArray>(); 
      }

      docArray.add(doc);
      File writeFile = SD_MMC.open(path, FILE_WRITE);
      if (!writeFile) {
        Serial.println("Kunne ikke åbne fil til skrivning!");
        return;
      }
      serializeJsonPretty(docArray, writeFile);
      writeFile.close();
      Serial.println("Gemt på sd kort");
      return;
}

void SendJsonArray(const char *path){
   DynamicJsonDocument docArray(4096); 
      File readFile = SD_MMC.open(path, FILE_READ);
      if (readFile) 
      {
        DeserializationError error = deserializeJson(docArray, readFile);
        if (!error)
        {
          PublishMqttJson(mqttClient, SECRET_MQTT_DATA_TEMPERATURE_TOPIC, docArray, false);
          SD_MMC.remove(path);
        }
      }
      Serial.println("json file sendt");
      readFile.close();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire1.begin(I2C_A_SDA, I2C_A_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  ConnectToWifi();
  espClient.setCACert(ca_cert);
  mqttClient.setServer(SECRET_MQTT_BROKER, SECRET_MQTT_PORT);
  mqttClient.setCallback(callback);
  ReconnectToMqttBroker();

  dht.begin();
  Wire.begin(SDAPIN, SCLPIN);
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    Serial.println("Fejl: SD kort kunne ikke initialiseres!");
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE){
    Serial.println("Ikke noget kort tilsluttet");
    return;
  }
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

  File root = SD_MMC.open("/Data");
    if(!root){
      Serial.println("Failed to open directory");
      createDir(SD_MMC, "/Data");
    }
    if(!root.isDirectory()){
      Serial.println("Not a directory");
    }
  root.close();
  File josnFile = SD_MMC.open("/Data/temperature.json", FILE_READ);
  if (!josnFile) {
    Serial.println("Ingen json fil");
    
  }
  josnFile.close();
  listDir(SD_MMC, "/", 0);
 
  Serial.println("SD kort klar.");
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  Serial.println("RTC klar");

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

void loop() {
  unsigned long miliesNow = millis();
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Reconnecting to WiFi...");
    display.println("Forbinder til WiFi...");
    display.display();
    WiFi.disconnect();
    WiFi.reconnect();
  }
  if (!mqttClient.connected())
  {
    ReconnectToMqttBroker();
    if(mqttClient.connected())
    {
      SendJsonArray(pathTemperature);
      SendJsonArray(pathHumidity);
    }
  }
 
 
  mqttClient.loop();
  
  display.setCursor(0,0);
  DateTime now = rtc.now(); // Reading date and time.
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity(); // Read temperature as Celsius (the default)
  float t = dht.readTemperature();// Read temperature as Fahrenheit (isFahrenheit = true)
  

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    display.println("Failed to read from DHT sensor!");
    display.display();
    mqttClient.publish(SECRET_MQTT_ERROR_TOPIC, "Kan ikke læse DHT sensor", true);
    dhtError = true;
    return;
  }
  if (dhtError)
  {
    mqttClient.publish(SECRET_MQTT_ERROR_TOPIC, "", true);
    dhtError = false;
  }
  

  display.print("Humidity:      ");
  display.print(h);
  display.println("%");
  display.print("Temperature:   ");
  display.print(t);
  display.drawCircle(122, 10, 2, WHITE);
  display.println(); 
  
  String yearStr = String(now.year(), DEC);
  String monthStr = (now.month() < 10 ? "0" : "") + String(now.month(), DEC);
  String dayStr = (now.day() < 10 ? "0" : "") + String(now.day(), DEC);
  String hourStr = (now.hour() < 10 ? "0" : "") + String(now.hour(), DEC); 
  String minuteStr = (now.minute() < 10 ? "0" : "") + String(now.minute(), DEC);
  String secondStr = (now.second() < 10 ? "0" : "") + String(now.second(), DEC);
  String dayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];

  String formattedTime = dayOfWeek + ", " + yearStr + "-" + monthStr + "-" + dayStr + " " + hourStr + ":" + minuteStr + ":" + secondStr;

  docTemperature["DeviceID"] = docHumidity["DevicesID"] = 1;
  docTemperature["Date"] =  docHumidity["Date"]  = formattedTime;
  docTemperature["DataTypeID"] = 1;
  docHumidity["DataTypeId"] = 2;
  docTemperature["Value"] = t;
  docHumidity["Value"] = h;

  if(miliesNow - lastRealtimeSend >= realtimeDelay){
    lastRealtimeSend = miliesNow;
    if (mqttClient.connected())
    {
      PublishMqttJson(mqttClient, SECRET_MQTT_REALTIME_TEMPERATURE_TOPIC, docTemperature, false);
  
      PublishMqttJson(mqttClient, SECRET_MQTT_REALTIME_HUMIDITY_TOPIC, docHumidity, false);
    }
    
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  }
  if (miliesNow - lastIntervalSend >= intervalDelay)
  {
    lastIntervalSend = miliesNow;
    Serial.println(formattedTime);
  
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE){
      display.println("Mangler SD kort");
    }
    else{
      display.println("SD kort tilsluttet");
    }
   
    if (!mqttClient.connected()) {
          Serial.println("start temp sd");
      WriteToSdCard(pathTemperature, docTemperature);
          Serial.println("start hum sd");
      WriteToSdCard(pathHumidity, docHumidity);
      return;
    } 
    display.print("Forbundet til Wifi");
    
    display.display();
    Serial.println();
    PublishMqttJson(mqttClient, SECRET_MQTT_DATA_TEMPERATURE_TOPIC, docTemperature, false);
    PublishMqttJson(mqttClient, SECRET_MQTT_DATA_HUMIDITY_TOPIC, docHumidity, false);
    Serial.println();
   }
   
}


