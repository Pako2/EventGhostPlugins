# ESP-SENSOR - Temperaure & humidity sensor and remote-controlled electrical socket
This project is basically the same as the [ESP-SOCKET](https://github.com/Pako2/EventGhostPlugins/tree/master/ESP-SOCKET) project. The difference is only that a temperature and humidity sensor HTU-21D has been added.
Therefore, the original text [README.md](https://github.com/Pako2/EventGhostPlugins/tree/master/ESP-SOCKET/README.md) is fully valid also for this project (only the word "SENSOR" must be used everywhere instead of "SOCKET").

## Initial requirements
I realized that the potential of ESP8266 is very little used in the [ESP-SOCKET](https://github.com/Pako2/EventGhostPlugins/tree/master/ESP-SOCKET) project and that it should be very easy to use it more.
As a first possibility of improvement I was thinking of adding some sensor. I had the HTU-21D sensor at home, so it was the simplest thing for me to add the HTU-21D sensor.
Eventghost plugin should allow to set any number of significant levels.
If any of the levels are exceeded (and / or the value drops below), the corresponding event should be triggered.
A change of relay state must trigger an event in EventGhost too.

![OFF](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SENSOR/Arduino/demo/OFF_1184x630.png)  
![ON](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SENSOR/Arduino/demo/ON_1184x630.png)  
![Plugin dialog](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SENSOR/Arduino/demo/EG_Plugin.png)  
![Events](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SENSOR/Arduino/demo/EG_Events.png)  
![Dashboard ON](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SENSOR/Arduino/demo/EG_ON.png)  
![Dashboard OFF](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SENSOR/Arduino/demo/EG_OFF.png)

### What You Will Need 
### Hardware
An ESP-12 module with at least 32Mbit Flash (equals to 4MBytes)
A HTU-21D module.

### Software
In contrast to the [ESP-SOCKET](https://github.com/Pako2/EventGhostPlugins/tree/master/ESP-SOCKET) project, one additional library was needed here:
* [HTU21D](https://github.com/enjoyneering/)  - an Arduino library for SHT21, HTU21D & Si70xx written by enjoyneering79

## How the development went
As can be seen from the pictures, this time I worked in two steps:: ![Development process](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SENSOR/Arduino/demo/development_process.png)
1) The breadboard construction (I just added the HTU-21D module to a previously prepared breadboard)
2) Newly designed printed circuit board
![Top](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SENSOR/Arduino/demo/top.png)
![Bottom](https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-SENSOR/Arduino/demo/bottom.png)
