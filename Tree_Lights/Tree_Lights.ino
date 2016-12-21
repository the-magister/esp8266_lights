/*
 * Demonstrate using an http server and an HTML form to control an LED.
 * The http server runs on the ESP8266.
 *
 * Connect to "http://esp8266WebForm.local" or "http://<IP address>"
 * to bring up an HTML form to control the LED connected GPIO#0. This works
 * for the Adafruit ESP8266 HUZZAH but the LED may be on a different pin on
 * other breakout boards.
 *
 * Imperatives to turn the LED on/off using a non-browser http client.
 * For example, using wget.
 * $ wget http://esp8266webform.local/ledon
 * $ wget http://esp8266webform.local/ledoff
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h> // add mDNS to overcome ping timeout issues
#include <Streaming.h>
#include <Metro.h>
#include <FastLED.h>

FASTLED_USING_NAMESPACE

// Fill in your WiFi router SSID and password
const char* ssid = "Looney";
const char* password = "TinyandTooney";

ESP8266WebServer server(80);
String message = "";

// Animation control
enum Anim_t { Off, Blink, On };
Anim_t Blue_LED = Off;
Anim_t Red_LED = Off;
// brightness settings
int Blue_Brightness = 1023;

const int RED_LED_PIN = 0;
const int BLUE_LED_PIN = 2;

// 45mm 12V modules
#define DATA_PIN   3
#define CLOCK_PIN    4
#define LED_TYPE    WS2801
#define COLOR_ORDER RGB
#define N_LED    14
CRGB leds[N_LED];

// general controls
#define FRAMES_PER_SECOND   20
#define BRIGHTNESS          255

// rotating "base color" used by many of the patterns
uint8_t gHue = 0; 

void handleRoot() {
  if (server.hasArg("Red_LED")) {
    handleSubmit();
  }

  returnForm();
}

void returnFail(String msg) {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void returnForm() {
  const char HEAD_FORM[] =
    "<!DOCTYPE HTML>"
    "<html>"
    "<head>"
    "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
    "<title>Christmas Tree Lights</title>"
    "<style>"
    "\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
    "</style>"
    "</head>"
    "<body>"
    "<h1>Christmas Tree Lights</h1>"
    ;
    
  const char TAIL_FORM[] =
    "</body>"
    "</html>"
    ;
    
  // append header
  message = HEAD_FORM;

  // report out states
  String redOnChecked = (Red_LED == On ? "checked" : "");
  String redBlinkChecked = (Red_LED == Blink ? "checked" : "");
  String redOffChecked = (Red_LED == Off ? "checked" : "");

  String blueOnChecked = (Blue_LED == On ? "checked" : "");
  String blueBlinkChecked = (Blue_LED == Blink ? "checked" : "");
  String blueOffChecked = (Blue_LED == Off ? "checked" : "");

  String blueBrightness = String(Blue_Brightness);

  // dynamic form
  message += "<FORM action=\"/\" method=\"post\">";
  message += "<P>";
  message += "Red LED<br>";
  message += "<INPUT type=\"radio\" name=\"Red_LED\" value=\"1\" " + redOnChecked + ">On<BR>";
  message += "<INPUT type=\"radio\" name=\"Red_LED\" value=\"2\" " + redBlinkChecked + ">Blink<BR>";
  message += "<INPUT type=\"radio\" name=\"Red_LED\" value=\"3\" " + redOffChecked + ">Off<BR>";
  message += "<br>";
  message += "Blue LED<br>";
  message += "<INPUT type=\"radio\" name=\"Blue_LED\" value=\"1\" " + blueOnChecked + ">On<BR>";
  message += "<INPUT type=\"radio\" name=\"Blue_LED\" value=\"2\" " + blueBlinkChecked + ">Blink<BR>";
  message += "<INPUT type=\"radio\" name=\"Blue_LED\" value=\"3\" " + blueOffChecked + ">Off<BR>";
  message += "Maximum Brightness: <INPUT type=\"number\" name=\"Blue_Bright\" min=\"0\" max=\"1023\" value=\"" + blueBrightness + "\"><BR>";
  message += "<br>";
  message += "<INPUT type=\"submit\" value=\"Send\"> <INPUT type=\"reset\">";
  message += "</P>";
  message += "</FORM>";

  // append footer
  message += TAIL_FORM;

  // send form
  server.send(200, "text/html", message);

  //
  Serial << endl << F("Sent ") << message.length() << F(" bytes:") << message << endl;
}

void handleSubmit() {
  Serial << endl;
  
  if (!server.hasArg("Red_LED")) return returnFail("BAD ARGS");
  if (!server.hasArg("Blue_LED")) return returnFail("BAD ARGS");
  if (!server.hasArg("Blue_Bright")) return returnFail("BAD ARGS");

  message = server.arg("Blue_LED");
  if (message == "1") {
    Blue_LED = On;
    Serial << F("Request to set Blue LED On") << endl;
  }
  else if (message == "2") {
    Blue_LED = Blink;
    Serial << F("Request to set Blue LED Blink") << endl;
  }
  else if (message == "3") {
    Blue_LED = Off;
    Serial << F("Request to set Blue LED Off") << endl;
  }
  else {
    returnFail("Unknown Blue_LED value");
  }

  message = server.arg("Red_LED");
  if (message == "1") {
    Serial << F("Request to set Red LED On") << endl;
    Red_LED = On;
  }
  else if (message == "2") {
    Serial << F("Request to set Red LED Blink") << endl;
    Red_LED = Blink;
  }
  else if (message == "3") {
    Serial << F("Request to set Red LED Off") << endl;
    Red_LED = Off;
  }
  else {
    returnFail("Unknown Red_LED value");
  }

  Blue_Brightness = server.arg("Blue_Bright").toInt();
  Serial << F("Request to set Blue Brightness ") << Blue_Brightness << endl;

}

void handleNotFound() {
  message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("subnet mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  if ( MDNS.begin ( "treelights" ) ) {
    Serial.println ( "MDNS responder started" );
  }

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.print("Connect to http://TreeLights.the-magister.com or http://");
  Serial.println(WiFi.localIP());

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  // big strings, so let's avoid fragmentation by pre-allocating
  message.reserve(4096);

  // add LEDs
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, N_LED).setCorrection(TypicalLEDStrip);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
 
  fill_solid(leds, N_LED, CRGB::Red);
  FastLED.show();  
  delay(1000);
  fill_solid(leds, N_LED, CRGB::Green);
  FastLED.show();  
  delay(1000);
  fill_solid(leds, N_LED, CRGB::Blue);
  FastLED.show();  
  delay(1000);
  FastLED.clear();
  FastLED.show();

  Serial << F("Setup complete.") << endl;
}

void animations() {
  // blink timing
  static Metro blinkTimer(500UL);
  static boolean state = false;
  if ( blinkTimer.check() ) state = !state;

  switch (Red_LED) {
    case On: digitalWrite(RED_LED_PIN, LOW); break;
    case Off: digitalWrite(RED_LED_PIN, HIGH); break;
    case Blink: digitalWrite(RED_LED_PIN, state); break;
  }

  switch (Blue_LED) {
    case On: analogWrite(BLUE_LED_PIN, 1024 - Blue_Brightness); break;
    case Off: analogWrite(BLUE_LED_PIN, 1023); break;
    case Blink: state ? analogWrite(BLUE_LED_PIN, 1024 - Blue_Brightness) : analogWrite(BLUE_LED_PIN, 1023); break;
  }

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  // animation
  fill_rainbow(leds, N_LED, gHue);

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
}

void loop(void) {
  animations();
  
  server.handleClient();

  static Metro heartbeat(5000UL);
  if( heartbeat.check() ) {
    if( WiFi.status() == WL_CONNECTION_LOST ) {
      Serial << F("WiFi connection lost!") << endl;
    }

    if( WiFi.status() == WL_DISCONNECTED ) {
      Serial << F("WiFi disconnected!") << endl;
    }
    
    Serial << F(".");
  }
}
