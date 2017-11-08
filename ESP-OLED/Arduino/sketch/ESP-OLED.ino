/*
  Copyright (c) 2017 Lubos Ruckl

  Released to Public Domain
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  ############################################################
  #    Based on the project "esp-rfid" by Omer Siar Baysal   #
  #    https://github.com/omersiar/esp-rfid                  #
  ############################################################

*/
#include <base64.h>
#include <ESP8266WiFi.h>              // Whole thing is about using Wi-Fi networks
#include <ESP8266mDNS.h>              // Zero-config Library (Bonjour, Avahi) http://esp-oled.local
#include <ArduinoJson.h>              // JSON Library for Encoding and Parsing Json object to send browser. We do that because Javascript has built-in JSON parsing.
#include <FS.h>                       // SPIFFS Library for storing web files to serve to web browsers
#include <ESPAsyncTCP.h>              // Async TCP Library is mandatory for Async Web Server
#include <ESPAsyncWebServer.h>        // Async Web Server with built-in WebSocket Plug-in
#include <SPIFFSEditor.h>             // This creates a web page on server which can be used to edit text based files.
#include <NtpClientLib.h>             // To timestamp of events we get Unix Time from NTP Server
#include <TimeLib.h>                  // Library for converting epochtime to a date
#include <WiFiUdp.h>                  // Library for manipulating UDP packets which is used by NTP Client to get Timestamps
#include <PubSubClient.h>             // A client library that provides support for MQTT
#include <brzo_i2c.h>                 // A fast I2C Implementation written in Assembly for the esp8266
#include "SSD1306Brzo.h"              // Driver for the SSD1306 and SH1106 based 128x64 pixel OLED display running on the Arduino/ESP8266 platform
#include "OLEDDisplayFonts.h"
#include "fonts.h"
#include "pitches.h"

#define HEARTBEAT_INTERVAL 30000
#define MAX_CLIENTS 16
#define MQTT_MAX_PACKET_SIZE 512
#define SDC D1
#define SDA D2
#define SSD_ADDRESS 0x3c
#define AVAILABLE_TONE_PINS 1
#define T1INDEX 0

// Variables for whole scope
const uint8_t buzzer_pin = D3; //Buzzer control port
const uint8_t buttons[3]    = {D5, D6, D7}; //
uint8_t sound_c = 1;
uint8_t sound_m = 1;
String filename = "/P/";
bool shouldReboot = false;
bool inAPMode = false;
bool wifiFlag = false;
int timeZone;
String ntpserver;
String adminpass;
String encodedApass;
String hstname;
uint8_t ipmode;
String address;
String gw;
String mask;
String wifiled = "";
uint8_t ntpinter;
static uint32_t last = millis() / 1000;
static uint32_t last2 = last;
static uint32_t now_;
static uint32_t heartbeatTimestamp = 0;
static uint32_t mqTimestamp = 0;
uint16_t newval;
const uint8_t pinsA[] = {A0, D0, D3, D4, D5, D6, D7, D8};
const String pinsS = "A0D0D3D4D5D6D7D8";
String channelToken;
String bbt_channel;
String bbt_cmdrsrc;
String bbt_msgrsrc;
String bbt_cmdtopic;
String bbt_msgtopic;
const char clientID[] = "ESP-OLED";     // Your client ID
const char host[] = "mqtt.beebotte.com";  // The Beebotte server
String titles[8];

// Holds the current button state.
volatile uint8_t state[3] = {1, 1, 1};
// Holds the last time debounce was evaluated (in millis).
volatile uint32_t lastDebounceTime[3] = {0, 0, 0};
// The delay threshold for debounce checking.
const uint8_t debounceDelay = 50;

typedef struct {
  String id;
  bool enabled;
  String title;
  bool out;
  bool pullup;
  bool defHIGH;
} PinData;
PinData pindata[8];

typedef enum {CL_CONN, CL_AUTH, CL_DEL, CL_UNKN} clistate;

typedef struct {
  uint32_t id;
  IPAddress ip;
  uint16_t port;
  clistate state;
} CliData;
CliData clients[MAX_CLIENTS];
uint16_t oldvals[8];

uint32_t lastredrw = 0;
//uint32_t refr = 100; //10 lines per second; the whole display in 6.4 seconds
uint32_t refr = 50;  //20 lines per second; the whole display in 3.2 seconds

const char *fontData;
char* txt;
char substr[64];
String logo = "ESP-OLED   version 1.0";
const uint8_t fontnum = 9;
const char * fnts[fontnum] =
{
  ArialMT_Plain_10,
  ArialMT_Plain_16,
  ArialMT_Plain_24,
  SansSerif_plain_13,
  SansSerif_plain_14,
  SansSerif_plain_15,
  SansSerif_plain_16,
  SansSerif_plain_17,
  SansSerif_plain_18
};
const char * fntlst[fontnum] =
{
  "ArialMT_Plain_10",
  "ArialMT_Plain_16",
  "ArialMT_Plain_24",
  "SansSerif_plain_13",
  "SansSerif_plain_14",
  "SansSerif_plain_15",
  "SansSerif_plain_16",
  "SansSerif_plain_17",
  "SansSerif_plain_18"
};

uint16_t indexes[256];
uint16_t rows = 0;
int lastPos;
uint8_t lastRow;
uint8_t fh;

