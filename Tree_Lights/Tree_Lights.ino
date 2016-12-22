/*
 * For the ESP8266 via Adafruit's Feather Huzzah
 * 
 * http://esp8266.github.io/Arduino/versions/2.0.0/doc/libraries.html for details on libraries
 * 
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h> // add mDNS to overcome ping timeout issues
#include <Streaming.h>
#include <Metro.h>
#include <FastLED.h>
#include <EEPROM.h>

FASTLED_USING_NAMESPACE

ESP8266WebServer server(80);
String message = "";

// Animation control
enum Anim_t { Off, Blink, On };

const int RED_LED_PIN = 0;
const int BLUE_LED_PIN = 2;

// 45mm 12V modules
#define DATA_PIN   13
#define CLOCK_PIN    14
#define LED_TYPE    WS2801
#define COLOR_ORDER RGB
#define N_LED    14
CRGB leds[N_LED];

// general controls
#define FRAMES_PER_SECOND   20UL
#define BRIGHTNESS          255

// rotating "base color" used by many of the patterns
uint8_t gHue = 0; 

// saved to EEPROM
struct Settings {
  char ssid[32];
  char password[32];
  
  Anim_t Blue_LED;
  Anim_t Red_LED;
  int Blue_Brightness;
};
Settings s;

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

const char HEAD_FORM[] PROGMEM =
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
  "<h2>Christmas Tree Lights</h2>"
  ;
  
const char TAIL_FORM[] PROGMEM =
  "</body>"
  "</html>"
  ;

// helper functions to construct a web form
String radioInput(String name, String value, boolean checked, String text) {
  String Checked = checked ? "checked" : "";

  return( "<INPUT type=\"radio\" name=\"" + name + "\" value=\"" + value + "\" " + Checked + ">" + text );
}
String numberInput(String name, int value, int minval, int maxval) {
  return( "<INPUT type=\"number\" name=\"" + name + "\" value=\"" + String(value) + "\" min=\"" + String(minval) + "\" max=\"" + String(maxval) + "\">");
}

void returnForm() {
  // append header
  message = FPSTR(HEAD_FORM);

  // dynamic form
  message += "<FORM action=\"/\" method=\"post\">";
  message += "<P>";
  message += "Red LED<br>";
  message += radioInput("Red_LED", "1", s.Red_LED==On, "On") + "<BR>";
  message += radioInput("Red_LED", "2", s.Red_LED==Blink, "Blink") + "<BR>";
  message += radioInput("Red_LED", "3", s.Red_LED==Off, "Off") + "<BR>"; 
  message += "<br>";
  
  message += "Blue LED<br>";
  message += radioInput("Blue_LED", "1", s.Blue_LED==On, "On") + "<BR>";
  message += radioInput("Blue_LED", "2", s.Blue_LED==Blink, "Blink") + "<BR>";
  message += radioInput("Blue_LED", "3", s.Blue_LED==Off, "Off") + "<BR>";
  message += numberInput("Blue_Bright", s.Blue_Brightness, 0, 255) + "<BR>";
  message += "<br>";
  
  message += "<INPUT type=\"submit\" value=\"Update Lights\">";
  message += "</P>";
  message += "</FORM>";

  // append footer
  message += FPSTR(TAIL_FORM);

  // send form
  server.send(200, "text/html", message);

  //
  Serial << F("Sent ") << message.length() << F(" bytes:") << message << endl;
}

void handleSubmit() {
  if (!server.hasArg("Red_LED")) return returnFail("BAD ARGS");
  if (!server.hasArg("Blue_LED")) return returnFail("BAD ARGS");
  if (!server.hasArg("Blue_Bright")) return returnFail("BAD ARGS");

  message = server.arg("Blue_LED");
  if (message == "1") {
    s.Blue_LED = On;
    Serial << F("Request to set Blue LED On") << endl;
  }
  else if (message == "2") {
    s.Blue_LED = Blink;
    Serial << F("Request to set Blue LED Blink") << endl;
  }
  else if (message == "3") {
    s.Blue_LED = Off;
    Serial << F("Request to set Blue LED Off") << endl;
  }
  else {
    returnFail("Unknown Blue_LED value");
  }

  message = server.arg("Red_LED");
  if (message == "1") {
    Serial << F("Request to set Red LED On") << endl;
    s.Red_LED = On;
  }
  else if (message == "2") {
    Serial << F("Request to set Red LED Blink") << endl;
    s.Red_LED = Blink;
  }
  else if (message == "3") {
    Serial << F("Request to set Red LED Off") << endl;
    s.Red_LED = Off;
  }
  else {
    returnFail("Unknown Red_LED value");
  }

  s.Blue_Brightness = server.arg("Blue_Bright").toInt();
  Serial << F("Request to set Blue Brightness ") << s.Blue_Brightness << endl;

  // update settings
  EEPROM.put(0, s);
  EEPROM.commit();
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

void animations() {
  // blink timing
  static Metro blinkTimer(500UL);
  static boolean state = false;
  if ( blinkTimer.check() ) state = !state;

  switch (s.Red_LED) {
    case On: digitalWrite(RED_LED_PIN, LOW); break;
    case Off: digitalWrite(RED_LED_PIN, HIGH); break;
    case Blink: digitalWrite(RED_LED_PIN, state); break;
  }

  switch (s.Blue_LED) {
    case On: analogWrite(BLUE_LED_PIN, 256 - s.Blue_Brightness); break;
    case Off: analogWrite(BLUE_LED_PIN, 255); break;
    case Blink: state ? analogWrite(BLUE_LED_PIN, 256 - s.Blue_Brightness) : analogWrite(BLUE_LED_PIN, 255); break;
  }

  // do some periodic updates
  EVERY_N_MILLISECONDS( 1000UL/FRAMES_PER_SECOND ) { 
    // animation
    gHue++; // slowly cycle the "base color" through the rainbow
    fill_rainbow(leds, N_LED, gHue);

    // send the 'leds' array out to the actual LED strip
    FastLED.show();  
  }
}

void setup(void) {
  Serial.begin(115200);

  // http://esp8266.github.io/Arduino/versions/2.0.0/doc/libraries.html#eeprom
  EEPROM.begin(512);

/*
  s.brightness = Blue_Brightness;
  strncpy(s.ssid, ssid, 32);
  strncpy(s.password, password, 32);
  EEPROM.put(0, s);
  EEPROM.commit();
  */
  EEPROM.get(0, s);
  
  Serial << F("Size of Settings:") << sizeof(s) << endl;
  Serial << F("SSID:") << s.ssid << endl;
  Serial << F("password:") << s.password << endl;
  Serial << F("Red_led:") << s.Red_LED << endl;
  Serial << F("Blue_led:") << s.Blue_LED << endl;
  Serial << F("Blue_bright:") << s.Blue_Brightness << endl;

  WiFi.mode(WIFI_STA); // don't need AP
  WiFi.begin(s.ssid, s.password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(s.ssid);
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
  // all pwm is software emulation; need to crank up the frequency with FastLED
  analogWriteRange(256-1);
//  analogWriteFreq(10000);
  
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
    
    Serial << F("FPS reported: ") << FastLED.getFPS() << endl;
  }
}
