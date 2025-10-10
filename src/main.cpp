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

// SD-card pins
#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0 40

#define DHTPIN 48
#define DHTTYPE DHT11
#define SDAPIN 19
#define SCLPIN 20

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1

#define I2C_A_SDA 13 // GPIO8
#define I2C_A_SCL 14 // GPIO9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);


const char *ssid_Router     =  SECRET_SSID; //Enter the router name
const char *password_Router =  SECRET_PASS; //Enter the router password

unsigned long previousMillis = 0;
unsigned long interval = 30000;

WiFiClientSecure espClient;

PubSubClient mqttClient(espClient);

RTC_DS3231 rtc;
DHT dht(DHTPIN, DHTTYPE);
JsonDocument doc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
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
  while(WiFi.status() != WL_CONNECTED && attempCount < 10)
  {
    Serial.print(" .");
    display.print(" .");
    display.display();
    delay(500);
    attempCount++;
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
  }
  else{
     display.clearDisplay();
     display.print("Kunne ikke oprette forbindelse til WiFi");
     display.display();
  }
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
  mqttClient.setKeepAlive(60);
  mqttClient.setCallback(callback);


      while (!mqttClient.connected()) {
        String client_id = "fmmiEsp32-" + String(WiFi.macAddress());
        Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        if (mqttClient.connect(client_id.c_str(), SECRET_MQTT_USERNAME, SECRET_MQTT_PASSWORD)) {
            Serial.println("Public EMQX MQTT broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.print(mqttClient.state());
            delay(2000);
        }
    }


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
  File josnFile = SD_MMC.open("/Data/telemetry.json", FILE_WRITE);
  if (!josnFile) {
    Serial.println("Kunne ikke åbne fil til skrivning!");
    writeFile(SD_MMC, "/Data/telemetry.json", "[");
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
  mqttClient.loop();
  delay(5000); // Wait a few seconds between measurements.
  display.clearDisplay();
  display.setCursor(0,0);
  DateTime now = rtc.now(); // Reading date and time.
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity(); // Read temperature as Celsius (the default)
  float t = dht.readTemperature();// Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    display.println("Failed to read from DHT sensor!");
    display.display();
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);
  display.print("Humidity:      ");
  display.print(h);
  display.println("%");
  display.print("Temperature:   ");
  display.print(t);
  display.drawCircle(122, 10, 2, WHITE);
  display.println(); 
  
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));

  String yearStr = String(now.year(), DEC);
  String monthStr = (now.month() < 10 ? "0" : "") + String(now.month(), DEC);
  String dayStr = (now.day() < 10 ? "0" : "") + String(now.day(), DEC);
  String hourStr = (now.hour() < 10 ? "0" : "") + String(now.hour(), DEC); 
  String minuteStr = (now.minute() < 10 ? "0" : "") + String(now.minute(), DEC);
  String secondStr = (now.second() < 10 ? "0" : "") + String(now.second(), DEC);
  String dayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];

  String formattedTime = dayOfWeek + ", " + yearStr + "-" + monthStr + "-" + dayStr + " " + hourStr + ":" + minuteStr + ":" + secondStr;

  doc["ID"] = 1;
  doc["Sensor"] = "DHT11";
  doc["Date"] = formattedTime;
  doc["Temperature"] = t;
  doc["Humidity"] = h;
  
  Serial.println(formattedTime);
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE){
    display.println("Mangler SD kort");
    
  }
  else{
    display.println("SD kort tilsluttet");
  }
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED)) {
  
    Serial.println("Reconnecting to WiFi...");
    display.println("Forbinder til WiFi...");
    display.display();
    WiFi.disconnect();
    WiFi.reconnect();

    File file = SD_MMC.open("/Data/telemetry.json", FILE_APPEND);
    if (!file) {
      Serial.println("Kunne ikke åbne fil til skrivning!");
      file.close();
      return;
    }
    serializeJsonPretty(doc, file);
    file.close();
    return;
  } 
  display.print("Forbundet til Wifi");
  
  display.display();
  Serial.println();
  serializeJsonPretty(doc, Serial);
  mqttClient.publish(SECRET_MQTT_TOPIC, "TEST");
  Serial.println();
}

