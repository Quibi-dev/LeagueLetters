#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WS2812FX.h>
#include <ArduinoJson.h>

#define WIFI_SSID           ""                 //Fill in SSID here
#define WIFI_PASSWORD       ""                 //Fill in Wifi Password here

#define API_URL             ""                 //API url where ritoApi.php is located

#define LED_PIN             5                  // DIO2 of NodeMcu
#define LED_COUNT           34                 // Amount of leds

#define WIFI_TIMEOUT        30000              // Checks WiFi every ...ms. Reset after this time, if WiFi cannot reconnect.

#define DEFAULT_COLOR       0xFF5900
#define DEFAULT_BRIGHTNESS  10
#define DEFAULT_SPEED       2000

#define MODE_ACTIVE         FX_MODE_CHASE_BLACKOUT
#define MODE_LETTER         FX_MODE_STATIC

#define LETTERS             5         //Amount of letters

#define LETTER_1_START      0
#define LETTER_1_END        7

#define LETTER_2_START      8
#define LETTER_2_END        12

#define LETTER_3_START      13
#define LETTER_3_END        18

#define LETTER_4_START      19
#define LETTER_4_END        24

#define LETTER_5_START      25
#define LETTER_5_END        33

void wifi_setup();

const size_t CAPACITY = JSON_ARRAY_SIZE(LETTERS+3);    // Compute the required size

// allocate the memory for the document
StaticJsonDocument<CAPACITY> doc;

unsigned long last_wifi_check_time = 0;

int letterPosStartArray[]   = {LETTER_1_START, LETTER_2_START, LETTER_3_START, LETTER_4_START, LETTER_5_START};
int letterPosEndArray[]     = {LETTER_1_END, LETTER_2_END, LETTER_3_END, LETTER_4_END, LETTER_5_END};

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Starting...");

  Serial.println("WS2812FX setup");
  ws2812fx.init();
  ws2812fx.setColor(DEFAULT_COLOR);
  ws2812fx.setSpeed(DEFAULT_SPEED);
  ws2812fx.setMode(FX_MODE_COMET);
  ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
  ws2812fx.start();

  Serial.println("Wifi setup");
  wifi_setup();

  Serial.println("Ready!");
}


void loop() {
  unsigned long now = millis();
  ws2812fx.service();
  if (now - last_wifi_check_time > WIFI_TIMEOUT) {                    // Checks API every x seconds if Wi-Fi connection is still active
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;              //Object of class HTTPClient
      http.begin(API_URL);
      int httpCode = http.GET();

      //Check the returning code
      if (httpCode > 0) {
        // Get the request response payload
        String payload = http.getString();

        // parse a JSON array
        deserializeJson(doc, payload);

        // extract the values
        JsonArray array = doc.as<JsonArray>();
        int count   = 0;
        bool ingame = false;

        for (JsonVariant v : array) {
          Serial.println(v.as<String>());
          
          if (count == 0) {
            if (v.as<bool>() == true) {
              ws2812fx.setSegment(count, letterPosStartArray[count], letterPosEndArray[count], MODE_ACTIVE, GREEN, 1000, false);   //Check if currently in game.
              ingame = true;
            } else {
              ingame = false;
            }
          } else {
            if (ingame == true) {
              if (v.as<String>() == "win") {
                ws2812fx.setSegment(count, letterPosStartArray[count], letterPosEndArray[count], MODE_LETTER, BLUE, 2000, false);
              } else if (v.as<String>() == "remake") {
                ws2812fx.setSegment(count, letterPosStartArray[count], letterPosEndArray[count], MODE_LETTER, WHITE, 2000, false);
              } else {
                ws2812fx.setSegment(count, letterPosStartArray[count], letterPosEndArray[count], MODE_LETTER, RED, 2000, false);
              }
            } else {
              if (v.as<String>() == "win") {
                ws2812fx.setSegment(count - 1, letterPosStartArray[count - 1], letterPosEndArray[count - 1], MODE_LETTER, BLUE, 2000, false);
              } else if (v.as<String>() == "remake") {
                ws2812fx.setSegment(count - 1, letterPosStartArray[count - 1], letterPosEndArray[count - 1], MODE_LETTER, WHITE, 2000, false);
              } else {
                ws2812fx.setSegment(count - 1, letterPosStartArray[count - 1], letterPosEndArray[count - 1], MODE_LETTER, RED, 2000, false);
              }
            }
          }
          count++;
          if(count > LETTERS){
            break;
          }
        }
      } else {
        Serial.print("HTTP error code: ");
        Serial.println(String(httpCode));
      }
      http.end();   //Close connection
      Serial.print("Checking WiFi... ");
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost. Reconnecting...");
        wifi_setup();
      } else {
        Serial.println("OK");
      }
      last_wifi_check_time = now;
    }
  }
}

/*
   Connect to WiFi. If no connection is made within WIFI_TIMEOUT, ESP gets resettet.
*/
void wifi_setup() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);

  unsigned long connect_start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (millis() - connect_start > WIFI_TIMEOUT) {
      Serial.println();
      Serial.print("Tried ");
      Serial.print(WIFI_TIMEOUT);
      Serial.print("ms. Resetting ESP now.");
      ESP.reset();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}