SSD1306Brzo display(SSD_ADDRESS, SDA, SDC);

// Variables
WiFiClient wifiClient;
PubSubClient psclient(host, 1883, wifiClient);

struct notetype {
  bool pause;
  uint32_t half_period_ticks;
  uint16_t num;
};
typedef notetype Note;
struct songtype { Note notes[32];};
typedef songtype Song;

const uint8_t tone_timers[] = { 1 };
static uint8_t tone_pins[AVAILABLE_TONE_PINS] = { 255, };
static long toggle_counts[AVAILABLE_TONE_PINS] = { 0, };
static uint8_t noteIx[AVAILABLE_TONE_PINS] = {0};
Song song0;
Song song[AVAILABLE_TONE_PINS] = {song0};


// Create AsyncWebServer instance on port "80"
AsyncWebServer server(80);
// Create WebSocket instance on URL "/ws"
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");


// ----------------- CODE ----------------------------
static int8_t tuneBegin(uint8_t _pin) {
  int8_t _index = -1;

  // if we're already using the pin, reuse it.
  for (int i = 0; i < AVAILABLE_TONE_PINS; i++) {
    if (tone_pins[i] == _pin) {
      return i;
    }
  }

  // search for an unused timer.
  for (int i = 0; i < AVAILABLE_TONE_PINS; i++) {
    if (tone_pins[i] == 255) {
      tone_pins[i] = _pin;
      _index = i;
      break;
    }
  }
  return _index;
}


void setupTuneTimer(uint32_t half_period_ticks)
{
  timer1_disable();
  timer1_isr_init();
  timer1_attachInterrupt(tuneHandler);
  timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
  timer1_write(half_period_ticks);
}

//  tune arguments example:
//  =======================
//  uint16_t melody[8] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_GS3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
//  uint8_t noteDurations[8] = {4, 8, 8, 4, 4, 4, 4, 4};
//  note durations: 4 = quarter note, 8 = eighth note, etc.:
void tune(uint8_t _pin, uint16_t melody[16], uint8_t noteDurations[16], uint8_t len)
{
  uint32_t numerator = clockCyclesPerMicrosecond() * 500000; // = 40 000 000 for 80MHz
  int8_t _index;
  _index = tuneBegin(_pin);
  timer1_disable();
  noteIx[_index] = 0;

  if (_index >= 0 && len > 0 && len <= 16)
  {
    // Set the pinMode as OUTPUT
    pinMode(_pin, OUTPUT);
    uint8_t j = 0;
    for (uint8_t k = 0; k < len; k++)
    {
      if (melody[k] > 0) // tone
      {
        song[_index].notes[j].pause = false;
        song[_index].notes[j].half_period_ticks = numerator / melody[k];
        song[_index].notes[j].num = 2 * melody[k] / noteDurations[k];
      }
      else              // pause
      {
        song[_index].notes[j].pause = true;
        song[_index].notes[j].half_period_ticks = numerator / 100; //100Hz (half-period = 5ms)
        song[_index].notes[j].num = 2 * 100 / noteDurations[k];
      }
      j++; //short pause after tone:
      song[_index].notes[j].pause = true;
      song[_index].notes[j].half_period_ticks = numerator / 100; //100Hz (half-period = 5ms)
      song[_index].notes[j].num = 6;// =30ms/5ms
      j++;
    }
    if (j < 32) {
      song[_index].notes[j].half_period_ticks = 0;
    }
    toggle_counts[_index] = song[_index].notes[0].num;
    setupTuneTimer(song[_index].notes[0].half_period_ticks);
  }
}


void disableTuneTimer(uint8_t _index)
{
  tone_pins[_index] = 255;
  switch (tone_timers[_index])
  {
    case 0:
      // Not currently supported
      break;

    case 1:
      timer1_disable();
      break;
  }
}


ICACHE_RAM_ATTR void tuneHandler()
{
  if (toggle_counts[T1INDEX] != 0)
  {
    if (!song[T1INDEX].notes[noteIx[T1INDEX]].pause)
    {
      // toggle the pin
      digitalWrite(tone_pins[T1INDEX], toggle_counts[T1INDEX] % 2);
    }
    toggle_counts[T1INDEX]--;
  }
  else
  {
    noteIx[T1INDEX]++;
    uint8_t nIx = noteIx[T1INDEX];
    if (nIx < 32 && song[T1INDEX].notes[nIx].half_period_ticks > 0)
    {
      if (song[T1INDEX].notes[nIx].pause)
      {
        digitalWrite(tone_pins[T1INDEX], LOW);
      }
      toggle_counts[T1INDEX] = song[T1INDEX].notes[nIx].num;
      setupTuneTimer(song[T1INDEX].notes[nIx].half_period_ticks);
    }
    else
    {
      digitalWrite(tone_pins[T1INDEX], LOW);
      disableTuneTimer(T1INDEX);
    }
  }
}

// Function prototypes
void messageReceived(const String message);
void callback(char* topic, byte* payload, unsigned int length);

bool delCli(uint32_t id)
{
  for (uint8_t i = 0; i < MAX_CLIENTS; i++)
  {
    if (id == clients[i].id)
    {
      Serial.print(F("[ INFO ] WebSocket client ["));
      Serial.print(id);
      Serial.print(F("]["));
      Serial.print(printIP(clients[i].ip));
      Serial.print(F(":"));
      Serial.print(clients[i].port);
      Serial.println(F("] disconnected"));
      clients[i].state = CL_DEL;
      return true;
    }
  }
  return false;
}


