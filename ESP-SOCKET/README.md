# ESP-SOCKET - Remote-controlled electrical socket
The project assumes the use of ESP-12 module either as **NodeMcu 1.0** or **WeMos D1 mini**.

## Preface
This project is based on my project [ESP-IO-BBT](https://github.com/Pako2/EventGhostPlugins/tree/master/ESP-IO-BBT).
*So, here we can say too:*
The device may be in a completely different network than EventGhost.
Only Internet connectivity at both ends is a condition.
No public IP or VPN or port redirection is required.
**This is possible thanks to the [Beebotte](https://beebotte.com) service.**

## Initial requirements
The basic requirement was a smooth collaboration with EventGhost. This means that the state of the output must be able to be controlled by EventGhost actions, changing the state must trigger an EventGhost event, and it must be possible to query the state of output.
With a web interface, it must be possible the entire device to configure and as well as to control.

![Login page](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SOCKET/Arduino/demo/Index_htm-1184x226.png)
![Control page A](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SOCKET/Arduino/demo/Control_htm_A-1184x520.png)
![Control page B](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SOCKET/Arduino/demo/Control_htm_B-1184x520.png)
![Settings page](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SOCKET/Arduino/demo/Settings_htm-1184x1476.png)
![Device status pop-up](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SOCKET/Arduino/demo/DeviceStatus-1184x503.png)
![Beebotte channel](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SOCKET/Arduino/demo/BeebotteChannel.png)
![Plugin dialog A](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SOCKET/Arduino/demo/Plugin_A.png)
![Plugin dialog B](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SOCKET/Arduino/demo/Plugin_B.png)
![Action dialog](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SOCKET/Arduino/demo/Action.png)

## Features
* Using WebSocket protocol to exchange data between Hardware and EventGhost or Web Browser
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

**You also need to upload web files to your ESP with ESP8266FS Uploader.**

* [ESP8266FS Uploader](https://github.com/esp8266/arduino-esp8266fs-plugin) - Arduino ESP8266 filesystem uploader

Unlisted libraries are part of ESP8266 Core for Arduino IDE, so you don't need to download them.

### Steps
* First, flash firmware 
* Flash webfiles data to SPIFFS either using ESP8266FS Uploader tool or with your favourite flash tool 
* (optional) Fire up your serial monitor to get informed
* Power on your ESP
* Search for Wireless Network "ESP-SOCKET" and connect to it (It should be an open network and does not reqiure password)
* Open your browser and type either "http://192.168.4.1" or "http://esp-socket.local" (.local needs Bonjour installed on your computer) on address bar.
* Log on to ESP, default password is "admin"
* Go to "Settings" page
* Configure your amazing access control device. Push "Scan" button to join your wireless network, configure the necessary parameters.
* Save settings, when rebooted your ESP will try to join your wireless network.
* Check your new IP address from serial monitor and connect to your ESP again. (You can also connect to "http://esp-socket.local")
* Congratulations, everything went well, if you encounter any issue feel free to ask help on GitHub.
