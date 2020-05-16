#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

const char *ssid = "Test";
const char *password = "Password";

IPAddress ap_local_IP(192, 168, 1, 1);
IPAddress ap_gateway(192, 168, 1, 254);
IPAddress ap_subnet(255, 255, 255, 0);

const int ONLINE_TIME_ADDRESS = 2;
const int OFFLINE_TIME_ADDRESS = 4;

const int ONE_SECOND_TICKS = 312500;
//const int ONE_MINUTE_TICKS = ONE_SECOND_TICKS * 60;
const int ONE_MINUTE_TICKS = ONE_SECOND_TICKS; // Debug

uint8_t online_time = 3; // minutes
uint8_t offline_time = 3; // minutes

bool state_on = false;

//const int RELAY_PIN = 2;
const int RELAY_PIN = LED_BUILTIN; // Debug

const char INDEX_HTML[] =
  "<!DOCTYPE HTML>"
  "<html>"
  "<head>"
  "<meta content=\"text/html; charset=ISO-8859-1\""
  " http-equiv=\"content-type\">"
  "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
  "<title>Web Relay Settings</title>"
  "<style>"
  "\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
  "</style>"
  "</head>"
  "<body>"
  "<h1>Relay Settings</h1>"
  "<FORM action=\"/\" method=\"post\">"
  "<P>"
  "<h2>%s</h2>"
  "<label>Online Time(minutes):&nbsp;</label>"
  "<input maxlength=\"3\" name=\"online\" value=\"%d\"><br>"
  "<label>Offline Time (minutes):&nbsp;</label>"
  "<input maxlength=\"3\" name=\"offline\" value=\"%d\"><br>"
  "<INPUT type=\"submit\" value=\"Update\">"
  "</P>"
  "</FORM>"
  "</body>"
  "</html>";


void ICACHE_RAM_ATTR onTimerISR()
{
  //TODO: Add code to toggle relay
  digitalWrite(RELAY_PIN, !(digitalRead(RELAY_PIN)));
  if (state_on)
  {
    state_on = false;
    timer1_write(ONE_MINUTE_TICKS * offline_time);
  }
  else
  {
    state_on = true;
    timer1_write(ONE_MINUTE_TICKS * online_time);
  }
}

byte safeReadEEPROM(int address, byte defaultValue)
{
  byte result = EEPROM.read(address);
  if (255 == result)
  {
    Serial.print("Reading default EEPROM");
    Serial.println(defaultValue);

    result = defaultValue;
  }
  else
  {
    byte inverseCopy = EEPROM.read(address + 1);
    if (result ^ inverseCopy)
    {
      Serial.print("Inverse copy broken EEPROM falback to default ");
      Serial.print(defaultValue);
      Serial.print(", result ");
      Serial.print(result);
      Serial.print(", inverse ");
      Serial.println(inverseCopy);

      result = defaultValue;
    }
  }
  return result;
}

bool safeWriteEEPROM(int address, byte value)
{
  bool result = true;
  EEPROM.write(address, value);
  EEPROM.write(address + 1, ~value);
  return result;
}


void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);
  pinMode(RELAY_PIN, OUTPUT);

  Serial.println("Reading timeouts from EEPROM...");
  online_time = safeReadEEPROM(ONLINE_TIME_ADDRESS, online_time);
  offline_time = safeReadEEPROM(OFFLINE_TIME_ADDRESS, offline_time);

  Serial.println("Setting up timer...");
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);

  // 2 Seconds warm up for timer
  timer1_write(ONE_SECOND_TICKS * 2);

  Serial.println("Configuring Soft-AP...");
  WiFi.softAPConfig(ap_local_IP, ap_gateway, ap_subnet);
  Serial.print("Starting Soft-AP... ");
  WiFi.softAP(ssid, password);

  Serial.println("Starting web server...");
  server.on("/", handleUpdate);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("General Initialization Completed!");
}

void loop()
{
  server.handleClient();
}

void handleUpdate()
{
  char page[sizeof(INDEX_HTML) + 500];
  if (server.hasArg("online") && server.hasArg("offline"))
  {
    String onlineValue = server.arg("online");
    onlineValue.trim();
    long onlineNumericalValue = onlineValue.toInt();

    String offlineValue = server.arg("offline");
    offlineValue.trim();
    long offlineNumericalValue = offlineValue.toInt();
    if (0 == onlineNumericalValue || 0 == offlineNumericalValue)
    {
      sprintf(page, INDEX_HTML, "Online and Offline Time should be a number between 1 and 254",
              online_time, offline_time);
    }
    else if (254 < onlineNumericalValue || 254 < offlineNumericalValue)
    {
      sprintf(page, INDEX_HTML, "Online and Offline Time should be less than 254",
              online_time, offline_time);
    }
    else
    {
      online_time = (uint8_t)onlineNumericalValue;
      offline_time = (uint8_t)offlineNumericalValue;
      //      safeWriteEEPROM(ONLINE_TIME_ADDRESS, online_time);
      //      safeWriteEEPROM(OFFLINE_TIME_ADDRESS, offline_time);
      sprintf(page, INDEX_HTML, "New Times Saved", online_time, offline_time);
    }
  }
  else
  {
    sprintf(page, INDEX_HTML, "", online_time, offline_time);
  }
  server.send(200, "text/html", page);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  server.send(404, "text/plain", message);
}