bool addCli(uint32_t id, IPAddress addr, uint16_t port)
{
  for (uint8_t i = 0; i < MAX_CLIENTS; i++)
  {
    if (clients[i].state == CL_DEL)
    {
      clients[i].state = CL_CONN;
      clients[i].id = id;
      clients[i].ip = addr;
      clients[i].port = port;
      return true;
    }
  }
  return false;
}


bool getCliStateAuth(uint32_t id)
{
  for (uint8_t i = 0; i < MAX_CLIENTS; i++)
  {
    if (id == clients[i].id)
    {
      return clients[i].state == CL_AUTH;
    }
  }
  return false;
}


bool setCliStateAuth(uint32_t id)
{
  for (uint8_t i = 0; i < MAX_CLIENTS; i++)
  {
    if (id == clients[i].id)
    {
      clients[i].state = CL_AUTH;
      return true;
    }
  }
  return false;
}


// Start NTP only after IP network is connected
void onSTAGotIP(WiFiEventStationModeGotIP ipInfo)
{
  Serial.print(F("[ INFO ] Got IP: "));
  Serial.println(ipInfo.ip.toString().c_str());
  if (sound_c)
  {
    uint16_t melody[8] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_GS3, NOTE_G3, PAUSE, NOTE_B3, NOTE_C4};
    uint8_t noteDurations[8] = {4, 8, 8, 4, 4, 4, 4, 4};
    tune(buzzer_pin, melody, noteDurations, 8);
  }
  if (wifiled == "D0" || wifiled == "D4")
  {
    digitalWrite(pinNum(wifiled), LOW); // Turn on LED}
  }
  NTP.begin(ntpserver, timeZone, true);
  NTP.setInterval(ntpinter * 60); // Poll every "ntpinter" minutes
}


// Manage ssid connection
void onSTAConnected(WiFiEventStationModeConnected event_info)
{
  wifiFlag = true;
  Serial.print(F("\r\n[ INFO ] Connected to WiFi \""));
  Serial.print(event_info.ssid.c_str());
  Serial.print(F("\". Channel: "));
  Serial.println(event_info.channel);
}

uint8_t pinNum(String s)
{
  uint8_t ix = pinsS.indexOf(s);
  return pinsA[ix / 2];
}

// Manage network disconnection
void onSTADisconnected(WiFiEventStationModeDisconnected event_info)
{
  if (wifiFlag)
  {
    Serial.print(F("[ WARN ] Disconnected from WiFi \""));
    Serial.print(event_info.ssid.c_str());
    Serial.print(F("\". Reason: "));
    Serial.println(event_info.reason);

    if (wifiled == "D0" || wifiled == "D4")
    {
      digitalWrite(pinNum(wifiled), HIGH); // Turn off LED}
    }

    NTP.stop(); // NTP sync can be disabled to avoid sync errors
    wifiFlag = false;
  }
}


void processSyncEvent(NTPSyncEvent_t ntpEvent)
{
  if (ntpEvent) {
    Serial.print(F("[ WARN ] Time Sync error: "));
    if (ntpEvent == noResponse)
    {
      Serial.print(F("NTP server "));
      Serial.print(NTP.getNtpServerName());
      Serial.println(F(" not reachable"));
    }
    else if (ntpEvent == invalidAddress)
      Serial.println(F("[ WARN ] Invalid NTP server address"));
  }
  else {
    Serial.print(F("[ INFO ] Got NTP time: "));
    Serial.print(NTP.getTimeDateString(NTP.getLastNTPSync()));
    Serial.println(NTP.isSummerTime() ? "  Summer Time" : "  Winter Time");
  }
}

boolean syncEventTriggered = false; // True if a time event has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event


String printIP(IPAddress adress) {
  return (String)adress[0] + "." + (String)adress[1] + "." + (String)adress[2] + "." + (String)adress[3];
}

uint8_t utf8ascii(byte ascii) {
  static uint8_t LASTCHAR;
  if ( ascii < 128 ) { // Standard ASCII-set 0..0x7F handling
    LASTCHAR = 0;
    return ascii;
  }
  uint8_t lastchar = LASTCHAR;   // get last char
  LASTCHAR = ascii;
  switch (lastchar) {    // conversion depnding on first UTF8-character
    case 0xC2: return  (ascii);  break;
    case 0xC3: return  (ascii | 0xC0);  break;
    case 0x82: if (ascii == 0xAC) return (0x80);    // special case Euro-symbol
  }
  return  0; // otherwise: return zero, if character has to be ignored
}

char* utf8ascii(String str) {
  uint16_t k = 0;
  uint16_t length = str.length() + 1;
  // Copy the string into a char array
  char* s = (char*) malloc(length * sizeof(char));
  if (!s) {
    DEBUG_OLEDDISPLAY("[OLEDDISPLAY][utf8ascii] Can't allocate another char array. Drop support for UTF-8.\n");
    return (char*) str.c_str();
  }
  str.toCharArray(s, length);
  length--;
  for (uint16_t i = 0; i < length; i++) {
    char c = utf8ascii(s[i]);
    if (c != 0) {
      s[k++] = c;
    }
  }
  s[k] = 0;
  return s;
}

