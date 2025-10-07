#include <Arduino.h>
#include <Wire.h>
#include "DHT.h"
#include "sd_read_write.h"
#include <SD_MMC.h>
#include "RTClib.h"
#include <ArduinoJson.h>

#define SD_MMC_CMD 15
#define SD_MMC_CLK 14
#define SD_MMC_D0 2

#define DHTPIN 48
#define DHTTYPE DHT11
#define SDAPIN 19
#define SCLPIN 20
#define SD_CS 10

RTC_DS3231 rtc;
DHT dht(DHTPIN, DHTTYPE);
JsonDocument doc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dht.begin();
  Wire.begin(SDAPIN, SCLPIN);
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    Serial.println("Fejl: SD kort kunne ikke initialiseres!");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE){
    Serial.println("Ikke noget kort tilsluttet");
    return;
  }
 
  
  Serial.println("SD kort klar.");
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

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
  delay(2000);
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
  
  serializeJsonPretty(doc, Serial);

  // Print the complete formatted time

  // // Getting temperature
  // Serial.print(rtc.getTemperature());
  // Serial.println("ºC");

  Serial.println();
  File fil = SD_MMC.open("/data.json", FILE_WRITE);
  if (!fil) {
    Serial.println("Kunne ikke åbne fil til skrivning!");
    return;
  }

  // Skriv JSON til fil
  serializeJson(doc, fil);
  fil.close();
}

