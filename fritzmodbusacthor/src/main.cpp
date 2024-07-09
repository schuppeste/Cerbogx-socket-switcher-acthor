#include "AsyncJson.h"
#include "ESPAsyncWebServer.h"
#include "FS.h"
#include "SPIFFS.h"
#include "esp_partition.h"
#include <Arduino.h>
#include <ArduinoFritzApi.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <ModbusClientTCP.h>
#include <WiFi.h>

WiFiClient theClient;

// Different Functions
void handleData(ModbusMessage response, uint32_t token);
bool SerialDebug = true;
void synchroniseWith_NTP_Time();
bool loadConfig();
bool saveConfig();
void syncConfig();
void acthor_heartbeat();
void set_Power(int Power);
void setonoff(bool dev);
int switchoff(String ain, int id);
int switchon(String ain, int id);
bool check(int checkid);
void removeblock();
void RollingLog(int id, int status);
bool isBetween(String &now, String &lo, String &hi);
void handleError(Error error, uint32_t token);
void setonoff(bool dev);
void serverpathes();
unsigned long getTime();

// Webform JSON
StaticJsonDocument<4096> config;
StaticJsonDocument<512> status;

String acthorIP = "";

// NTP-time
time_t nowsync;
tm myTimeInfo;
char now[5];

int device_listcount = 0;
unsigned long lastupdate = 0;
unsigned lastblock = 0;
bool WiFiConnected = false;
bool blocked = false;
// Vars ACTHOR_Control
unsigned long lastheartbeat = 0;
uint outputpower = 300;
bool acthor_lastdirection = true;
uint32_t acthor_countdirection = 0;

String hostname = "switcher";
AsyncWebServer server(80);
// AP WLAN DATA
const char *ssidap = "Standard";
const char *passap = "123456789";

// IP und Benutzer Konfiguration
// Fritzbox
String ssid = "";
String pass = "";
String fritz_user = "";
String fritz_password = "";
String fritz_ip = "";
String fritz_ain = "";
String device_item[10] = {"", "", "", "", "", "", "", "", "", ""};
String switchoffact = "";
String switchonact = "";
unsigned long lastMillisTime = 0;
// CerbogX 116300189689
const char TIME_ZONE[] = "MEZ-1MESZ-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
const char NTP_SERVER_POOL[] = "192.168.178.1";
// MODBUS SETTINGS
// esp32ModbusTCP cerbogx(100, {192, 168, 178, 68}, 502);
IPAddress ip = {192, 168, 178, 68};
uint16_t port = 502; // port of modbus server
ModbusClientTCP MB(theClient);

// Create a ModbusTCP client instance
unsigned long blocktime = 0;
unsigned int acthor_power = 0;
unsigned int acthor_regelung = 0;
unsigned long acthor_time = 0;
int acthor_count_repeat_increase = 2;
int acthor_count_repeat_decrease = 4;
double voltmin = 0.0;
double voltmax = 0.0;
double lastvoltage = 0;
unsigned int lastl1powermax = 0;
unsigned int lastl2powermax = 0;
unsigned int lastl3powermax = 0;
int lastbattsoc = 0;
int lastcurrent = 0;
int lastbattcurrent = 0;
int lastmpptstate = 0;

boolean lastalarm = 0;
int lastkwh = 0;
int kwhmin = 0;
int kwhmax = 0;
int lastaction = 0;
enum modbusType
{
  ENUM,  // enumeration
  UFIX0, // unsigned, no decimals
  SFIX0, // signed, no decimals
};
struct modbusData
{
  const char *name;
  uint16_t address;
  uint16_t length;
  modbusType type;
  uint16_t packetId;
  uint16_t serverid;
};

struct ains
{
  bool active = true;
  String id;
  String name;
  double voltmin;
  double voltmax;
  unsigned long delon;
  unsigned long deloff;
  bool invert;
  int order;
  int depon;
  int depoff;
  int hour;
  int minute;
  unsigned long lastblock = 0;
  int blocked = 0;
  bool firevent = false;
  String on;
  String off;
  int laststatus = 0;
  int status = 0;
  int itime;
  int dummy = 0;
  bool acthor = false;
};
bool acthorenable = false;
int acthorid = 255;
struct ains device_list[10];