void displayString(int16_t xMove, int16_t yMove, uint16_t maxLineWidth, String strUser)
{
  strUser.replace("\r\n","\n");
  strUser.replace("\r","\n");
  
  uint16_t firstChar  = pgm_read_byte(fontData + FIRST_CHAR_POS);
  uint16_t lastDrawnPos = 0;
  uint16_t strWidth = 0;
  uint16_t preferredBreakpoint = 0;
  uint16_t widthAtBreakpoint = 0;
  txt = utf8ascii(strUser);
  uint16_t length = strlen(txt);

  rows = 0;
  for (uint16_t i = 0; i < length; i++)
  {
    strWidth += pgm_read_byte(fontData + JUMPTABLE_START + (txt[i] - firstChar) * JUMPTABLE_BYTES + JUMPTABLE_WIDTH);

    // Always try to break on a space or dash
    if (txt[i] == ' ' || txt[i] == '-' || txt[i] == '\n' || txt[i] == '\r') {
      preferredBreakpoint = i;
      widthAtBreakpoint = strWidth;
    }

    if (txt[i] == '\n' || txt[i] == '\r')
    {
      txt[i]=32;
      indexes[rows] = lastDrawnPos;
      lastDrawnPos = preferredBreakpoint + 1;
      strWidth = strWidth - widthAtBreakpoint;
      preferredBreakpoint = 0;
      rows++;
    }

    else if (strWidth >= maxLineWidth) 
    {
      if (preferredBreakpoint == 0) {
        preferredBreakpoint = i;
        widthAtBreakpoint = strWidth;
      }
      indexes[rows] = lastDrawnPos;
      lastDrawnPos = preferredBreakpoint + 1;
      strWidth = strWidth - widthAtBreakpoint;
      preferredBreakpoint = 0;
      rows++;
    }
  }

  // Draw last part if needed
  if (lastDrawnPos < length) {
    indexes[rows] = lastDrawnPos;
    rows++;
  }
  indexes[rows] = length;
  lastPos = DISPLAY_HEIGHT;
  lastRow = 0;
}

void draw(uint8_t rw, int pos)
{
  uint16_t line;
  uint8_t lng;
  display.clear();
  for (uint8_t i = 0; i * fh + pos < DISPLAY_HEIGHT; i++)
  {
    lng = indexes[rw + 1] - indexes[rw];
    strncpy(substr, txt + indexes[rw], lng);
    substr[lng] = '\0'; // place the null terminator !!!
    line = i * fh + pos;
    display.drawString(0, line, String(substr));
    rw++;
    if (rw >= rows)
    {
      break;
    }
  }
  display.display();
}


void onChange(uint8_t btn) {
  // Get the pin reading.
  int reading = digitalRead(buttons[btn]);

  // Ignore dupe readings.
  if (reading == state[btn]) return;

  boolean debounce = false;

  // Check to see if the change is within a debounce delay threshold.
  if ((millis() - lastDebounceTime[btn]) <= debounceDelay) {
    debounce = true;
  }

  // This update to the last debounce check is necessary regardless of debounce state.
  lastDebounceTime[btn] = millis();

  // Ignore reads within a debounce delay threshold.
  if (debounce) return;

  // All is good, persist the reading as the state.
  state[btn] = reading;

  // Work with the value now.
  if (reading == 0)
  {
    sendMQTTbtn(btn);
  }
}

void onChange1() {
  onChange(0);
}

void onChange2() {
  onChange(1);
}

void onChange3() {
  onChange(2);
}

