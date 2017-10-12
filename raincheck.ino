#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include "settings.h"

int ledPin = LED_BUILTIN;

String SMHI_URL = "http://opendata-download-metfcst.smhi.se/"
                  "api/category/pmp3g/version/2/geotype/point"
                  "/lon/" TARGET_LONGITUDE
                  "/lat/" TARGET_LATITUDE
                  "/data.json";
float precipitation[NUMBER_OF_HOURS];

// http://opendata.smhi.se/apidocs/metfcst/parameters.html#parameters

// Used to keep track of parsing state machine
typedef enum ParseMode{
  parse_init,
  parsing_key,
  key_end,
  parsing_value,
  value_ended
};

bool refresh_forecast() {
  // Retrieves a fresh forecast from the SMHI API, parses the
  // response and updates the precipitation array
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
      uint8_t buff[256] = {0};
      WiFiClient *stream = http.getStreamPtr();
      // Initialize parser state machine
      ParseMode parser = parse_init;
      String parse_buffer = "";

      // Read all data from server
      byte parsed_values = 0;
      while(http.connected() && (len > 0 || len == -1)) {
        if (parsed_values == NUMBER_OF_HOURS) {
          // All needed values have been parsed
          success = true;
          Serial.println("Parsing completed");
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

void check_rain_status() {
  // Lookup rain data
  bool success = refresh_forecast();
  if (success) {
    Serial.print("Lookup succeeded, values: ");
    bool it_will_rain = false;
    for (int i=0; i<NUMBER_OF_HOURS; i++) {
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
 
void setup() {
  Serial.begin(115200);
  delay(10);
 
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
 
  // Connect to WiFi
  Serial.print("\nConnecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  while (WiFi.status() != WL_CONNECTED) {
    blink(1, 250);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}
 
void loop() {
  check_rain_status();
  delay(60000);
}