modbusData modbusRegisters[] = {
    // 843 Batterie SOC
    // L1 Output
    // Panel gesamtleistung

    "batteryvoltage", 840, 1, UFIX0, 0, 100, // Voltage
    "batterysoc", 843, 1, UFIX0, 0, 100,
    "batterycurrent", 841, 1, SFIX0, 0, 100, // Voltage // Voltage
    "l1output", 808, 1, UFIX0, 0, 100,       // Voltage // Voltage
    "l2output", 809, 1, UFIX0, 0, 100,       // Voltage // Voltage
    "l3output", 810, 1, UFIX0, 0, 100        // Voltage // Voltage
};
uint8_t numbermodbusRegisters =
    6; // sizeof(modbusRegisters) / sizeof(modbusRegisters[0]);
uint8_t currentmodbusRegister = 0;
FritzApi fritz(fritz_user.c_str(), fritz_password.c_str(), fritz_ip.c_str());

/// @brief
void setup()
{

  WiFi.disconnect(true);
  esp_log_level_set("*", ESP_LOG_VERBOSE);

  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  // bool formatted = SPIFFS.format();
  // delay(10000);
  Serial.begin(115200);
  loadConfig();
  syncConfig();
 
  // WiFi.disconnect(true);
  WiFi.setHostname(hostname.c_str());

  delay(1000);
  WiFi.onEvent(
      [](WiFiEvent_t event, WiFiEventInfo_t info)
      {
        Serial.print("WiFi connected. IP: ");
        Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
        WiFiConnected = true;
      },
      ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(
      [](WiFiEvent_t event, WiFiEventInfo_t info)
      {
        Serial.print("WiFi lost connection. Reason: ");
        Serial.println(info.wifi_sta_disconnected.reason);
        WiFi.disconnect(true);
        WiFiConnected = false;
        WiFi.begin(ssid, pass);
        MDNS.begin("switcher.fritz.box");
        // MB.clearQueue();
      },
      ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  if (config["ssid"].as<String>() != "")
  {
    WiFi.begin(ssid, pass);
    MDNS.begin("switcher.fritz.box");
  }
  else
    WiFi.softAP(ssidap, passap);
  delay(5000);
  try
  {
    fritz.init();
  }
  catch (int e)
  {
    Serial.println("Could not connect to fritzbox: " + String(e));
  }
  setonoff(true);
  serverpathes();

  configTzTime(TIME_ZONE, NTP_SERVER_POOL);
  int cnt = 0;
  while (cnt < device_listcount)
  {
    // switchoff(device_list[cnt].id, cnt);
    cnt++;
    delay(100);
  }

  MB.onDataHandler(&handleData);
  MB.onErrorHandler(&handleError);
  // Set message timeout to 2000ms and interval between requests to the same
  // host to 200ms
  MB.setTimeout(2000, 200);
  MB.begin();
  MB.setTarget(IPAddress(192, 168, 178, 68), 502); // CerbogGX IP

  if (SerialDebug)
    Serial.println("Setup Ready");
}
static uint32_t lastMillis = 0;

void loop()
{

  // NTP Update
  if ((millis() - lastMillisTime > 60000UL) && WiFiConnected)
  {
    getLocalTime(&myTimeInfo);
    while (strftime(now, 6, "%H:%M", &myTimeInfo) == 0)
    {
      if (SerialDebug)
        Serial.print("sync");
    }
    lastMillisTime = millis();
  }

  if ((millis() - lastMillis > blocktime) && WiFiConnected)
  {
    lastMillis = millis();
    getLocalTime(&myTimeInfo);
    if (strftime(now, 6, "%H:%M", &myTimeInfo) == 0)
    {
      if (SerialDebug)
        Serial.print("sync");
    }
    if (SerialDebug)
      Serial.println(now);
    removeblock(); // remove Blocktimes

    if (SerialDebug)
      Serial.print("reading registers\n");
    if (WiFiConnected)
    {

      for (uint8_t i = 0; i < numbermodbusRegisters; i++)
      {
        uint32_t newtoken = (i << 24) | (newtoken) & 0x00FFFFFF;
        Error err = MB.addRequest(newtoken, 100, READ_HOLD_REGISTER,
                                  modbusRegisters[i].address,
                                  modbusRegisters[i].length);
        if (err != SUCCESS)
        {
          ModbusError e(err);
          if (SerialDebug)
            Serial.printf("Error creating request: %02X - %s\n", (int)e,
                          (const char *)e);
          lastupdate = 0;
        }
      }
    }
    else
    {
      lastupdate = 0;
    }
    if (SerialDebug)
      Serial.println(lastvoltage);
    int cnt = 0;
    if (lastupdate != 0) // VoltageUpdate?
    {
      acthor_heartbeat();
      while (cnt < device_listcount)
      {
        if (device_list[cnt]
                .active)
        { // Check active before check Configuration Entrys
          if (check(
                  cnt)) // All Checks Positive? (Depency, Voltage and condition)
          {
            if (device_list[cnt].status == false) // Check latest status
            {
              if (device_list[cnt].blocked == 0) // Check for ready block
              {
                device_list[cnt].blocked = 1; // Set 1 one to activate blocking
                device_list[cnt].lastblock = millis();
              }
              else if (device_list[cnt].status == false &&
                       device_list[cnt].blocked == 2)
              {
                if (switchon(device_list[cnt].id, cnt) == 1)
                {
                  device_list[cnt].status = true;
                  device_list[cnt].blocked = 0;
                }
              }
            }
          }
          else
          {
            if (device_list[cnt].status == true)
            {
              if (device_list[cnt].blocked == 0)
              {
                device_list[cnt].blocked = 3;
                device_list[cnt].lastblock = millis();
              }
              else if (device_list[cnt].status == true &&
                       device_list[cnt].blocked == 4)
              {
                if (switchoff(device_list[cnt].id, cnt) == 0)
                {
                  device_list[cnt].status = false;
                  device_list[cnt].blocked = 0;
                }
              }
            }
          }
        }
        else
        {
        }

        cnt++;
      }
    }
    else
    {
      
      outputpower = 0;
    }
  }
  else if (!WiFiConnected)
  {

  }
}

/**
 * @brief Switch On Function
 *
 * @param ain
 * @param id
 * @return int
 */
int switchon(String ain, int id)
{
  if (ain == "")
    return 1;
  int checkon = 2;
  try
  {

    boolean b = fritz.setSwitchOn(ain);
    if (b)
    {
      checkon = 1;
      if (SerialDebug)
        Serial.println("Switch is on");
    }
    else
    {
      checkon = 0;
      if (SerialDebug)
        Serial.println("Switch is off");
    }
  }
  catch (int e)
  {
    if (SerialDebug)
      Serial.println("Got errorCode during execution " + String(e));
  }
  if (checkon == 1)
    RollingLog(id, 1);
  return checkon;
}
/**
 * @brief Switch off Function
 *
 * @param ain
 * @param id
 * @return int
 */
int switchoff(String ain, int id)
{
  int checkoff = 2;
  if (ain == "")
    return 0;
  try
  {
    boolean b = fritz.setSwitchOff(ain);
    if (b)
    {
      checkoff = 1;
      if (SerialDebug)
        Serial.println("Switch is on");
    }
    else
    {
      checkoff = 0;
      if (SerialDebug)
        Serial.println("Switch is off");
    }
  }
  catch (int e)
  {
    if (SerialDebug)
      Serial.println("Got errorCode during execution " + String(e));
  }
  if (checkoff == 0)
    RollingLog(id, 0);
  return checkoff;
}
/**
 * @brief Initilization of Server Pages
 *
 */
void serverpathes()
{

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response =
        request->beginResponse(200, "text/html", "Ok");
    request->send(SPIFFS, "/www/index.html", "text/html"); });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/www/favicon.png", "image/png"); });

  server.on("/css/bootstrap.min.css", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
              AsyncWebServerResponse *response =
                  request->beginResponse(200, "text/css", "Ok");
              request->send(SPIFFS, "/www/css/bootstrap.min.css", "text/css");
            });

  server.on("/js/jsoneditor.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response =
        request->beginResponse(200, "text/javascript", "Ok");
    request->send(SPIFFS, "/www/js/jsoneditor.js"); });

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler(
      "/postconfig", [](AsyncWebServerRequest *request, JsonVariant &json)
      {
        if (json.is<JsonArray>())
        {
          config = json.as<JsonArray>();
        }
        else if (json.is<JsonObject>())
        {
          config = json.as<JsonObject>();
        }
        String response = "";
        // serializeJson(config, response);
        saveConfig();
        request->send(200, "application/json");
        // Serial.println(response);
      });

  loadConfig();

  server.addHandler(handler);

  server.on("/getconfig", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String response = "";
    serializeJson(config, response);
    request->send(200, "application/json", response); });

  server.on("/getstats", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String response = "";
    // PrettyAsyncJsonResponse(overall_log, response);
    request->send(200, "application/json", response); });

  server.on("/getstatus", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    int zn = 0;
    while (zn < device_listcount) {
      status.add(device_list[zn].status);
      zn++;
    }
    String response = "";
    serializeJson(status, response);
    request->send(200, "application/json", response); });
  server.begin();
}
/**
 * @brief Load Config
 *
 * @return true
 * @return false
 */
