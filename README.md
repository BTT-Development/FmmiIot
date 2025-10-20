# FmmiIot

### Introdution

This device can read temperature and humidity. It is connected to the internet via wifi where it can send data that it reads. There is a clock in it that uses UTC+1 as standart and UTC+2 as daylight saving time to register the time when data is read. It is connected to an mqtt broker and if it loses connection to the broker it saves the data on the sd card and then sends it out when it receives connection to the broker again.

### Esp32-s3 WROOM

![Pinout](/docs/Images/Esp32-Pinout.png)

[More documation](https://docs.freenove.com/projects/fnk0085/en/latest/)
### Pin connection

#### [DHT11](https://ebits.dk/products/temperatur-og-fugtighedsmaler-dht11?variant=37535294947494&country=DK&currency=DKK&utm_medium=product_sync&utm_source=google&utm_content=sag_organic&utm_campaign=sag_organic&gad_source=1&gad_campaignid=22147854649&gbraid=0AAAAAohruY3tMekSc3gYaUIvvKqP2uyz5&gclid=CjwKCAjwmNLHBhA4EiwA3ts3mTiPgkO3jFwyt5THXcZOz_EUk1SwqlfzGmhldGVdFjsF_B5JK6sosBoCAW4QAvD_BwE)

- VVC to VVC 3.3V
- GND to GND
- Signal to pin 48

#### [OLED DISPLAY](https://ardustore.dk/produkt/display-oled-0-96%E2%80%B3-i2c-128x64-module-hvid?gad_source=1&gad_campaignid=21613294775&gbraid=0AAAAAo4di3JYrA-S0BUgA_rib8UlQD3NB&gclid=CjwKCAjwmNLHBhA4EiwA3ts3mfcl3Okdb2zN0NU21foVy_hMQcU66zsDLWxqz2SGFc3zjzoU9w5AORoCVtwQAvD_BwE)

- VCC to 3.3V
- GND to GND
- SCK to pin 14
- SDA to pin 13

#### [rtc 3231](https://ardustore.dk/produkt/ds3231-rtc-clock-i2c-zs-042-module?gad_source=1&gad_campaignid=21613294775&gbraid=0AAAAAo4di3JYrA-S0BUgA_rib8UlQD3NB&gclid=CjwKCAjwmNLHBhA4EiwA3ts3mb1n_jBsjDmh0RIahdC-VWsoCltn_z-wOSIlWQVgUn9qTSlDL5ZS9xoCCUYQAvD_BwE)

- VCC to 3.3V
- GND to GND
- SCL to pin 20
- SDA to pin 19
### Libraries
- [RTClib Version: 2.1.4](https://docs.arduino.cc/libraries/rtclib/)
- [ArduinoJson Version: 7.4.2](https://arduinojson.org/)
- [DHT sensor library Version: 1.4.6](https://docs.arduino.cc/libraries/dht-sensor-library/)
- [Adafruit GFX Library Version: 1.12.3](https://docs.arduino.cc/libraries/adafruit-gfx-library/)
- [Adafruit SSD1306 Version: 2.5.15](https://docs.arduino.cc/libraries/adafruit-ssd1306/)
- [PubSubClient Version: 2.8](https://docs.arduino.cc/libraries/pubsubclient/)


## Getting startet

Install [Visual studio code](https://code.visualstudio.com/download) and when its intallt go to extensions and download [Platformio](https://docs.platformio.org/en/latest/integration/ide/vscode.html#quick-start).

#### Konfiguration
Set up konfiguration in the arduino_secret.h. 

arduino_secret.h
- Wifi SSID and password.
- Mqtt settings
- mqtt topics
- CERTIFICATE for mqtt

Go to the mqtt broker and setup device as pubisher and subricer.

Connect the device to pc and uplaod Ctrl+Alt+U and wait for device to connect to wifi and broker.

## MQTT

|  Topic |  Description |  Pub/sub |  
|---|---|---|
|device/{DeviceId}/data/temperature|Send temperature as json in timeinterval   | Pub  |
|device/{DeviceId}/data/humidity|Send humidity as json in timeinterval   |Pub   |
|device/{DeviceId}/realtime/temperature|Send temperature as json in realtime   |Pub   |
|device/{DeviceId}/realtime/humidity|Send humidity as json in realtime   |Pub   |
|device/{DeviceId}/alarm/status|Last will msg   |Pub   |
|device/{DeviceId}/alarm/DHT11|Send error for DHT11   |Pub   |
|device/{DeviceId}/settings|settings for device    |Sub   |

**Example on jsonfile for data**
```
{
  "DeviceID": 1,
  "Data": 11.5,
  "Date": "Wednesday, 2025-10-15 10:17:44",
  "DataTypeID": 1
}
```
**Example on jsonfile for settings**
```
{
    DeviceId: 1,
    Timeinterval: 10000,
    RealTimeinterval: 1000
}
```
**DataTypeId**

Datatypeid is for creating a relation in the database when the json file is sent.

- DateTypeId 1 is temperature
- DateTypeId 2 is humidity

TimeInterval 

Default settings
- Timeinterval: 10000
- RealTimeinterval: 1000