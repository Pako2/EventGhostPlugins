/*-----( Import needed libraries )-----*/
#include "DigiUSB.h"
#include "TinyWireM.h"         // Comes with Digispark
#include <LiquidCrystal_I2C.h> // Comes with Digispark
#include "EEPROM.h"

/*-----( Declare Constants )-----*/
#define I2C_ADDR     0x27
#define ROWS         4
#define COLS         20
#define STX          2
#define ETX          3
#define ENQ          5
#define MAX         56

/*-----( Declare objects )-----*/
LiquidCrystal_I2C lcd(I2C_ADDR, COLS, ROWS);

/*-----( Declare Global Variables )-----*/
unsigned char recChars[72];
unsigned char charDef[8];
uint8_t ix;
uint8_t cmd;
uint8_t lngth;


void setup()
{
  TinyWireM.begin();   // Digispark - initialize I2C lib - comment this out to use with standard arduinos
  lcd.init();          // initialize the lcd
  DigiUSB.begin();
  initCGRAM();
  lcd.backlight();     // finish with backlight on
  lcd.setCursor(1, 0); // Start at character 2 on line 1
  lcd.print(F("www.eventghost.net"));
}

void initCGRAM()
{
  uint16_t addr = MAX * 8;
  for (uint8_t j = 0; j < 8; j++)
  {
    uint8_t m =  EEPROM.read(addr);
    if (m != 0 && m != 255)
    {
      uint8_t k = j * 8;
      for (uint8_t i = 0; i < 8; i++)
      {
        charDef[i] = EEPROM.read(k);
        k ++;
      }
      lcd.createChar(j, charDef);
    }
    addr++;
  }
}

void clearLine()
{ lcd.setCursor(0, cmd);
  for (uint8_t i = 0; i < COLS; i++)
  {
    lcd.write(32);
  }
}

void processing()
{
  if (cmd < 4) // output to display
  {
    //lcd.write(77);
    //lcd.write(78);
    if (lngth <= COLS) // normal display
    {
      clearLine();
      lcd.setCursor(0, cmd);
      for (uint8_t i = 0; i < lngth; i++)
      {
        lcd.write(recChars[i]);
      }
    }
    else // autoscroll
    {
      recChars[ix] = ' ';
      ix += 1;
      recChars[ix] = ' ';
      ix += 1;
      recChars[ix] = ' ';
      ix += 1;
      recChars[ix] = ' ';
      ix += 1;
      for (uint8_t i = 0; i < ix + 1; i++)
      {
        lcd.setCursor(0, cmd);
        for (uint8_t j = 0; j < COLS; j++)
        {
          uint8_t n = i + j;
          if (n >= ix)
          {
            n = n - ix;
          }
          lcd.write(recChars[n]);
        }
        DigiUSB.delay(200);
      }
    }
  }
  else if (cmd > 15) // customized character
  {
    lcd.createChar(cmd, charDef);
  }
}

void loop()
{
  uint8_t state = 0;
  while (true)
  {
    if (DigiUSB.available())
    {
      uint8_t lastRead = DigiUSB.read();
      if (state == 0)// waiting for STX or ENQ
      {
        if (lastRead == STX)
        {
          state = 1;
          DigiUSB.write("B"); // Busy flag
        }
        else if (lastRead == ENQ) // Enquiry
        {
          DigiUSB.write(STX);
          DigiUSB.write(ROWS);
          DigiUSB.write(COLS);
          uint16_t addr = MAX * 8 + MAX;
          uint8_t count =  EEPROM.read(addr);
          DigiUSB.write(count);
          DigiUSB.write(ETX);
        }
      }
      else if (state == 1) //command
      {
        if (lastRead > 3 && lastRead < 13)// short comands
        {
          if (lastRead == 4)
          {
            lcd.backlight();
          }
          else if (lastRead == 5)
          {
            lcd.noBacklight();
          }
          else if (lastRead == 6)
          {
            lcd.clear();
          }
          else if (lastRead == 7)
          {
            lcd.home();
          }
          else if (lastRead == 8)
          {
            lcd.display();
          }
          else if (lastRead == 9)
          {
            lcd.noDisplay();
          }
          else if (lastRead == 10)
          {
            if (cmd < 4)
            {
              if (lngth > COLS) {
                ix -= 4;
              }
              processing();
            }
          }
          else if (lastRead == 11)
          {
            initCGRAM();
          }
          state = 0;
          DigiUSB.write("F"); // Clear Busy flag
        }
        else if (lastRead < 4 || lastRead > 12)
        {
          state = 2;
          cmd = lastRead;
        }
      }
      else if (state == 2) //length
      {
        if (lastRead == 0 && cmd < 4) //clear line
        {
          clearLine();
          state = 0;
          DigiUSB.write("F"); // Clear Busy flag
        }
        else
        {
          lngth = lastRead;
          ix = 0;
          state = 3;
          //lcd.setCursor(0, 1);
        }
      }
      else if (state == 3) // data
      {
        if (cmd > 15)
        {
          charDef[ix] = lastRead;
        }
        else
        {
          recChars[ix] = lastRead;
        }
        ix += 1;


        //lcd.write(60);
        //lcd.write(ix + 48);
        //lcd.write(62);

        if (ix == lngth) // all data received -> processing
        {
          if (cmd < 5 || cmd > 15)
          {
            processing();
          }
          else if (cmd == 13) // save user char. to EEPROM
          {
            {
              if (ix == 10)
              {
                uint16_t addr = recChars[8] * 8;
                for (uint8_t i = 0; i < 8; i++)
                {
                  EEPROM.write(addr, recChars[i]);
                  addr += 1;
                }
                uint16_t addr2 = MAX * 8 + recChars[8];
                EEPROM.write(addr2, recChars[9]);
              }
              else if (ix == 1) //user chars count
              {
                uint16_t addr = MAX * 8 + MAX;
                EEPROM.write(addr, recChars[0]);
              }
            }
          }
          else if (cmd == 14) // load user char (from EEPROM to CGRAM)
          {
            if (ix % 2 == 0)  // check for even nubers
            {
              for (uint8_t k = 0; k < ix; k += 2)
              {
                if (recChars[k] < MAX && recChars[k + 1] < 8)
                {
                  uint8_t j = recChars[k] * 8;
                  for (uint8_t i = 0; i < 8; i++)
                  {
                    charDef[i] = EEPROM.read(j);
                    j++;
                  }
                  lcd.createChar(recChars[k + 1], charDef);
                }
              }
            }
          }
          state = 0;
          DigiUSB.write("F"); // Clear Busy flag
        }
      }
      DigiUSB.refresh(); // ToDo : test if disabled
    }
    DigiUSB.delay(10);
  }
}