void setup()
{
  pinMode (buzzer_pin, OUTPUT );
  digitalWrite(buzzer_pin, 0);
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }
  // Attach an interrupt to the pin, assign the onChange function as a handler and trigger on changes (LOW or HIGH).
  attachInterrupt(buttons[0], onChange1, CHANGE);
  attachInterrupt(buttons[1], onChange2, CHANGE);
  attachInterrupt(buttons[2], onChange3, CHANGE);

  Serial.begin(115200);
  Serial.println("\nCONSOLE OUTPUT ...");
  Serial.println(F("[ INFO ] ESP-OLED v0.0.1"));
  for (uint8_t i = 0; i < MAX_CLIENTS; i++)
  {
    clients[i].state = CL_DEL;
  }

  static WiFiEventHandler e1, e2, e3;
  e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start NTP Client
  e2 = WiFi.onStationModeDisconnected(onSTADisconnected);
  e3 = WiFi.onStationModeConnected(onSTAConnected);

  // Start SPIFFS filesystem
  SPIFFS.begin();

  // Try to load configuration file so we can connect to an Wi-Fi Access Point
  // Do not worry if no config file is present, we fall back to Access Point mode and device can be easily configured
  if (!loadConfiguration())
  {
    fallbacktoAPMode();
  }

  // attach AsyncEventSource
  server.addHandler(&events);

  events.onConnect([&encodedApass](AsyncEventSourceClient * client)
  {
    client->send(encodedApass.c_str(), NULL, millis(), 1000);
  });


  NTP.onNTPSyncEvent([](NTPSyncEvent_t event) {
    ntpEvent = event;
    syncEventTriggered = true;
  });

  // Start WebSocket Plug-in and handle incoming message on "onWsEvent" function
  server.addHandler(&ws);
  ws.onEvent(onWsEvent);

  // Configure web server
  // Serve all files in root folder
  server.serveStatic("/", SPIFFS, "/");
  // Handle what happens when requested web file couldn't be found
  server.onNotFound([](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
    response->addHeader("Location", "http://192.168.4.1");
    request->send(response);
  });

  // Simple Firmware Update Handler
  server.on("/auth/update", HTTP_POST, [](AsyncWebServerRequest * request) {
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.print(F("[ UPDT ] Firmware update started: "));
      Serial.println(filename.c_str());
      Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
        Update.printError(Serial);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true))
      {
        Serial.print(F("[ UPDT ] Firmware update finished: "));
        Serial.println(index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  fh = pgm_read_byte(fontData + HEIGHT_POS);
  lastPos = DISPLAY_HEIGHT;
  lastRow = 0;

  // Start Web Server
  server.begin();

}//setup end


bool connectSTA(const char* ssid, const char* password, byte bssid[6])
{
  //return false;
  WiFi.mode(WIFI_STA);
  if (ipmode == 0)
  {
    IPAddress ip;
    IPAddress gateway;
    IPAddress subnet;
    if (!ip.fromString(address) || !gateway.fromString(gw) || !subnet.fromString(mask))
    {
      return false;
    }
    WiFi.config(ip, gateway, subnet, gateway);
  }

  if ((String)bssid[0] == "0") {
    WiFi.begin(ssid, password, 0);
  }
  else {
    WiFi.begin(ssid, password, 0, bssid);
  }

  // Inform user we are trying to connect
  Serial.print(F("[ INFO ] Trying to connect WiFi \""));
  Serial.print(ssid);
  Serial.print(F("\""));
  Serial.println(F(" (30 s timeout)"));
  // We try it for 30 seconds and give up on if we can't connect
  unsigned long strt = millis();
  uint16_t diff;
  uint8_t timeout = 30; // define when to time out in seconds
  // Wait until we connect or 30 seconds pass
  do
  {
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
    delay(1);
    diff = millis() - strt;
    if (diff % 1000 == 0)
    {
      if (!wifiFlag)
      {
        Serial.print(F("."));
      }
    }
  }
  while (diff < timeout * 1000);
  // We now out of the while loop, either time is out or we connected. check what happened
  if (WiFi.status() == WL_CONNECTED)
  {
    return true;
  }
  else
  {
    Serial.println();
    Serial.println(F("[ WARN ] Couldn't connect in time"));
    return false;
  }
}


void parseBytes(const char* str, char sep, byte * bytes, uint8_t maxBytes, uint8_t base)
{
  for (uint8_t i = 0; i < maxBytes; i++) {
    bytes[i] = strtoul(str, NULL, base);  // Convert byte
    str = strchr(str, sep);               // Find next separator
    if (str == NULL || *str == '\0') {
      break;                              // No more separators, exit
    }
    str++;                                // Point to next character after separator
  }
}

// Fallback to AP Mode, so we can connect to ESP if there is no Internet connection
void fallbacktoAPMode() {
  inAPMode = true;
  WiFi.mode(WIFI_AP);
  Serial.print(F("[ INFO ] Configuring access point... "));
  Serial.println(WiFi.softAP("ESP-OLED") ? "Ready" : "Failed!");
  // Access Point IP
  IPAddress myIP = WiFi.softAPIP();
  Serial.print(F("[ INFO ] Access point IP address: "));
  Serial.println(myIP);
  server.serveStatic("/auth/", SPIFFS, "/auth/").setDefaultFile("settings.htm").setAuthentication("admin", "admin");
  // Add Text Editor (http://esp-oled.local/edit) to Web Server. This feature likely will be dropped on final release.
  server.addHandler(new SPIFFSEditor("admin", "admin"));
}


bool loadConfiguration()
{
  File configFile = SPIFFS.open("/auth/config.json", "r");
  if (!configFile)
  {
    Serial.println(F("[ WARN ] Failed to open config file"));
    return false;
  }
  size_t size = configFile.size();
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);
  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& cfg = jsonBuffer.parseObject(buf.get());
  if (!cfg.success())
  {
    Serial.println(F("[ WARN ] Failed to parse config file"));
    return false;
  }
  Serial.println(F("[ INFO ] Config file found"));
  cfg.prettyPrintTo(Serial);
  Serial.println();
  ntpserver              = cfg["ntpser"].as<String>();
  ntpinter               = cfg["ntpint"];
  timeZone               = cfg["tz"];
  const char * ssid      = cfg["ssid"];
  const char * password  = cfg["pswd"];
  uint8_t wmode          = cfg["wmode"];
  hstname                = cfg["hostnm"].as<String>();
  wifiled                = cfg["wled"].as<String>();
  uint8_t font           = cfg["font"];
  uint8_t contrast       = cfg["contrast"];
  sound_c                = cfg["sound_c"];
  sound_m                = cfg["sound_m"];

  fontData = fnts[font];
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(fontData);
  display.setContrast(contrast);//takes a value between 0 and 255, where 0 is the lowest and 255 the highest contrast
  displayString(0, 0, 128, logo);


  //display.setContrast(128);//takes a value between 0 and 255, where 0 is the lowest and 255 the highest contrast
  bbt_channel            = cfg["chnnl"].as<String>();
  bbt_cmdrsrc            = cfg["cmdrsrc"].as<String>();
  bbt_msgrsrc            = cfg["msgrsrc"].as<String>();
  bbt_cmdtopic           = bbt_channel + "/" + bbt_cmdrsrc;
  bbt_msgtopic           = bbt_channel + "/" + bbt_msgrsrc;
  channelToken           = cfg["tkn"].as<String>();
  address                = cfg["addr"].as<String>();
  gw                     = cfg["gw"].as<String>();
  mask                   = cfg["mask"].as<String>();
  ipmode                 = cfg["ipmode"];
  const char * bssidmac  = cfg["bssid"];
  adminpass              = cfg["apwd"].as<String>();
  encodedApass           = base64::encode(adminpass);

  if (wifiled == "D0" || wifiled == "D4")
  {
    uint8_t ix = pinsS.indexOf(wifiled) / 2;
    uint8_t wfled = pinsA[ix];
    pinMode(wfled, OUTPUT);
    digitalWrite(wfled, HIGH);
  }

  byte bssid[6];
  parseBytes(bssidmac, ':', bssid, 6, 16);

  // Set Hostname.
  WiFi.hostname(hstname);

  // Start mDNS service so we can connect to http://esp-oled.local (if Bonjour installed on Windows or Avahi on Linux)
  if (!MDNS.begin(hstname.c_str())) {
    Serial.println(F("Error setting up MDNS responder!"));
  }
  else
  {
    Serial.println(F("[ INFO ] Start mDNS service ..."));
  }

  // Add Web Server service to mDNS
  MDNS.addService("http", "tcp", 80);

  // Serve confidential files in /auth/ folder with a Basic HTTP authentication
  server.serveStatic("/auth/", SPIFFS, "/auth/").setDefaultFile("settings.htm").setAuthentication("admin", adminpass.c_str());

  // Add Text Editor (http://esp-oled.local/edit) to Web Server. This feature likely will be dropped on final release.
  server.addHandler(new SPIFFSEditor("admin", adminpass));

  if (wmode == 1)
  {
    inAPMode = true;
    Serial.println(F("[ INFO ] ESP-OLED is running in AP Mode "));
    WiFi.mode(WIFI_AP);
    Serial.print(F("[ INFO ] Configuring access point... "));
    IPAddress apIP;
    if (!apIP.fromString(address))
    {
      apIP.fromString("192.168.4.1");
    }
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    Serial.println(WiFi.softAP(ssid, password) ? "Ready" : "Failed!");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print(F("[ INFO ] Access point IP address: "));
    Serial.println(myIP);
    Serial.print(F("[ INFO ] Access point SSID: "));
    Serial.println(ssid);
    return true;
  }
  else if (!connectSTA(ssid, password, bssid))
  {
    Serial.print(F("[ WARN ] Connect to WiFi \""));
    Serial.print(ssid);
    Serial.println(F("\" failed"));
    return false;
  }
  return true;
}

