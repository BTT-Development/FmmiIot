# FmmiIot

### Introdution

This device can read temperature and humidity. It is connected to the internet via wifi where it can send data that it reads. There is a clock in it that uses Danish time to register the time when data is read. It is connected to an mqtt broker and if it loses connection to the broker it saves the data on the sd card and then sends it out when it receives connection to the broker again.

### Esp32-s3 WROOM

![Pinout](/docs/Images/Esp32-Pinout.png)

[More documation](https://docs.freenove.com/projects/fnk0085/en/latest/)
### External hardware

**DHT11**

**OLED DISPLAY**

**rtc 3231**

### Libraries
- [RTClib](https://docs.arduino.cc/libraries/rtclib/)
- [ArduinoJson](https://arduinojson.org/)
- [DHT sensor library](https://docs.arduino.cc/libraries/dht-sensor-library/)
- [Adafruit GFX Library](https://docs.arduino.cc/libraries/adafruit-gfx-library/)
- [Adafruit SSD1306](https://docs.arduino.cc/libraries/adafruit-ssd1306/)
- [PubSubClient](https://docs.arduino.cc/libraries/pubsubclient/)

### MQTT

|  Topic |  Description |  Pub/sub |  
|---|---|---|
|device/esp32/data/temperature|Send temperature as json in timeinterval   | Pub  |
|device/esp32/data/humidity|Send humidity as json in timeinterval   |Pub   |
|device/esp32/realtime/temperature|Send temperature as json in realtime   |Pub   |
|device/esp32/realtime/humidity|Send humidity as json in realtime   |Pub   |
|device/esp32/status|Last will msg   |Pub   |
|device/esp32/dht11/error|Send error for DHT11   |Pub   |
|device/esp32/settings|settings for device    |Sub   |

**Example on jsonfile for data**
```
{
  "ID": 1,
  "Data": 11.5,
  "Date": "Wednesday, 2025-10-15 10:17:44",
  "DataTypeId": 1
}
```
**Example on jsonfile for settings**
```
{
    Timeinterval: 10000,
    RealTimeinterval: 1000
}
```
**DataTypeId**

Datatypeid is for creating a relation in the database when the json file is sent.

- DateTypeId 1 is temperature
- DateTypeId 2 is humidity

