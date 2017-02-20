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