// Handles WebSocket Events
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
  String remIP;
  uint16_t remPort;
  if (type == WS_EVT_CONNECT)
  {
    remIP = printIP(client->remoteIP());
    remPort = client->remotePort();
    Serial.print(F("[ INFO ] WebSocket client ["));
    Serial.print(client->id());
    Serial.print(F("]["));
    Serial.print(remIP);
    Serial.print(F(":"));
    Serial.print(remPort);
    Serial.println(F("] connected"));
    sendCommand(client, "password");
    addCli(client->id(), client->remoteIP(), remPort);
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    delCli(client->id());
  }
  else  if (type == WS_EVT_ERROR)
  {
    Serial.printf("[ WARN ] WebSocket[%s][%u] error(%u): %s\r\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      for (size_t i = 0; i < info->len; i++) {
        msg += (char) data[i];
      }
      if (msg == "ping")
      {
        sendPong(client);
        return;
      }
      // We should always get a JSON object (stringfied) from browser, so parse it
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(msg);
      if (!root.success()) {
        Serial.println(F("[ WARN ] Couldn't parse WebSocket message:"));
        Serial.println(msg);
        return;
      }
      if (root.containsKey("command"))
      {
        uint16_t res;
        uint8_t ix;
        // Client sends some commands, check which command is given
        const char * command = root["command"];
        if (strcmp(command, "password")  == 0)
        {
          String psw = root["password"].as<String>();
          if (psw == encodedApass)
          {
            setCliStateAuth(client->id());
            sendCommand(client, "authorized");
            Serial.print(F("[ INFO ] Websocket client ["));
            Serial.print(client->id());
            Serial.println(F("] authorized !"));
          }
          else
          {
            Serial.print(F("[ WARN ] Unauthorized client ["));
            Serial.print(client->id());
            Serial.println(F("] forcibly disconnected !"));
            client->close();
          }
        }
        else if (!getCliStateAuth(client->id()))
        {
          Serial.print(F("[ WARN ] Unauthorized client ["));
          Serial.print(client->id());
          Serial.println(F("] forcibly disconnected !"));
          client->close();
        }
        else
        {
          // Check whatever the command is and act accordingly
          if (strcmp(command, "remove")  == 0) {
            const char* uid = root["uid"];
            filename = "/P/";
            filename += uid;
            SPIFFS.remove(filename);
          }
          else if (strcmp(command, "configfile")  == 0) {
            //Serial.print("configfile received ...");
            //ToDo: Only save the file after checking !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            File f = SPIFFS.open("/auth/config.json", "w+");
            if (f) {
              root.prettyPrintTo(f);
              //f.print(msg);
              f.close();
              ESP.reset();
            }
          }
          else if (strcmp(command, "fontlist")  == 0) {
            sendFontList(client);
          }
          else if (strcmp(command, "status")  == 0) {
            sendStatus();
          }
          else if (strcmp(command, "scan")  == 0) {
            WiFi.scanNetworksAsync(printScanResult);
          }
          else if (strcmp(command, "gettime")  == 0) {
            sendTime();
          }
          else if (strcmp(command, "settime")  == 0) {
            unsigned long t = root["epoch"];
            setTime(t);
            sendTime();
          }
          else if (strcmp(command, "getconf")  == 0)
          {
            File configFile = SPIFFS.open("/auth/config.json", "r");
            if (configFile) {
              size_t len = configFile.size();
              AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
              if (buffer) {
                configFile.readBytes((char *)buffer->get(), len + 1);
                ws.textAll(buffer);
              }
              configFile.close();
            }
          }
          else
          {
            Serial.print(F("[ INFO ] Got unknown command: \""));
            Serial.print(command);
            Serial.println(F("\""));
          }
        }
      }//end of:  if root.containsKey("command")
    }
  }
}

