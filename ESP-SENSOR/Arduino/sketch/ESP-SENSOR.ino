/*
  Copyright (c) 2018 Lubos Ruckl

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
#include <ESP8266mDNS.h>              // Zero-config Library (Bonjour, Avahi) http://esp-sensor.local
#include <ArduinoJson.h>              // JSON Library for Encoding and Parsing Json object to send browser. We do that because Javascript has built-in JSON parsing.
#include <FS.h>                       // SPIFFS Library for storing web files to serve to web browsers
#include <ESPAsyncTCP.h>              // Async TCP Library is mandatory for Async Web Server
#include <ESPAsyncWebServer.h>        // Async Web Server with built-in WebSocket Plug-in
#include <SPIFFSEditor.h>             // This creates a web page on server which can be used to edit text based files.
#include <TimeLib.h>                  // Library for converting epochtime to a date
#include <NtpClientLib.h>             // To timestamp of events we get Unix Time from NTP Server
#include <WiFiUdp.h>                  // Library for manipulating UDP packets which is used by NTP Client to get Timestamps


#define HEARTBEAT_INTERVAL 30000
#define MAX_CLIENTS 16
#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h>
#include <Wire.h>
#include <HTU21D.h>


//static const uint8_t SCL_speed = 200;
//static const uint8_t HTU21_ADR = 0x40;
// Temperature measuring time is max 50 msec @ 14 bits
//static const uint32_t SCL_STRETCH_TIMEOUT = 50000;
static const uint8_t MAX_RETRIES = 3;
bool htuinit = false;
//uint8_t htustate = 0;
//uint8_t htubuffer[5];
//uint8_t error = 0;
//float value = 0.0;

// Variables for whole scope
static const uint8_t onOffPin  = D1;
static const uint8_t configPin = D2;
static const uint8_t wifiled   = D4;
static const uint8_t SCL_PIN   = D5;
static const uint8_t SDA_PIN   = D6;
static const uint8_t relpin    = D7;
static const uint16_t port     = 80;
const uint8_t buttons[2]    = {onOffPin, configPin};
bool configMode = false;
bool shouldReboot = false;
bool inAPMode = false;
bool ipFlag = false;
bool wifiFlag = false;
int timeZone;
String ssid;
String password;
uint8_t wmode;
byte bssid[6];
const char * bssidmac;
String filename = "/P/";
String ntpserver;
String adminpass;
String encodedApass;
String hstname;
uint8_t ipmode;
String address;
String gw;
String mask;
uint8_t ntpinter;
static uint32_t blink_ = millis() / 100;
static uint32_t last = blink_ / 10;
static uint32_t last2 = last;
static uint32_t now_;
static uint32_t heartbeatTimestamp = 0;
static uint32_t htuTimestamp = 0;
static uint32_t mqTimestamp = 0;
String channelToken;
String bbt_channel;
String bbt_cmdrsrc;
String bbt_msgrsrc;
String bbt_cmdtopic;
String bbt_msgtopic;
float humidity_ = 0.0;
float temperature_ = 0.0;

String defName = "ESP-SENSOR";     // Your client ID
const char host[] = "mqtt.beebotte.com";  // The Beebotte server

// Holds the current button state.
volatile uint8_t btnState[2] = {1, 1};
// Holds the last time debounce was evaluated (in millis).
volatile uint32_t lastDebounceTime[2] = {0, 0};
// The delay threshold for debounce checking.
const uint8_t debounceDelay = 50;

typedef enum {CL_CONN, CL_AUTH, CL_DEL, CL_UNKN} clistate;

typedef struct {
  uint32_t id;
  IPAddress ip;
  uint16_t port;
  clistate state;
} CliData;
CliData clients[MAX_CLIENTS];

uint8_t defstate;


// Variables
WiFiClient wifiClient;
PubSubClient psclient(host, 1883, wifiClient);
//sensor object
HTU21D myHTU21D(HTU21D_RES_RH12_TEMP14);


// Function prototypes
void callback(char* topic, byte* payload, unsigned int length);


// Create AsyncWebServer instance on port "port"
AsyncWebServer server(port);
// Create WebSocket instance on URL "/ws"
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

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
  String ipString = ipInfo.ip.toString();
  ipFlag = ipString != "0.0.0.0";
  if (ipFlag) {
    Serial.print(F("[ INFO ] Got IP: "));
    Serial.println(ipString);
    digitalWrite(wifiled, LOW); // Turn on LED
  }
  NTP.begin(ntpserver, timeZone, true);
  NTP.setInterval(ntpinter * 60); // Poll every "ntpinter" minutes
}


// Manage ssid connection
void onSTAConnected(WiFiEventStationModeConnected event_info)
{
  wifiFlag = true;
  Serial.print(F("[ INFO ] Connected to WiFi \""));
  Serial.print(event_info.ssid);
  Serial.print(F("\". Channel: "));
  Serial.println(event_info.channel);
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
    digitalWrite(wifiled, HIGH); // Turn off LED
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


// Set things up
void setup()
{
  setupGPIOS();
  for (uint8_t i = 0; i < 2; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }
  // Attach an interrupt to the pin, assign the onChange function as a handler and trigger on changes (LOW or HIGH).
  attachInterrupt(buttons[0], onChange1, CHANGE);
  attachInterrupt(buttons[1], onChange2, CHANGE);
  Serial.begin(115200);
  Serial.println();

  uint8_t i = 1;
  do {
    if (myHTU21D.begin(SDA_PIN, SCL_PIN)) {
      htuinit = true;
      Serial.println(F("HTU21D sensor found"));
      break;
    }
    else {
      Serial.println(F("HTU21D not found"));
      if (i < MAX_RETRIES)
      {
        i++;
        delay(2000);
      }
      else
      {
        break;
      }
    }
  } while (true);

  Serial.println();
  Serial.println(F("[ INFO ] ESP-SENSOR v0.0.1"));

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
  bool cfgfile = loadConfiguration();
  if (!cfgfile)
  {
    Serial.println(F("[ INFO ] Config file not found -> configuration mode forced"));
  }
  bool pinPressed = digitalRead(configPin) == 0;
  if (pinPressed)
  {
    Serial.println(F("[ INFO ] Button pressed -> configuration mode selected"));
  }
  configMode = !cfgfile || pinPressed;
  startServer();
  if (configMode)
  {
    fallbacktoAPMode();
  }

  else {
    if (wmode == 1)
    {
      inAPMode = true;
      Serial.println(F("[ INFO ] ESP-SENSOR is running in AP Mode "));
      WiFi.mode(WIFI_AP);
      Serial.print(F("[ INFO ] Configuring access point... "));
      IPAddress apIP;
      if (!apIP.fromString(address))
      {
        apIP.fromString("192.168.4.1");
      }
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      bool APrslt = WiFi.softAP(ssid.c_str(), password.c_str());
      if (APrslt) {
        Serial.println("Ready");
        digitalWrite(wifiled, LOW);
        IPAddress myIP = WiFi.softAPIP();
        Serial.print(F("[ INFO ] Access point IP address: "));
        Serial.println(myIP);
        Serial.print(F("[ INFO ] Access point SSID: "));
        Serial.println(ssid);
      }
      else {
        Serial.println("Failed!");
      }
    }
    else   //waiting for connection in an infinite loop (no timeout !!!)
    {
      connectSTA();
    }
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

  // Start Web Server
  server.begin();
}


// Main Loop
void loop()
{
  // check for a new update and restart
  if (shouldReboot) {
    Serial.println(F("[ UPDT ] Rebooting..."));
    delay(100);
    ESP.restart();
  }

  now_ = millis();
  if (!configMode && !wmode) {
    if (!wifiFlag)
    {
      if ((now_ - 100 * blink_) > 500)
      {
        blink_ += 5;
        digitalWrite(wifiled, !digitalRead(wifiled));
      }
      yield();
    }
    else {
      digitalWrite(wifiled, LOW);
    }
  }
  if ((now_ - 1000 * last) > 60000)
  {
    NTP.getTimeDateString();
    last += 60;
  }

  if ((now_ - heartbeatTimestamp) > HEARTBEAT_INTERVAL)
  {
    if (htuinit) {
      temperature_ = myHTU21D.readTemperature();
      humidity_ = myHTU21D.readCompensatedHumidity(temperature_);
      Serial.print(F("[ INFO ] Temperature, humidity = "));
      Serial.print(temperature_, 1);
      Serial.print(F(", "));
      Serial.println(humidity_, 1);
    }
    heartbeatTimestamp = now_;
    broadcastHeartbeatMessage();
  }

  if (syncEventTriggered) {
    processSyncEvent(ntpEvent);
    syncEventTriggered = false;
  }

  if (ipFlag && wifiFlag && !configMode && !wmode && bbt_channel.length() > 0 && bbt_cmdrsrc.length() > 0 && bbt_msgrsrc.length() > 0 && channelToken.length() > 10)
  {
    // If the psclient is not connected to the server
    if (!psclient.connected() && (now_ - mqTimestamp) > 5000)
    {
      mqTimestamp = now_;
      // Try connecting to the server
      String username = "token:";
      username += channelToken;
      psclient.connect(defName.c_str(), username.c_str(), NULL);

      if (psclient.connected()) {
        Serial.println(F("[ INFO ] MQTT connected"));
        psclient.setCallback(callback);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["message"] = "connected";
        publish(root);

        // Subscribe to topic
        psclient.subscribe(bbt_cmdtopic.c_str());
      }
      else
      {
        Serial.print(F("[ WARN ] MQTT error: "));
        switch (psclient.state()) {
          case -4:
            Serial.print(F("MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time"));
            break;
          case -3 :
            Serial.print(F("MQTT_CONNECTION_LOST - the network connection was broken"));
            break;
          case -2 :
            Serial.print(F("MQTT_CONNECT_FAILED - the network connection failed"));
            break;
          case -1 :
            Serial.print(F("MQTT_DISCONNECTED - the psclient is disconnected cleanly"));
            break;
          case 0 :
            Serial.print(F("MQTT_CONNECTED - the cient is connected"));
            break;
          case 1 :
            Serial.print(F("MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT"));
            break;
          case 2 :
            Serial.print(F("MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the psclient identifier"));
            break;
          case 3 :
            Serial.print(F("MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection"));
            break;
          case 4 :
            Serial.print(F("MQTT_CONNECT_BAD_CREDENTIALS - the username / password were rejected"));
            break;
          case 5 :
            Serial.print(F("MQTT_CONNECT_UNAUTHORIZED - the psclient was not authorized to connect"));
            break;
          default:
            break;
        }
        Serial.print(F(" ("));
        Serial.print(psclient.state());
        Serial.println(F(")"));

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
        String tkn;
        if (root.containsKey("token"))
        {
          tkn = root["token"].as<String>();
        }
        else
        {
          tkn = "None";
        }
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
          if (strcmp(command, "configfile")  == 0) {
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
          else if (strcmp(command, "getstate")  == 0) {
            sendState(client, tkn);
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
          else if (strcmp(command, "toggle")  == 0)
          {
            uint8_t st = getState();
            setState(st, !st, tkn);
          }
          else if (strcmp(command, "setstate")  == 0)
          {
            setState(getState(), root["state"], tkn);
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

void setState(uint8_t oldstate, uint8_t state, String token)
{
  digitalWrite(relpin, state);
  sendChange(state, oldstate != state, token);
}

void broadcastHeartbeatMessage()
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "nop";
  root["epoch"] = now();
  root["timezone"] = timeZone;
  root["temperature"] = my_round(temperature_);
  root["humidity"] = my_round(humidity_);
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


void sendChange(uint16_t val, bool change, String token)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "change";
  root["change"] = change;
  root["state"] = val;
  if (token != "None")
  {
    root["token"] = token;
  }
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    ws.textAll(buffer);
  }
  if (psclient.connected())
  {
    DynamicJsonBuffer jsonBuffer;
    root.remove("command");
    root["message"] = "change";
    publish(root);
  }
}

void sendBtnEvent()
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "button";
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    ws.textAll(buffer);
  }
  if (psclient.connected())
  {
    root.remove("command");
    root["message"] = "button";
    publish(root);
  }
}


void sendTime() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "gettime";
  root["epoch"] = now();
  root["timezone"] = timeZone;
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    ws.textAll(buffer);
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

uint8_t getState()
{
  return digitalRead(relpin);
}

void sendState(AsyncWebSocketClient * client, String token) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "state";
  root["state"] = getState();
  root["temperature"] = my_round(temperature_);
  root["humidity"] = my_round(humidity_);
  if (token != "None") {
    root["token"] = token;
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


void sendPong(AsyncWebSocketClient * client) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "pong";
  size_t len = root.measureLength();
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    root.printTo((char *)buffer->get(), len + 1);
    if (client) {
      client->text(buffer);
    }
  }
}


#ifdef ESP8266
extern "C"
{
#include "user_interface.h"  // Used to get Wifi status information
}
#endif


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
  root["ip"]      = printIP(info.ip.addr);
  root["gw"]      = printIP(info.gw.addr);
  root["netmask"] = printIP(info.netmask.addr);
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


// Fallback to AP Mode, so we can connect to ESP if there is no Internet connection
void fallbacktoAPMode() {
  ssid = defName;;
  IPAddress apIP;
  apIP.fromString("192.168.4.1");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  inAPMode = true;
  WiFi.mode(WIFI_AP);
  Serial.print(F("[ INFO ] Configuring access point... "));
  bool APrslt = WiFi.softAP(ssid.c_str());
  if (APrslt) {
    Serial.println("Ready");
    digitalWrite(wifiled, LOW);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print(F("[ INFO ] Access point IP address: "));
    Serial.println(myIP);
    Serial.print(F("[ INFO ] Access point SSID: "));
    Serial.println(ssid);
    server.serveStatic("/auth/", SPIFFS, "/auth/").setDefaultFile("settings.htm").setAuthentication("admin", "admin");
    // Add Text Editor (http://esp-sensor.local/edit) to Web Server. This feature likely will be dropped on final release.
    server.addHandler(new SPIFFSEditor("admin", "admin"));
  }
  else {
    Serial.println("Failed!");
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
  ssid                   = cfg["ssid"].as<String>();
  password               = cfg["pswd"].as<String>();
  wmode                  = cfg["wmode"];
  hstname                = cfg["hostnm"].as<String>();
  defstate               = cfg["defstate"];
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
  bssidmac               = cfg["bssid"];
  adminpass              = cfg["apwd"].as<String>();
  encodedApass           = base64::encode(adminpass);
  return true;
}

void startServer()
{
  parseBytes(bssidmac, ':', bssid, 6, 16);

  // Set Hostname.
  WiFi.hostname(hstname);

  // Start mDNS service so we can connect to http://esp-sensor.local (if Bonjour installed on Windows or Avahi on Linux)
  if (!MDNS.begin(hstname.c_str())) {
    Serial.println(F("[ WARN ] Error setting up MDNS responder!"));
  }

  else
  {
    Serial.println(F("[ INFO ] Start mDNS service ..."));
  }

  // Add Web Server service to mDNS
  MDNS.addService("http", "tcp", port);

  // Serve confidential files in /auth/ folder with a Basic HTTP authentication
  server.serveStatic("/auth/", SPIFFS, "/auth/").setDefaultFile("control.htm").setAuthentication("admin", adminpass.c_str());

  // Add Text Editor (http://esp-sensor.local/edit) to Web Server. This feature likely will be dropped on final release.
  server.addHandler(new SPIFFSEditor("admin", adminpass));
}




// Configure GPIOS
void setupGPIOS()
{
  pinMode(wifiled, OUTPUT);
  digitalWrite(wifiled, HIGH);
  pinMode(relpin, OUTPUT);
  digitalWrite(relpin, defstate);
  Serial.println(F("[ INFO ] Configuring of I / O ports done"));
}


void connectSTA()
{
  WiFi.mode(WIFI_STA);
  if (ipmode == 0)
  {
    IPAddress ip;
    IPAddress gateway;
    IPAddress subnet;
    WiFi.config(ip, gateway, subnet, gateway);
  }

  if ((String)bssid[0] == "0") {
    WiFi.begin(ssid.c_str(), password.c_str(), 0);
  }
  else {
    WiFi.begin(ssid.c_str(), password.c_str(), 0, bssid);
  }

  // Inform user we are trying to connect
  Serial.print(F("[ INFO ] Trying to connect WiFi \""));
  Serial.print(ssid);
  Serial.println(F("\""));
  do
  {
    now_ = millis();
    if ((now_ - 100 * blink_) > 500)
    {
      blink_ += 5;
      digitalWrite(wifiled, !digitalRead(wifiled));
    }
    yield();
  }
  while (!wifiFlag);
  digitalWrite(wifiled, LOW);
}


String copyToString(char *arrayOriginal, int arraySize)
{
  String strRes;
  for (uint8_t i = 0; i < arraySize; i++)
  {
    strRes += arrayOriginal[i];
  }
  return strRes;
}

// Handle a received message
void callback(char* topic, byte* payload, unsigned int length)
{
  // A buffer to receive a message
  char buffer[MQTT_MAX_PACKET_SIZE];

  snprintf(buffer, sizeof(buffer), "%s", payload);
  String cleanBuffer = copyToString(buffer, length);

  //Serial.println(cleanBuffer); //DEBUG

  // Decode the received message in JSON format
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
      if (command == "getinitstate")
      {
        //Serial.print(F("Command: ")); //DEBUG
        //Serial.println(command); //DEBUG
        DynamicJsonBuffer jsonBuffer;
        JsonObject& data = jsonBuffer.createObject();
        data["message"]  = "initstate";
        data["state"]    = getState();
        data["temperature"] = my_round(temperature_);
        data["humidity"] = my_round(humidity_);
        publish(data);
      }
      if (command == "getstate")
      {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& data = jsonBuffer.createObject();
        data["message"]  = "state";
        data["state"]    = getState();
        data["temperature"] = my_round(temperature_);
        data["humidity"] = my_round(humidity_);
        data["token"]    = data_["token"];
        publish(data);
      }
      if (command == "setstate")
      {
        setState(getState(), data_["value"], data_["token"]);
      }
      if (command == "toggle")
      {
        uint8_t st = getState();
        setState(st, !st, data_["token"]);
      }
    }
  }
}


// publishes data to the specified resource
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


void onChange(uint8_t btn) {
  // Get the pin reading.
  int reading = digitalRead(buttons[btn]);

  // Ignore dupe readings.
  if (reading == btnState[btn]) return;

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
  btnState[btn] = reading;

  // Work with the value now.
  if (reading == 0)
  {
    if (btn == 0)
    {
      uint8_t st = getState();
      setState(st, !st, "None");
    }
    else
    {
      sendBtnEvent();
    }
  }
}

void onChange1() {
  onChange(0);
}

void onChange2() {
  onChange(1);
}

String my_round(float val) {
  static char outstr[16];
  dtostrf(val, 15, 1, outstr);
  String strng = String(outstr);
  strng.trim();
  return strng;
}