bool loadConfig()
{

  File configFile = SPIFFS.open("/www/config.json", "r");
  if (!configFile)
  {
    Serial.println("- failed to open config file for writing");
    return false;
  }

  deserializeJson(config, configFile);
  configFile.close();
  return true;
}
/**
 * @brief Save Config
 *
 * @return true
 * @return false
 */
bool saveConfig()
{

  Serial.print("ConfigFile_Save_Variable: ");

  File configFile = SPIFFS.open("/www/config.json", "w");
  if (!configFile)
  {
    Serial.println("- failed to open config file for writing");
    return false;
  }

  serializeJson(config, configFile);
  configFile.close();
  syncConfig();
  return true;
}

/// @brief Sync Config, Json Save Object to Array
void syncConfig()
{
  ssid = config["ssid"].as<String>();
  pass = config["password"].as<String>();
  fritz_user = config["fritzuser"].as<String>();
  fritz_password = config["fritzpass"].as<String>();
  fritz_ip = config["fritzbox"].as<String>();
  blocktime = (unsigned long)config["blocktime"].as<long>() * 1000UL;
  acthorIP = config["a_ip"].as<String>();
  acthor_regelung = config["a_r"].as<int>();
  acthor_power = config["a_p"].as<int>();
  acthor_count_repeat_decrease = config["a_r_d"].as<int>();
  acthor_count_repeat_increase = config["a_r_i"].as<int>();
  acthor_time = (unsigned long)config["a_t"].as<long>();
  int z = 0;

  while (!config["device_item"].as<JsonArray>().operator[](z).isNull())
  {
    device_list[z].active =
        config["device_item"].operator[](z)["active"].as<bool>();
    device_list[z].name =
        config["device_item"].operator[](z)["name"].as<String>();
    if (device_list[z].name.startsWith("acthor"))
    {
      device_list[z].acthor = true;
      acthorenable = true;
      acthorid = z;
    }
    else
      device_list[z].acthor = false;
    device_list[z].id =
        config["device_item"].operator[](z)["urlid"].as<String>();
    device_list[z].voltmin =
        config["device_item"].operator[](z)["voltmin"].as<double>();
    device_list[z].voltmax =
        config["device_item"].operator[](z)["voltmax"].as<double>();
    if (config["device_item"].operator[](z)["invert"].as<String>() == "OffOn")
      device_list[z].invert = true;
    else
      device_list[z].invert = false;
    device_list[z].delon =
        config["device_item"].operator[](z)["delon"].as<u_long>();
    device_list[z].deloff =
        config["device_item"].operator[](z)["deloff"].as<u_long>();
    if (config["device_item"].operator[](z)["itime"].as<String>() == "OnOff")
      device_list[z].itime = true;
    else
      device_list[z].itime = false;
    device_list[z].depon =
        config["device_item"].operator[](z)["depon"].as<int>();
    device_list[z].depoff =
        config["device_item"].operator[](z)["depoff"].as<int>();
    device_list[z].on = config["device_item"].operator[](z)["on"].as<String>();
    device_list[z].off =
        config["device_item"].operator[](z)["off"].as<String>();
    device_list[z].dummy = 1;

    z++;
  }
  device_listcount = z;
}
/**
 * @brief Check for Action and Values of Configuration
 *
 * @param checkid
 * @return true
 * @return false
 */