void sendCommand(AsyncWebSocketClient * client, String cmd) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = cmd;
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    if (client) {
      client->text(buffer);
    }
  }
}


void sendPong(AsyncWebSocketClient * client) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  //root["method"] = hstname + ".Pong";
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    if (client) {
      client->text(buffer);
    }
  }
}


void sendStatus()
{
  struct ip_info info;
  FSInfo fsinfo;
  if (!SPIFFS.info(fsinfo)) {
    Serial.print(F("[ WARN ] Error getting info on SPIFFS"));
  }
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "status";
  root["heap"] = ESP.getFreeHeap();
  root["chipid"] = String(ESP.getChipId(), HEX);
  root["cpu"] = ESP.getCpuFreqMHz();
  root["availsize"] = ESP.getFreeSketchSpace();
  root["availspiffs"] = fsinfo.totalBytes - fsinfo.usedBytes;
  root["spiffssize"] = fsinfo.totalBytes;
  if (inAPMode) {
    wifi_get_ip_info(SOFTAP_IF, &info);
    struct softap_config conf;
    wifi_softap_get_config(&conf);
    root["ssid"] = String(reinterpret_cast<char*>(conf.ssid));
    root["dns"] = printIP(WiFi.softAPIP());
    root["mac"] = WiFi.softAPmacAddress();
  }
  else {
    wifi_get_ip_info(STATION_IF, &info);
    struct station_config conf;
    wifi_station_get_config(&conf);
    root["ssid"] = String(reinterpret_cast<char*>(conf.ssid));
    root["dns"] = printIP(WiFi.dnsIP());
    root["mac"] = WiFi.macAddress();
  }
  IPAddress ipaddr = IPAddress(info.ip.addr);
  IPAddress gwaddr = IPAddress(info.gw.addr);
  IPAddress nmaddr = IPAddress(info.netmask.addr);
  root["ip"] = printIP(ipaddr);
  root["gateway"] = printIP(gwaddr);
  root["netmask"] = printIP(nmaddr);
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    ws.textAll(buffer);
  }
}


// Send Scanned SSIDs to websocket clients as JSON object
void printScanResult(uint8_t networksFound)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "ssidlist";
  JsonArray& data2 = root.createNestedArray("bssid");
  JsonArray& data = root.createNestedArray("ssid");
  for (uint8_t i = 0; i < networksFound; ++i) {
    // Print SSID for each network found
    data.add(WiFi.SSID(i));
    data2.add(WiFi.BSSIDstr(i));
  }
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    ws.textAll(buffer);
  }
  WiFi.scanDelete();
}


void sendTime() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "gettime";
  //root["method"] = hstname + ".GetTime";
  root["epoch"] = now();
  root["timezone"] = timeZone;
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    ws.textAll(buffer);
  }
}


void sendAnswer(AsyncWebSocketClient * client, String id, uint16_t val, String token) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "pinstate";
  root["id"] = id;
  root["value"] = val;
  root["token"] = token;
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    if (client)
    {
      client->text(buffer);
    }
  }
}


void broadcastHeartbeatMessage()
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "nop";
  root["epoch"] = now();
  root["timezone"] = timeZone;
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    ws.textAll(buffer);
  }
  if (psclient.connected())
  {
    root.remove("command");
    root["message"] = "nop";
    publish(root);
  }
}


// publishes data to the specified resource
//void publish(const char* resource, float data, bool persist)
void publish(JsonObject &data)
{
  DynamicJsonBuffer jsonOutBuffer;
  JsonObject& root = jsonOutBuffer.createObject();
  root["channel"] = bbt_channel;
  root["resource"] = bbt_msgrsrc;
  root["data"] = data;

  // Now print the JSON into a char buffer
  char buffer[MQTT_MAX_PACKET_SIZE];
  root.printTo(buffer, sizeof(buffer));
  // Now publish the char buffer to Beebotte
  psclient.publish(bbt_msgtopic.c_str(), buffer);
}

