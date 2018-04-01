# ESP-PROG  (ESP8266 flasher)
This is not an EventGhost plugin !!!

## Preface
Modules such as **NodeMCU V1.0** or **WeMos D1 mini** are great in the development phase and in many cases even as the ultimate solution. Sometimes, however, it is better to use only a "bare" module, such as ESP-12F.
In this case, it is necessary to resolve how we will upload the firmware to the module (which is built into the new device). One of the possible solutions is described in this article.

## Initial requirements
When I started with the development, I gave myself a simple input condition: flashing should be completely automatic (no push buttons). *The module with the connected programmer should behave the same way as NodeMCU (or WeMos D1 mini).*

## Solution
So I studied the NodeMCU V1.0 circuit diagram and I found it quite simple. It was obvious that I will need some USB converter that also has the RTS signal in addition to the DTR signal. Fortunately, it was not hard to find the right type. Moreover, it is quite cheap (I found a seller with a price just a little over one dollar).  
<img src="https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-PROG/Images/CP2102-top.jpg" alt="Top view" width="25%">  
<img src="https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-PROG/Images/CP2102-bottom.jpg" alt="Bottom view" width="25%">

### Flasher schematic:
<img src="https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-PROG/Eagle/esp-prog-SMD-SCH.png" alt="Schematic" width="50%">

### Test version:
<img src="https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-PROG/Images/test.jpg" alt="Test" width="50%">

### Final verion with USB converter:
<img src="https://github.com/Pako2/EventGhostPlugins/raw/master/ESP-PROG/Images/flasher.jpg" alt="Flasher" width="50%">

## Conclusion
The finished programmer works as expected.  
In the Arduino environment, choose the **NodeMCU 1.0 (ESP-12E Module)** board.  
It is also possible to choose the highest speed **921600** !  
