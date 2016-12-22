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

// Animation control
enum Anim_t { Off, Cheerlights, Manual };

// saved to EEPROM
struct Settings {
  char ssid[32];
  char password[32];

  byte color;
  boolean sparkles;
  byte bright;
};
Settings s;

CRGB newColor = CRGB::Black;
CRGB currentColor = CRGB::Black;

CRGB getCheerLightsColor() {
  static CRGB lastColor = CRGB::Black;

  const char host[] = "api.thingspeak.com";

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return ( lastColor );
  }

  // We now create a URI for the request
  String url = "/channels/1417/field/2/last.txt";

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  Serial << F("Getting Cheerlights color ... ");
  delay(100);

  // Read all the lines of the reply from server and scan for hex color
  while (client.available()) {
    unsigned long color;
    String line = client.readStringUntil('\n');
    //    Serial.println(line);
    // Example line: #808182
    if ((line[0] == '#') && (line.length() == 8)) {
      int r, g, b;
      color = strtoul(line.c_str() + 1, NULL, 16);
      Serial.print(line);
      r = (color & 0xFF0000) >> 16;
      g = (color & 0x00FF00) >>  8;
      b = (color & 0x0000FF);
      Serial << F("\t=>\tRed:") << r << F("  Green:") << g << F("  Blue:") << b;
      lastColor = CRGB(r, g, b);
    }
  }

  Serial << endl;
  return ( lastColor );
}

void handleRoot() {
  blueOn();
  if (server.hasArg("Color")) {
    handleSubmit();
  }

  returnForm();
  blueOff();
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

  return ( "<INPUT type=\"radio\" name=\"" + name + "\" value=\"" + value + "\" " + Checked + ">" + text );
}
String numberInput(String name, int value, int minval, int maxval) {
  return ( "<INPUT type=\"number\" name=\"" + name + "\" value=\"" + String(value) + "\" min=\"" + String(minval) + "\" max=\"" + String(maxval) + "\">");
}

void returnForm() {
  // append header
  message = FPSTR(HEAD_FORM);

  // dynamic form
  message += "<FORM action=\"/\" method=\"post\">";
  message += "<P>";
  message += "Color<br>";
  message += radioInput("Color", "0", s.color == 0, "Off") + "<BR>";
  message += radioInput("Color", "1", s.color == 1, "CheerLights") + "<BR>";
  // https://github.com/FastLED/FastLED/wiki/Pixel-reference
  message += radioInput("Color", "2", s.color == 2, "FairyLight") + "<BR>";
  message += radioInput("Color", "3", s.color == 3, "White") + "<BR>";
  message += radioInput("Color", "4", s.color == 4, "Snow") + "<BR>";
  message += radioInput("Color", "5", s.color == 5, "OldLace") + "<BR>";
  message += "<br>";

  message += "Sparkles ";
  message += radioInput("Sparkles", "0", !s.sparkles, "Off") ;
  message += radioInput("Sparkles", "1", s.sparkles, "On");
  message += "<br><br>";

  message += "Brightness ";
  message += numberInput("Brightness", s.bright, 0, 255);
  message += "<br><br>";

  message += "<INPUT type=\"submit\" value=\"Update Lights\">";
  message += "</P>";
  message += "</FORM>";

  // append footer
  message += FPSTR(TAIL_FORM);

  // send form
  server.send(200, "text/html", message);

  //  Serial << F("Sent ") << message.length() << F(" bytes:") << message << endl;
}

