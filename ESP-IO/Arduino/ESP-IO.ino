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
#include <ESP8266mDNS.h>              // Zero-config Library (Bonjour, Avahi) http://esp-io.local
#include <ArduinoJson.h>              // JSON Library for Encoding and Parsing Json object to send browser. We do that because Javascript has built-in JSON parsing.
#include <FS.h>                       // SPIFFS Library for storing web files to serve to web browsers
#include <ESPAsyncTCP.h>              // Async TCP Library is mandatory for Async Web Server
#include <ESPAsyncWebServer.h>        // Async Web Server with built-in WebSocket Plug-in
#include <SPIFFSEditor.h>             // This creates a web page on server which can be used to edit text based files.
#include <NtpClientLib.h>             // To timestamp of events we get Unix Time from NTP Server
#include <TimeLib.h>                  // Library for converting epochtime to a date
#include <WiFiUdp.h>                  // Library for manipulating UDP packets which is used by NTP Client to get Timestamps

#define ONBOARDLED_D4 D4 // Built in LED on ESP-12/ESP-07
#define ONBOARDLED_D0 D0 // Built in LED on NodeMCU
#define HEARTBEAT_INTERVAL 30000
#define MAX_CLIENTS 16

// Variables for whole scope
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
uint16_t newval;
const uint8_t pinsA[10] = {A0, D0, D1, D2, D3, D4, D5, D6, D7, D8};
const String pinsS = "A0D0D1D2D3D4D5D6D7D8";
String titles[10];

typedef struct {
  String id;
  bool enabled;
  String title;
  bool out;
  bool pullup;
  bool defHIGH;
} PinData;
PinData pindata[10];

typedef enum {CL_CONN, CL_AUTH, CL_DEL, CL_UNKN} clistate;

typedef struct {
  uint32_t id;
  IPAddress ip;
  uint16_t port;
  clistate state;
} CliData;
CliData clients[MAX_CLIENTS];
uint16_t oldvals[10];