bool check(int checkid)
{
  int timecheck = 0;
  int voltagecheck = 0;
  int depencycheck = 0;
  String mynow = String(now);
  String zero = "";

  if (device_list[checkid].itime)
  {
    if (!(device_list[checkid].on == "00:00" &&
          device_list[checkid].off == "00:00"))
    {
      if (isBetween(mynow, device_list[checkid].on, device_list[checkid].off))
      {
        timecheck = 2;
      }
    }
    else
      timecheck = 1;
  }
  else
  {
    if (!(device_list[checkid].on == "00:00" &&
          device_list[checkid].off == "00:00"))
    {
      if (!isBetween(mynow, device_list[checkid].on,
                     device_list[checkid].off))
      {
        timecheck = 2;
      }
      else
        timecheck = 1;
    }
  }
  if ((device_list[checkid].on == "00:00" &&
       device_list[checkid].off == "00:00"))
    timecheck = 3;

  /**
   * @brief Check Voltages
   *
   */
  int templastvoltage = lastvoltage;
  if (device_list[checkid].voltmin == 0 && device_list[checkid].voltmax == 0)
    voltagecheck = 3;

  else if (!device_list[checkid].invert)
  {

    if (templastvoltage <= device_list[checkid].voltmin &&
        device_list[checkid].firevent == false)
    {
      voltagecheck = 1;
      device_list[checkid].firevent = true;
    }
    else if (templastvoltage <= device_list[checkid].voltmax &&
             device_list[checkid].firevent == true)
      voltagecheck = 1;

    else
      device_list[checkid].firevent = false;
  }
  else
  {
    if (templastvoltage >= device_list[checkid].voltmin &&
        device_list[checkid].firevent == false)
    {
      voltagecheck = 1;
    }
    else
    {
      if (!device_list[checkid].firevent)
        device_list[checkid].firevent = true;
      if (templastvoltage >= device_list[checkid].voltmax)
        device_list[checkid].firevent = false;
    }
  }

  if (device_list[checkid].voltmin == 0 && device_list[checkid].voltmax == 0)
    voltagecheck = 3;

  /**
   * @brief Check Depencys
   *
   */
  if (device_list[checkid].depoff != 0 || device_list[checkid].depon != 0)
  {
    if (device_list[checkid].depon != 0)
      if (device_list[device_list[checkid].depon - 1].status == 1)
        depencycheck = 1;
    if (device_list[checkid].depoff != 0)
      if (device_list[device_list[checkid].depoff - 1].status == 0)
        depencycheck = 1;
  }
  else
    depencycheck = 3;

  // summarize checks
  if (SerialDebug)
  {
    Serial.print(checkid);
    Serial.print("--");
    Serial.print(voltagecheck);
    Serial.print("--");
    Serial.print(depencycheck);
    Serial.print("--");
    Serial.print(timecheck);
    Serial.print("----");
    Serial.println(device_list[checkid].status);
  }
  if (timecheck == 2 || timecheck == 3)
    if (voltagecheck == 1 || voltagecheck == 3)
      if (depencycheck == 1 || depencycheck == 3)
        return true;

  return false;
}

