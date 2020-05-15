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
const int OFFLINE_TIME_ADDRESS = 3;

byte online_time = 3; // minutes
byte offline_time = 3; // minutes

bool state_on = false;

const int RELAY_PIN = 2; 

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
    // Add code to toggle relay
    digitalWrite(RELAY_PIN,!(digitalRead(RELAY_PIN)));
    if (state_on)
    {
      state_on = false;
      timer1_write(offline_time * ???);
    }
    else
    {
      state_on = true;
      timer1_write(online_time * ???);
    }
}
//=======================================================================
//                               Setup
//=======================================================================
void setup()
{
    Serial.begin(115200);
    Serial.println("");
 
    pinMode(LED,OUTPUT);
 
    //Initialize Ticker every 0.5s
}
/

void setup() 
{
  Serial.begin(115200);
  EEPROM.begin(512);

  Serial.println("Reading timeouts from EEPROM...");
  online_time = EEPROM.read(ONLINE_TIME_ADDRESS);
  offline_time = EEPROM.read(OFFLINE_TIME_ADDRESS);

  Serial.println("Setting up timer...");
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);

  // TODO: Initial start after two seconds !!!
  timer1_write(600000); //120000 us ???
  

  Serial.println("Configuring Soft-AP...");
  WiFi.softAPConfig(ap_local_IP, ap_gateway, ap_subnet);
  Serial.print("Starting Soft-AP... ");
  WiFi.softAP(ssid, password);

  Serial.println("Staring web server...");
  server.on("/", handleUpdate);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("General Initialization Completed!");
}

void loop() 
{
  server.handleClient();
}
//Dealing with the call to root
void handleRoot() 
{
   if (server.hasArg("ssid")&& server.hasArg("Password")&& server.hasArg("IP")&&server.hasArg("GW") ) {//If all form fields contain data call handelSubmit()
    handleSubmit();
  }
  else {//Redisplay the form
    server.send(200, "text/html", INDEX_HTML);
  }
}
void handleSubmit(){//dispaly values and write to memmory
  String response="<p>The ssid is ";
 response += server.arg("ssid");
 response +="<br>";
 response +="And the password is ";
 response +=server.arg("Password");
 response +="<br>";
 response +="And the IP Address is ";
 response +=server.arg("IP");
 response +="</P><BR>";
 response +="<H2><a href=\"/\">go home</a></H2><br>";

 server.send(200, "text/html", response);
 //calling function that writes data to memory 
 write_to_Memory(String(server.arg("ssid")),String(server.arg("Password")),String(server.arg("IP")),String(server.arg("GW")));
}
//Write data to memory
/**
 * We prepping the data strings by adding the end of line symbol I decided to use ";". 
 * Then we pass it off to the write_EEPROM function to actually write it to memmory
 */
void write_to_Memory(String s,String p,String i, String g){
 s+=";";
 write_EEPROM(s,0);
 p+=";";
 write_EEPROM(p,100);
 i+=";";
 write_EEPROM(i,200); 
 g+=";";
 write_EEPROM(g,220); 
 EEPROM.commit();
}
//write to memory
void write_EEPROM(String x,int pos){
  for(int n=pos;n<x.length()+pos;n++){
     EEPROM.write(n,x[n-pos]);
  }
}
//Shows when we get a misformt or wrong request for the web server
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  message +="<H2><a href=\"/\">go home</a></H2><br>";
  server.send(404, "text/plain", message);
}
