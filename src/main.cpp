#include <Arduino.h>
#include <Wire.h>
#include "DHT.h"
#include "sd_read_write.h"
#include <SD_MMC.h>
#include "RTClib.h"
#include <ArduinoJson.h>

#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0 40

#define DHTPIN 48
#define DHTTYPE DHT11
#define SDAPIN 19
#define SCLPIN 20


RTC_DS3231 rtc;
DHT dht(DHTPIN, DHTTYPE);
JsonDocument doc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dht.begin();
  Wire.begin(SDAPIN, SCLPIN);
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    Serial.println("Fejl: SD kort kunne ikke initialiseres!");
    while (1)
    {
      /* code */
    }
    
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

  File file = SD_MMC.open("/Data/telemetry.json", FILE_WRITE);
  if (!file) {
    Serial.println("Kunne ikke åbne fil til skrivning!");
    writeFile(SD_MMC, "/Data/telemetry.json", "[");
  }
  listDir(SD_MMC, "/", 0);
 
  Serial.println("SD kort klar.");
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
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
  // Wait a few seconds between measurements.
  delay(5000);
  DateTime now = rtc.now();
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

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
 
  
  File file = SD_MMC.open("/Data/telemetry.json", FILE_APPEND);
  if (!file) {
    Serial.println("Kunne ikke åbne fil til skrivning!");
    return;
  }
  //WriteLoggingStream loggedFile(file, Serial);
  // char r = file.read();
  // String fileInText;
  // while(file.available())
  //   fileInText += r;

  // Serial.println(fileInText);

  serializeJsonPretty(doc, file);
  file.close();
  Serial.println();
  serializeJsonPretty(doc, Serial);
  

  Serial.println();
  
  

}