void handleSubmit() {
  if (!server.hasArg("Color")) return returnFail("BAD ARGS");

  // get form results
  s.color = server.arg("Color").toInt();
  s.sparkles = server.arg("Sparkles").toInt();
  s.bright = server.arg("Brightness").toInt();

  // display
  Serial << F("Request: color=") << s.color << F(" sparkles=") << s.sparkles << F(" brightness=") << s.bright << endl;

  updateFromSettings();
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

void updateFromSettings() {

  switch ( s.color ) {
    case 0 : newColor = CRGB::Black; break;
    case 1 : newColor = getCheerLightsColor(); break;
    case 2 : newColor = CRGB::FairyLight; break;
    case 3 : newColor = CRGB::White; break;
    case 4 : newColor = CRGB::Snow; break;
    case 5 : newColor = CRGB::OldLace; break;
  }

  // set master brightness control
  FastLED.setBrightness(s.bright);

  // TODO: sparkles

  // push settings to EEPROM for power-up recovery
  EEPROM.put(0, s);
  EEPROM.commit();
}

void addSparkles(fract8 chanceOfGlitter) {
  if( random8() < chanceOfGlitter) {
    leds[ random16(N_LED) ] += CRGB::White;
  }
}

void animations() {
  static byte maxBlend = 10;

  // do some periodic updates
  EVERY_N_MILLISECONDS( 1000UL / FRAMES_PER_SECOND ) {

    // perform a blend across all leds to the new color
    if ( currentColor != newColor ) {
      //      Serial << currentColor.red << F("/") << currentColor.green << F("/") << currentColor.blue;
      //      Serial << F(" -> ");
      //      Serial << newColor.red << F("/") << newColor.green << F("/") << newColor.blue;
      //      Serial << endl;

      blueOn();

      // do a shift, but not for too long
      if( maxBlend < 255 ) currentColor = blend(currentColor, newColor, ++maxBlend);
      else currentColor = newColor;
      
    } else {
      maxBlend = 10;
    }
    
    // render
    fill_solid(leds, N_LED, currentColor);

    // add some glitter
    if( s.sparkles ) addSparkles(24);

    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    blueOff();
  }
}

void redOff() {
  digitalWrite(RED_LED_PIN, HIGH);
}
void redOn() {
  digitalWrite(RED_LED_PIN, LOW);
}
void blueOff() {
  digitalWrite(BLUE_LED_PIN, HIGH);
}
void blueOn() {
  digitalWrite(BLUE_LED_PIN, LOW);
}

void heartBeat() {
  redOff();

  if ( WiFi.status() == WL_CONNECTION_LOST ) {
    Serial << F("WiFi connection lost!") << endl;
  }

  if ( WiFi.status() == WL_DISCONNECTED ) {
    Serial << F("WiFi disconnected!") << endl;
  }

  // auto update from the webz
  if ( s.color == 1 ) newColor = getCheerLightsColor();

  Serial << F("Status.");
  Serial << F("  Current Color: ");
  if ( s.color == 0 ) Serial << F("Off");
  if ( s.color == 1 ) Serial << F("CheerLights");
  if ( s.color == 2 ) Serial << F("FairyLight");
  if ( s.color == 3 ) Serial << F("White");
  if ( s.color == 4 ) Serial << F("Snow");
  if ( s.color == 5 ) Serial << F("OldLace");
  Serial << F("  Current Color: ") << currentColor.red << F("/") << currentColor.green << F("/") << currentColor.blue;
  Serial << F("  FPS reported: ") << FastLED.getFPS();
  Serial << endl;

  redOn();
}

void getTime() {
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 13;
  const char* host = "time.nist.gov";

  Serial.print("connecting to ");
  Serial.println(host);

  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // This will send the request to the server
  client.print("HEAD / HTTP/1.1\r\nAccept: */*\r\nUser-Agent: Mozilla/4.0 (compatible; ESP8266 NodeMcu Lua;)\r\n\r\n");

  delay(100);

  // Read all the lines of the reply from server and print them to Serial
  // expected line is like : Date: Thu, 01 Jan 2015 22:00:14 GMT
  char buffer[12];
  String dateTime = "";

  while(client.available())
  {
    String line = client.readStringUntil('\r');

    if (line.indexOf("Date") != -1)
    {
      Serial.print("=====>");
    } else
    {
      Serial.println(line);
      String Hour = line.substring(16,16+2);
      Serial.println(Hour);
      /*
      // date starts at pos 7
      TimeDate = line.substring(7);
      Serial.println(TimeDate);
      // time starts at pos 14
      TimeDate = line.substring(7, 15);
      TimeDate.toCharArray(buffer, 10);
      sendStrXY("UTC Date/Time:", 4, 0);
      sendStrXY(buffer, 5, 0);
      TimeDate = line.substring(16, 24);
      TimeDate.toCharArray(buffer, 10);
      sendStrXY(buffer, 6, 0);
      */
    }
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
  //  strcpy(s.ssid,"Looney");
  //  strcpy(s.password,"TinyandTooney");
  strcpy(s.ssid, "Magister");
  strcpy(s.password, "Magister");

  Serial << F("Size of Settings:") << sizeof(s) << endl;
  Serial << F("SSID:") << s.ssid << endl;
  Serial << F("password:") << s.password << endl;

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
  redOn();

  pinMode(BLUE_LED_PIN, OUTPUT);
  blueOff();

  // all pwm is software emulation; need to crank up the frequency with FastLED
  analogWriteRange(256 - 1);
  //  analogWriteFreq(10000);

  // big strings, so let's avoid fragmentation by pre-allocating
  message.reserve(4096);

  // add LEDs
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, N_LED).setCorrection(TypicalSMD5050);
  // set master brightness control
  FastLED.setBrightness(255);

  fill_solid(leds, N_LED, CRGB::Red);
  FastLED.show();
  delay(1000);
  fill_solid(leds, N_LED, CRGB::Green);
  FastLED.show();
  delay(1000);
  fill_solid(leds, N_LED, CRGB::Blue);
  FastLED.show();
  delay(1000);
  fill_solid(leds, N_LED, currentColor);
  FastLED.show();

  updateFromSettings();

  Serial << F("Setup complete.") << endl;

  getTime();

}

void loop(void) {
  animations();

  server.handleClient();

  static Metro heartbeat(15000UL);
  if ( heartbeat.check() ) {
    heartBeat();
  }
}
