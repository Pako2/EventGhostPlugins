# ESP-OLED - Remote display for EventGhost
The project assumes the use of ESP-12 module either as **NodeMcu 1.0** or **WeMos D1 mini**.

## Preface
This project is based on my project [ESP-IO-BBT](https://github.com/Pako2/EventGhostPlugins/tree/master/ESP-IO-BBT).
*So, here too we can say:*
The device may be in a completely different network than EventGhost.
Only Internet connectivity at both ends is a condition.
No public IP or VPN or port redirection is required.
**This is possible thanks to the [Beebotte](https://beebotte.com) service.**

## Initial requirements
The basic requirement was a smooth collaboration with EventGhost. This means that the content of the display can be changed using the EventGhost action.
With a web interface, it must be possible the entire device to configure.

![Login page](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-OLED/Arduino/demo/Index_htm-1192x220.jpg)
![Settings page](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-OLED/Arduino/demo/Settings_htm-1192x1665.jpg)
![Device status pop-up](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-OLED/Arduino/demo/DeviceStatus-1192x498.jpg)
![Beebotte channel](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-OLED/Arduino/demo/BeebotteChannel.jpg)
![Plugin dialog](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-OLED/Arduino/demo/PluginDialog.jpg)
![Action dialog](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-OLED/Arduino/demo/SendMessageDialog.jpg)
![Eventghost log](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-OLED/Arduino/demo/EventGhostLog.jpg)

## Features
* Max. message length: 300 characters
* Possibility to repeat the last and also the last but one message
* Three buttons to invoke event in EventGhost
* Optional playback of a sound when connecting to a wifi and when a new message is received
* Using WebSocket protocol to exchange data between Hardware and Web Browser
* Using Beebotte/MQTT protocol to exchange data between Hardware and EventGhost
* Data is encoded as JSON object
* Bootstrap for beautiful Web Pages for both Mobile and Desktop Screens
* Thanks to ESPAsyncWebServer Library communication is asynchronous

## Getting Started
This project still in its development phase.
Latest development version is 0.0.1

### What You Will Need 
### Hardware
A development board like **WeMos D1 mini** or **NodeMcu 1.0** with at least 32Mbit Flash (equals to 4MBytes).

### Software

#### Building From Source
Please install Arduino IDE if you didn't already, then add ESP8266 Core (**Beware! [Install Git Version](https://github.com/esp8266/Arduino#using-git-version)**) on top of it. Additional Library download links are listed below:

* [Arduino IDE](http://www.arduino.cc) - The development IDE
* [ESP8266 Core for Arduino IDE](https://github.com/esp8266/Arduino) - ESP8266 Core
* [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) - Asyncrone Web Server with WebSocket Plug-in
* [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) - Mandatory for ESPAsyncWebServer
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson) - JSON Library for Arduino IDE
* [NTPClientLib](https://github.com/gmag11/NtpClient/) - NTP Client Library for Arduino IDE
* [TimeLib](https://github.com/PaulStoffregen/Time) - Mandatory for NTP Client Library
* [PubSubClient](https://github.com/knolleary/pubsubclient) - A client library that provides support for MQTT
* [brzo_i2c](https://github.com/pasko-zh/brzo_i2c) - A fast I2C Implementation written in Assembly for the esp8266
* [SSD1306Brzo](https://github.com/squix78/esp8266-oled-ssd1306/blob/master/SSD1306Brzo.h) - Driver for the SSD1306 and SH1106 based 128x64 pixel OLED display running on the Arduino/ESP8266 platform

**You also need to upload web files to your ESP with ESP8266FS Uploader.**

* [ESP8266FS Uploader](https://github.com/esp8266/arduino-esp8266fs-plugin) - Arduino ESP8266 filesystem uploader

Unlisted libraries are part of ESP8266 Core for Arduino IDE, so you don't need to download them.

### Steps
* First, flash firmware 
* Flash webfiles data to SPIFFS either using ESP8266FS Uploader tool or with your favourite flash tool 
* (optional) Fire up your serial monitor to get informed
* Power on your ESP
* Search for Wireless Network "ESP-OLED" and connect to it (It should be an open network and does not reqiure password)
* Open your browser and type either "http://192.168.4.1" or "http://esp-oled.local" (.local needs Bonjour installed on your computer) on address bar.
* Log on to ESP, default password is "admin"
* Go to "Settings" page
* Configure your amazing access control device. Push "Scan" button to join your wireless network, configure inputs and outputs.
* Save settings, when rebooted your ESP will try to join your wireless network.
* Check your new IP address from serial monitor and connect to your ESP again. (You can also connect to "http://esp-oled.local")
* Congratulations, everything went well, if you encounter any issue feel free to ask help on GitHub.
