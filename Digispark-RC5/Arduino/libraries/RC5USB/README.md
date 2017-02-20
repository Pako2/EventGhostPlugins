Arduino RC5USB remote control decoder library
=============================================

This is an Arduino/Digispark library for decoding infrared remote control commands encoded
with the Philips RC5 protocol.
This is only slightly modified the original RC5 library for cooperation with DigiUSB.

For more information see original library at https://github.com/guyc/RC5
and http://digistump.com/wiki/digispark/tutorials/digiusb

Using the Library
-----------------

```C++

#include <DigiUSB.h>
#include <RC5USB.h>

int IR_PIN = 0;
int LED_PIN = 1;
RC5 rc5(IR_PIN);

void setup() {
  DigiUSB.begin();
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  char out_str[7];
  unsigned char toggle;
  unsigned char address;
  unsigned char command;
  while (1) {
    if (rc5.read(&toggle, &address, &command))
    {
      sprintf(out_str, "%01X.%02X.%02X", toggle, address, command);
      DigiUSB.println(out_str);
      digitalWrite(LED_PIN, HIGH);
      DigiUSB.delay(30);
      digitalWrite(LED_PIN, LOW);
    }
  }
}