// Handle a received message
void callback(char* topic, byte* payload, unsigned int length)
{
  uint16_t rslt;
  uint8_t ix;

  // A buffer to receive a message
  char buffer[MQTT_MAX_PACKET_SIZE];
  snprintf(buffer, sizeof(buffer), "%s", payload);
  buffer[length] = 0; // place null-terminal !!!
  String cleanBuffer = String(buffer);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(cleanBuffer);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  JsonObject& data_ = root["data"];
  if (data_.success()) {
    if (data_.containsKey("command"))
    {
      String command = data_["command"].as<String>();
      if (command == "getconf")
      {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& data = jsonBuffer.createObject();
        data["message"]  = "config";
        //data["wled"]     = wifiled;
        publish(data);
      }
      else if (command == "msg")
      {
        if (sound_m)
        {
          uint16_t melody[3] = {NOTE_C4, NOTE_E4, NOTE_G4};
          uint8_t noteDurations[3] = {8, 8, 8};
          // note durations: 4 = quarter note, 8 = eighth note, etc.:
          tune(buzzer_pin, melody, noteDurations, 3);
        }
        Serial.print(F("[ INFO ] Got message command: "));
        String mssg = data_["msg"].as<String>();
        displayString(0, 0, 128, mssg);
        Serial.println(mssg);
      }
    }
  }
}


void sendMQTTanswer(String id, uint16_t val, String token) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& data = jsonBuffer.createObject();
  data["message"] = "pinstate";
  data["id"] = id;
  data["value"] = val;
  data["token"] = token;
  publish(data);
}


void sendMQTTbtn(uint8_t btn) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& data = jsonBuffer.createObject();
  data["message"] = "button";
  data["button"] = btn;
  publish(data);
}


void sendFontList(AsyncWebSocketClient * client) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "fontlist";
  JsonArray& fonts = root.createNestedArray("fonts");
  for (uint8_t i = 0; i < fontnum; i++)
  {
    fonts.add(fntlst[i]);
  }
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    if (client) {
      client->text(buffer);
    }
  }
}


void loop()
{
  // check for a new update and restart
  if (shouldReboot) {
    Serial.println(F("[ UPDT ] Rebooting..."));
    delay(100);
    ESP.restart();
  }

  uint32_t now_ = millis();
  //display redraw
  if (now_ - lastredrw > refr)
  {
    lastredrw = now_;
    if (!(lastPos == 0 && lastRow == 0 && rows * fh <= DISPLAY_HEIGHT))
      //if (false)
    {
      lastPos--;
      //lastPos -= 2;
      //if (lastPos + fh = 0)
      if (lastPos + fh <= 0)
      {
        //lastPos = 0;
        lastPos = lastPos + fh;
        lastRow++;
        if (lastRow >= rows - 1)
        {
          lastRow = 0;
          lastPos = DISPLAY_HEIGHT;
        }
      }
      draw(lastRow, lastPos);
    }
  } //display redraw end

  if ((now_ - 1000 * last) > 60000)
  {
    NTP.getTimeDateString();
    last += 60;
  }

  if ((now_ - heartbeatTimestamp) > HEARTBEAT_INTERVAL)
  {
    heartbeatTimestamp = now_;
    broadcastHeartbeatMessage();
  }

  if (syncEventTriggered) {
    processSyncEvent(ntpEvent);
    syncEventTriggered = false;
  }

  if (bbt_channel.length() > 0 && bbt_cmdrsrc.length() > 0 && bbt_msgrsrc.length() > 0 && channelToken.length() > 10)
  {
    // If the psclient is not connected to the server
    if (!psclient.connected() && (now_ - mqTimestamp) > 5000)
    {
      mqTimestamp = now_;
      // Try connecting to the server
      String username = "token:";
      username += channelToken;
      psclient.connect(clientID, username.c_str(), NULL);

      if (psclient.connected()) {
        Serial.println(F("[ INFO ] MQTT connected"));
        psclient.setCallback(callback);

        // Subscribe to topic
        psclient.subscribe(bbt_cmdtopic.c_str());
      }
      else
      {
        Serial.print("MQTT connection failed: ");
        switch (psclient.state()) {
          case -4:
            Serial.println("MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time");
            break;
          case -3 :
            Serial.println("MQTT_CONNECTION_LOST - the network connection was broken");
            break;
          case -2 :
            Serial.println("MQTT_CONNECT_FAILED - the network connection failed");
            break;
          case -1 :
            Serial.println("MQTT_DISCONNECTED - the psclient is disconnected cleanly");
            break;
          case 0 :
            Serial.println("MQTT_CONNECTED - the cient is connected");
            break;
          case 1 :
            Serial.println("MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT");
            break;
          case 2 :
            Serial.println("MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the psclient identifier");
            break;
          case 3 :
            Serial.println("MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection");
            break;
          case 4 :
            Serial.println("MQTT_CONNECT_BAD_CREDENTIALS - the username / password were rejected");
            break;
          case 5 :
            Serial.println("MQTT_CONNECT_UNAUTHORIZED - the psclient was not authorized to connect");
            break;
          default:
            break;
        }
        Serial.println(psclient.state());
      }
    }
    else
    {
      // This should be called regularly to allow the psclient
      // to process incoming messages
      // and maintain its connection to the server
      psclient.loop();
    }
  }
  yield();
}

