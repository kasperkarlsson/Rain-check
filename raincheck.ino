#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include "settings.h"

// This corresponds to the number of hours to check precipitation for,
// e.g. 10 means "Will it rain in the next 10 hours?"
#define VALUES_TO_PARSE 10


int ledPin = LED_BUILTIN;
int ledStatus = LOW;
WiFiServer server(80);

String SMHI_URL = "http://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/11.5759/lat/57.4225/data.json";
float precipitation[VALUES_TO_PARSE];

// http://opendata.smhi.se/apidocs/metfcst/parameters.html#parameters

typedef enum ParseMode{
  parse_init,
  parsing_key,
  key_end,
  parsing_value,
  value_ended
};

bool get_precipitation() {
  bool success = false;
  HTTPClient http;

  Serial.print("[HTTP] Begin - ");
  Serial.println(SMHI_URL);
  http.begin(SMHI_URL);

  Serial.println("[HTTP] Sending GET request");
  // Start connection and send request
  int httpCode = http.GET();

  if(httpCode > 0) {
    Serial.printf("[HTTP] Response code: %d\n", httpCode);
    if(httpCode == HTTP_CODE_OK) {
      int len = http.getSize();
      // Create read buffer
      uint8_t buff[256] = { 0 };
      WiFiClient *stream = http.getStreamPtr();
      // Initialize parser state machine
      ParseMode parser = parse_init;
      String parse_buffer = "";

      // Read all data from server
      byte parsed_values = 0;
      while(http.connected() && (len > 0 || len == -1)) {
        if (parsed_values == VALUES_TO_PARSE) {
          success = true;
          Serial.println("Break!");
          break;
        }
        // Get available data size
        size_t available_size = stream->available();

        if(available_size) {
          // Process chunk
          int c = stream->readBytes(buff, ((available_size > sizeof(buff)) ? sizeof(buff) : available_size));

          // Parse JSON, looking for values in the following format:
          // {"name":"pmax","levelType":"hl","level":0,"unit":"kg/m2/h","values":[0.0]}
          for (int i=0; i<c; i++) {
            char current_char = (char)buff[i];
            //Serial.write(current_char);
            switch (parser) {
              case parse_init:
                if (current_char == '"') {
                  parser = parsing_key;
                  parse_buffer = "";
                }
                break;
              case parsing_key:
                if (current_char == '"') {
                  // Key parsed - check if it matches
                  if (parse_buffer == "pmax") {
                    parser = key_end;
                  }
                  else {
                    parser = parse_init;
                  }
                }
                else {
                  // Protect against too long strings
                  if (parse_buffer.length() < 20) {
                    parse_buffer += current_char;
                  }
                }
                break;
              case key_end:
                if (current_char == '[') {
                  parser = parsing_value;
                  parse_buffer = "";
                }
                break;
              case parsing_value:
                if (current_char == ']') {
                  // Finished parsing value - save and continue
                  float parsed_value = parse_buffer.toFloat();
                  precipitation[parsed_values++] = parsed_value;
                  parser = parse_init;
                }
                else {
                  // Protect against too long values
                  if (parse_buffer.length() < 5) {
                    parse_buffer += current_char;
                  }
                }
            }
          }

          if(len > 0) {
            len -= c;
          }
        }
        delay(1);
      }
      //Serial.println();
    }
  }
  else {
    Serial.printf("[HTTP] GET failed: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  return success;
}

void blink(byte blinks, int blink_duration) {
  for (byte i=0; i<blinks; i++) {
    digitalWrite(ledPin, HIGH);
    delay(blink_duration);
    digitalWrite(ledPin, LOW);
    delay(blink_duration);
  }
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
    blink(1, 250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Serving on ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  // Lookup rain data
  bool success = get_precipitation();
  if (success) {
    Serial.print("Lookup succeeded, values: ");
    bool it_will_rain = false;
    for (int i=0; i<VALUES_TO_PARSE; i++) {
      if (precipitation[i] > 0.0) {
        it_will_rain = true;
      }
      Serial.print(precipitation[i]);
      Serial.print(" ");
    }
    Serial.println();
    if (it_will_rain) {
      Serial.println("It will rain!");
      blink(50, 100);
    }
    else {
      Serial.println("It will not rain.");
      blink(5, 1000);
    }
  } else {
    Serial.println("Lookup failed");
  }
}
 
void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Wait until the client sends some data
  Serial.println("New client");
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
    get_precipitation();
  }
  
  client.println("</html>");
 
  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");
 
}