/**
 * @brief check Time As String Compare
 *
 * @param now Value to Check
 * @param lo  Lower Value
 * @param hi Higher Value
 * @return true
 * @return false
 */
bool isBetween(String &now, String &lo, String &hi)
{
  return (now >= lo) && (now <= hi);
}

/**
 * @brief Remove Delay Blocks
 *
 */
void removeblock()
{
  int cntblocked = 0;
  while (cntblocked < device_listcount)
  {
    if (device_list[cntblocked].blocked == 1 ||
        device_list[cntblocked].blocked == 3)
    {
      if (millis() - device_list[cntblocked].lastblock >
              (1000UL * device_list[cntblocked].delon) &&
          device_list[cntblocked].blocked == 1)
      {
        device_list[cntblocked].blocked = 2;
      }

      if (millis() - device_list[cntblocked].lastblock >
              (1000UL * device_list[cntblocked].deloff) &&
          device_list[cntblocked].blocked == 3)
      {
        device_list[cntblocked].blocked = 4;
      }
    }
    cntblocked++;
  }
}

/// @brief Template to send actions to a Php-Script and Database
/// @return
void RollingLog(int id, int status)
{
  /*HTTPClient http;
  String queryString = "time=" + String(getTime()) + "&dev=" + String(id) +
                       "&status=" + String(status) +
                       "&voltage=" + String(lastvoltage);
  http.begin("http://myexampledatabase.connect/myinsert.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setConnectTimeout(100);
  int httpCode = http.POST(queryString);
  if (SerialDebug)
    Serial.println(queryString);
  // httpCode will be negative on error
  if (httpCode > 0) {
    ;
  }*/
}

/// @brief get Time from integrated local Time, synched by NTP
/// @return
unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