// Create AsyncWebServer instance on port "80"
AsyncWebServer server(80);
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
  Serial.print(F("[ INFO ] Got IP: "));
  Serial.println(ipInfo.ip.toString().c_str());
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


  // Set things up
  void setup()
  {
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("[ INFO ] ESP-IO v0.0.4"));

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

    for (uint8_t i = 0; i < 10; i++)
    {
      if (pindata[i].enabled)
      {
        if (i == 0)
        {
          if ((now_ - (1000 * last2)) > 5000)
          {
            newval = analogRead(A0);
            //          if (abs(newval - oldvals[i]) < 5)
            //          {
            //            newval = oldvals[i];
            //          }
            last2 += 5;
          }
          else
          {
            newval = oldvals[i];
          }
        }
        else
        {
          newval = digitalRead(pinsA[i]);
        }
        if (newval != oldvals[i])
        {
          oldvals[i] = newval;
          sendChange(pindata[i].id, pindata[i].title, newval);
        }
      }
      yield();
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
                f.close();
                ESP.reset();
              }
            }
            else if (strcmp(command, "pinlist")  == 0) {
              sendPinList(client);
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
            else if (strcmp(command, "getconf")  == 0) {
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
            else if (strcmp(command, "toggle")  == 0) {
              Serial.print(F("[ INFO ] Got command: toggle("));
              String id = root["id"].as<String>();
              Serial.print(id);
              Serial.println(F(")"));
              uint8_t p = pinNum(id);
              res = digitalRead(p);
              digitalWrite(p, !res);
              res = digitalRead(p);
              ix = pinsS.indexOf(id) / 2;
              oldvals[ix] = res;
              if (root.containsKey("token"))
              {
                String token = root["token"].as<String>();
                sendAnswer(id, res, token);
              }
              sendChange(id, pindata[ix].title, res);
            }
            else if (strcmp(command, "getpinstate")  == 0) {
              Serial.print(F("[ INFO ] Got command: getpinstate("));
              String id = root["id"].as<String>();
              String token = root["token"].as<String>();
              Serial.print(id);
              Serial.println(F(")"));
              uint8_t p = pinNum(id);
              if (p == A0) {
                res = analogRead(p);
              }
              else {
                res = digitalRead(p);
              }
              sendAnswer(id, res, token);
            }
            else if (strcmp(command, "setpinstate")  == 0) {
              Serial.print(F("[ INFO ] Got command: setpinstate("));
              String id = root["id"].as<String>();
              uint8_t value = root["value"];
              String token = root["token"].as<String>();
              Serial.print(id);
              Serial.print(F(", "));
              Serial.print(value);
              Serial.println(F(")"));
              uint8_t p = pinNum(id);
              digitalWrite(p, value);
              res = digitalRead(p);
              sendAnswer(id, res, token);
              ix = pinsS.indexOf(id) / 2;
              if (res != oldvals[ix])
              {
                oldvals[ix] = res;
                sendChange(id, pindata[ix].title, res);
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
  }


  void sendAnswer(String id, uint16_t val, String token) {
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
      ws.textAll(buffer);
    }
  }


  void sendChange(String id, String title, uint16_t val) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["command"] = "change";
    //root["method"] = "Change";
    root["id"] = id;
    root["title"] = title;
    root["value"] = val;
    size_t len = root.measureLength();
    AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
    if (buffer) {
      root.printTo((char *)buffer->get(), len + 1);
      ws.textAll(buffer);
    }
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


  void sendPinList(AsyncWebSocketClient * client) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["command"] = "pinlist";
    JsonArray& pins = root.createNestedArray("pinstates");
    for (uint8_t i = 0; i < 10; i++)
    {
      String id = pinsS.substring(2 * i, 2 + 2 * i);
      if (pindata[i].enabled) {
        JsonArray& pin_ = pins.createNestedArray();
        pin_.add(pindata[i].title);
        pin_.add(pindata[i].out);
        uint8_t p = pinNum(id);
        if (id == "A0")
        {
          pin_.add(analogRead(p));
        }
        else
        {
          pin_.add(digitalRead(p));
        }
        pin_.add(id);
      }
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


  void sendValue(uint16_t val, uint8_t gpio, AsyncWebSocketClient * client) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    //root["method"] = hstname + ".digitalRead";
    root["gpio"] = gpio;
    root["value"] = val;
    size_t len = root.measureLength();
    AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
    if (buffer) {
      root.printTo((char *)buffer->get(), len + 1);
      if (client) {
        client->text(buffer);
      } else {
        ws.textAll(buffer);
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


  // Fallback to AP Mode, so we can connect to ESP if there is no Internet connection
  void fallbacktoAPMode() {
    inAPMode = true;
    WiFi.mode(WIFI_AP);
    Serial.print(F("[ INFO ] Configuring access point... "));
    Serial.println(WiFi.softAP("ESP-IO") ? "Ready" : "Failed!");
    // Access Point IP
    IPAddress myIP = WiFi.softAPIP();
    Serial.print(F("[ INFO ] Access point IP address: "));
    Serial.println(myIP);
    server.serveStatic("/auth/", SPIFFS, "/auth/").setDefaultFile("settings.htm").setAuthentication("admin", "admin");
    // Add Text Editor (http://esp-io.local/edit) to Web Server. This feature likely will be dropped on final release.
    server.addHandler(new SPIFFSEditor("admin", "admin"));
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
    const char * ssid      = cfg["ssid"];
    const char * password  = cfg["pswd"];
    uint8_t wmode          = cfg["wmode"];
    hstname                = cfg["hostnm"].as<String>();
    wifiled                = cfg["wled"].as<String>();
    address                = cfg["addr"].as<String>();
    gw                     = cfg["gw"].as<String>();
    mask                   = cfg["mask"].as<String>();
    ipmode                 = cfg["ipmode"];
    const char * bssidmac  = cfg["bssid"];
    adminpass              = cfg["apwd"].as<String>();
    encodedApass           = base64::encode(adminpass);

    JsonArray& gpios_ = cfg["gpios"];
    for (uint8_t i = 0; i < 10; i++)  //Iterate through pins
    {
      pindata[i].id      = gpios_[i][0].as<String>();
      pindata[i].enabled = gpios_[i][1].as<bool>();
      pindata[i].title   = gpios_[i][2].as<String>();
      pindata[i].out     = gpios_[i][3].as<bool>();
      pindata[i].pullup  = gpios_[i][4].as<bool>();
      pindata[i].defHIGH = gpios_[i][5].as<bool>();
      titles[i]          = pindata[i].title;
    }

    setupGPIOS();

    byte bssid[6];
    parseBytes(bssidmac, ':', bssid, 6, 16);

    // Set Hostname.
    WiFi.hostname(hstname);

    // Start mDNS service so we can connect to http://esp-io.local (if Bonjour installed on Windows or Avahi on Linux)
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
    server.serveStatic("/auth/", SPIFFS, "/auth/").setDefaultFile("control.htm").setAuthentication("admin", adminpass.c_str());

    // Add Text Editor (http://esp-io.local/edit) to Web Server. This feature likely will be dropped on final release.
    server.addHandler(new SPIFFSEditor("admin", adminpass));

    if (wmode == 1)
    {
      inAPMode = true;
      Serial.println(F("[ INFO ] ESP-IO is running in AP Mode "));
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


  // Configure GPIOS
  void setupGPIOS()
  {
    if (wifiled == "D0" || wifiled == "D4")
    {
      uint8_t ix = pinsS.indexOf(wifiled) / 2;
      pindata[ix].enabled = false;
      uint8_t wfled = pinsA[ix];
      pinMode(wfled, OUTPUT);
      digitalWrite(wfled, HIGH);
    }

    oldvals[0] = analogRead(A0);
    for (uint8_t i = 1; i < 10; i++)  //Iterate through pins (except A0 !)
    {
      uint8_t pin = pinNum(pindata[i].id);
      if (pindata[i].enabled)
      {
        if (pindata[i].out)
        {
          pinMode(pin, OUTPUT);
          if (pindata[i].defHIGH)
          {
            digitalWrite(pin, HIGH);
          }
        }
        else if (pindata[i].pullup)
        {
          pinMode(pin, INPUT_PULLUP);
        }
        else {
          pinMode(pin, INPUT);
        }
        oldvals[i] = digitalRead(pin);
      }
    }
    Serial.println(F("[ INFO ] Configuring of I / O ports is done"));
  }


  bool connectSTA(const char* ssid, const char* password, byte bssid[6])
  {
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


  uint8_t pinNum(String s)
  {
    uint8_t ix = pinsS.indexOf(s);
    return pinsA[ix / 2];
  }
