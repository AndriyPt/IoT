#include <DHT.h>
#include <ESP8266WiFi.h>
 
// replace with your channel’s thingspeak API key and your SSID and password
//String apiKey = "thingspeak api key";
//const char* ssid = "ssid name";
//const char* password = "ssid password";
String apiKey = "thingspeak api key";
const char* ssid = "ssid name";
const char* password = "ssid password";
const char* server = "api.thingspeak.com";
 
#define DHTPIN D4
#define DHTTYPE DHT11 
 
DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

//const int sleepSeconds = 1800;
const int sleepSeconds = 30; // DEBUG !!!
const int wifiReconnectTimes = 5;
const int dhtRereadTimes = 5;
 
void setup() {
  
  Serial.begin(9600);
  Serial.println("\n\nWake up");

  // Connect D0 to RST to wake up
  pinMode(D0, WAKEUP_PULLUP);

  WiFi.begin(ssid, password);
   
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
   
  WiFi.begin(ssid, password);

  for (int i = 0; (i < wifiReconnectTimes) && (WL_CONNECTED != WiFi.status()); i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  if (WL_CONNECTED == WiFi.status()) {
    Serial.println("WiFi connected");

    Serial.println("Initialize DHT sensor");   
    dht.begin();
    delay(2000);
   
    float humidity = -1;
    float temperature = -1;
    bool dhtFailed = true;

    for (int i = 0; (i < dhtRereadTimes) && dhtFailed; i++) {
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
      dhtFailed = isnan(humidity) || isnan(temperature);
      if (dhtFailed) {
        Serial.println("Failed to read from DHT sensor!");
        delay(1000);
      }
    }

    if (!dhtFailed) {
      float heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  
      if (client.connect(server,80)) {
        String postStr = apiKey;
        postStr +="&field1=";
        postStr += String(temperature);
        postStr +="&field2=";
        postStr += String(humidity);
        postStr +="&field3=";
        postStr += String(heatIndex);
        postStr += "\r\n\r\n";
         
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(postStr.length());
        client.print("\n\n");
        client.print(postStr);
         
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.print(" degrees Celsius Humidity: ");
        Serial.print(humidity);
        Serial.print(" Heat Index: ");
        Serial.print(heatIndex);
        Serial.println("Sending data to Thingspeak");
      }
    }
    client.stop();
  }
  
  Serial.printf("Sleep for %d seconds\n\n", sleepSeconds);
  
  ESP.deepSleep(sleepSeconds * 1000000);  
}
 
void loop() {
}
