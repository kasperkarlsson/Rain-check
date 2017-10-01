#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include "settings.h"
 
int ledPin = LED_BUILTIN;
int ledStatus = LOW;
WiFiServer server(80);

/*
 * Get preticipation value
 * 
 * https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/11.5759/lat/57.4225/data.json
 * http://opendata.smhi.se/apidocs/metfcst/parameters.html#parameters
 * 
 * YR:
 * http://www.yr.no/stad/Sverige/V%C3%A4stra_G%C3%B6taland/G%C3%B6teborg/varsel.xml
*/

String http_get(String url) {
  // 
  HTTPClient http;
  String httpResponse = "";

  Serial.println("[HTTP] Begin");
  http.begin(url);

  Serial.println("[HTTP] Sending GET request");
  // Start connection and send request
  int httpCode = http.GET();

  if(httpCode > 0) {
    Serial.printf("[HTTP] Response code: %d\n", httpCode);
    if(httpCode == HTTP_CODE_OK) {
      httpResponse = http.getString();
      Serial.println(httpResponse);
    }
  }
  else {
    Serial.printf("[HTTP] GET failed: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  return httpResponse;
}
 
void setup() {
  Serial.begin(115200);
  delay(10);
 
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
 
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, HIGH);
    delay(250);
    digitalWrite(ledPin, LOW);
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
 
}
 
void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
 
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
 
  // LED control
  if (request.indexOf("/?action=LED_OFF") != -1)  {
    ledStatus = HIGH;
  }
  else if (request.indexOf("/?action=LED_ON") != -1)  {
    ledStatus = LOW;
  }
  // Update LED pin
  digitalWrite(ledPin, ledStatus);
 
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("<!DOCTYPE html>");
  client.println("<html>");
 
  client.print("Led status: <font color='");
 
  if(ledStatus == LOW) {
    client.print("green'>ON");
  } else {
    client.print("red'>OFF");
  }
  client.print("</font>");
  client.println("<br /><br />");
  client.println("<a href='/?action=LED_ON'><button>Turn On</button></a>");
  client.println("<a href='/?action=LED_OFF'><button>Turn Off</button></a><br /><br />");  
  client.println("<a href='/?action=HTTP'><button>Send HTTP request</button></a><br /><br />");  
  
  if (request.indexOf("/?action=HTTP") != -1)  {
    client.println("<b>Server response:</b><br /><br />");
    String response = http_get("http://www.example.com/");
    client.println(response);
  }
  
  client.println("</html>");
 
  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");
 
}