/// @brief Set Acthor htpp Request
/// @param Power
void set_Power(int Power)
{
  HTTPClient http;
  String url = "http://" + acthorIP + "/control.html?power=" + String(Power);
  http.begin(url);
  http.setConnectTimeout(100);
  int httpCode = http.GET();
  if (SerialDebug)
    Serial.println("acthor set " + String(Power));
  // httpCode will be negative on error
  if (httpCode > 0)
  {
    ;
  }
  http.end();
}

/// @brief Error Backup, Shutdwon Act-Thor (not used)
/// @param dev
void setonoff(bool dev)
{
  HTTPClient http;
  String url =
      "http://" + acthorIP + "/setup.jsn?devmode=" + String(dev ? 1 : 0);
  http.begin(url.c_str());
  http.setConnectTimeout(100);
  int httpCode = http.GET();
  if (SerialDebug)
    Serial.println("acthor set " + String(dev));
  // httpCode will be negative on error
  if (httpCode > 0)
  {
    ;
  }
  http.end();
}

/// @brief Ac-Thor Controller Function
void acthor_heartbeat()
{
  if (SerialDebug)
  {
    Serial.println(lastbattsoc);
    Serial.println(lastbattcurrent);
    Serial.println(lastl1powermax);
    Serial.println(lastl2powermax);
    Serial.println(lastl3powermax);
    Serial.println(acthor_lastdirection);
    Serial.println(acthor_countdirection);
    Serial.println(acthor_count_repeat_increase);
    Serial.println(acthor_power);
  }
  if (lastbattsoc > 95 && lastbattcurrent > 0 && lastl1powermax < 2500 &&
      lastl2powermax < 2500 && lastl3powermax < 2500 &&
      device_list[acthorid].status) //|| (lastmpptstate == 4)
  {
    if (acthor_lastdirection == true)
    {
      acthor_countdirection++;
    }
    else
    {
      acthor_countdirection = 0;
    }

    if (outputpower <= acthor_power &&
        acthor_countdirection >= acthor_count_repeat_increase)
    {
      if (outputpower + acthor_regelung < acthor_power)
        outputpower = outputpower + acthor_regelung;
      else
      {
        outputpower = acthor_power;
      }
      acthor_countdirection = 0;
    }
    acthor_lastdirection = true;
  }
  else
  {

    if (acthor_lastdirection == false)
    {
      acthor_countdirection++;
    }
    else
      acthor_countdirection = 0;

    if (outputpower > 0 &&
        acthor_countdirection > acthor_count_repeat_decrease)
    {
      if (outputpower > acthor_regelung)
        outputpower = outputpower - acthor_regelung;
      else
        outputpower = 0;
      acthor_countdirection = 0;
    }
    acthor_lastdirection = false;
  }

  set_Power(outputpower);

  if (SerialDebug)
    Serial.println("Acthor" + String(outputpower));
}

/// @brief Modbus Response -> parse Bytes
/// @param response
/// @param token
void handleData(ModbusMessage response, uint32_t token)
{
  if (SerialDebug)
    Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n",
                  response.getServerID(), response.getFunctionCode(), token,
                  response.size());

  uint8_t type = (token >> 24) & 0xff;
  uint32_t value = 0;
  if (SerialDebug)
    Serial.println("token" + String(type));
  switch (modbusRegisters[type].type)
  {

  case ENUM:
  case UFIX0:
  {
    value = (response[3] << 8) | (response[4]);
    Serial.printf("%s: %u\n", modbusRegisters[type].name, value);
    break;
  }
  case SFIX0:
  {
    value = (response[3] << 8) | (response[4]);
    Serial.printf("%s: %i\n", modbusRegisters[type].name, value);
    break;
  }
  }
  if (type == 0)
  {
    lastvoltage = (double)value / 10.0;
  }
  else if (type == 1)
  {
    lastbattsoc = (int)value;
  }
  else if (type == 2)
  {
    lastbattcurrent = (int16_t)value;
  }
  else if (type == 3)
  {
    lastl1powermax = (u_int16_t)value;
  }
  else if (type == 4)
  {
    lastl2powermax = (u_int16_t)value;
  }
  else if (type == 5)
  {
    lastl3powermax = (u_int16_t)value;
  }
  lastupdate = millis();
  return;
}
// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify
// the causing request
void handleError(Error error, uint32_t token)
{
  // ModbusError wraps the error code and provides a readable error message for
  // it
  ModbusError me(error);
  Serial.printf("Error response: %02X - %s token: %d\n", (int)me,
                (const char *)me, token);

  lastupdate = 0;
}