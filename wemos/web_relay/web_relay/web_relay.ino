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

const int ONLINE_TIME_ADDRESS = 0;
const int OFFLINE_TIME_ADDRESS = ONLINE_TIME_ADDRESS + 2;

const uint32_t ONE_SECOND_TICKS = 312500;

uint8_t online_time = 5; // minutes
uint8_t offline_time = 2; // minutes

uint16_t seconds_left = 0;

bool state_on = false;

const int RELAY_PIN = D1;

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
  if (seconds_left > 0)
  {
    seconds_left--;
  }
  if (0 == seconds_left)
  {
    if (state_on)
    {
      state_on = false;
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(LED_BUILTIN, HIGH);
      seconds_left = offline_time * 60;
    }
    else
    {
      state_on = true;
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(LED_BUILTIN, LOW);
      seconds_left = online_time * 60;
    }
  }
  timer1_write(ONE_SECOND_TICKS);
}

byte safeReadEEPROM(int address, byte defaultValue)
{
  byte result = EEPROM.read(address);
  if (0xff == result)
  {
    Serial.print("Reading default since EEPROM is not initialized: ");
    Serial.println(defaultValue);

    result = defaultValue;
  }
  else
  {
    byte inverseCopy = EEPROM.read(address + 1);
    if (0xff != (result ^ inverseCopy))
    {
      Serial.print("Inverse copy broken. EEPROM fallback to default: ");
      Serial.print(defaultValue);
      Serial.print(", result: ");
      Serial.print(result);
      Serial.print(", inverse: ");
      Serial.println(inverseCopy);

      result = defaultValue;
    }
    else
    {
      Serial.print("Reading actual value from EEPROM:  ");
      Serial.print(result);
      Serial.print(", inverse: ");
      Serial.println(inverseCopy);
    }
  }
  return result;
}

bool safeWriteEEPROM(int address, byte value)
{
  bool result = true;
  byte inverseCopy = ~value;
  EEPROM.write(address, value);
  EEPROM.write(address + 1, inverseCopy);

  Serial.print("Writing in EEPROM value:  ");
  Serial.print(value);
  Serial.print(", inverse ");
  Serial.println(inverseCopy);

  return result;
}


void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("Reading timeouts from EEPROM...");
  online_time = safeReadEEPROM(ONLINE_TIME_ADDRESS, online_time);
  offline_time = safeReadEEPROM(OFFLINE_TIME_ADDRESS, offline_time);

  Serial.println("Setting up timer...");
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);

  // 2 Seconds warm up for timer
  seconds_left = 2;
  timer1_write(ONE_SECOND_TICKS);

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

      safeWriteEEPROM(ONLINE_TIME_ADDRESS, online_time);
      safeWriteEEPROM(OFFLINE_TIME_ADDRESS, offline_time);

      if (EEPROM.commit())
      {
        sprintf(page, INDEX_HTML, "New Times Saved", online_time, offline_time);
      }
      else
      {
        sprintf(page, INDEX_HTML, "ERROR! EEPROM commit failed", online_time, offline_time);
      }
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
