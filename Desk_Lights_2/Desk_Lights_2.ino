
/*
   For the Wemos D1 & Mini

   http://esp8266.github.io/Arduino/versions/2.0.0/doc/libraries.html for details on libraries

*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h> // add mDNS to overcome ping timeout issues
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WiFiClient.h>

#include <Streaming.h>
#include <Metro.h>
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include <EEPROM.h>

FASTLED_USING_NAMESPACE

ESP8266WebServer server(80);
String message = "";

// pin definitions
// C:\Users\MikeD\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.3.0\variants\d1_mini
/*
  static const uint8_t SDA = 4;
  static const uint8_t SCL = 5;

  static const uint8_t LED_BUILTIN = 2;
  static const uint8_t BUILTIN_LED = 2;

  static const uint8_t D0   = 16;
  static const uint8_t D1   = 5;
  static const uint8_t D2   = 4;
  static const uint8_t D3   = 0;
  static const uint8_t D4   = 2;
  static const uint8_t D5   = 14;
  static const uint8_t D6   = 12;
  static const uint8_t D7   = 13;
  static const uint8_t D8   = 15;
  static const uint8_t RX   = 3;
  static const uint8_t TX   = 1;
*/
// LED light defs
#define NUM_LEDS_1 20
#define CLOCK_PIN1 D6
#define DATA_PIN1 D5

#define NUM_LEDS_2 20
#define CLOCK_PIN2 D8
#define DATA_PIN2 D7

#define NUM_LEDS_3 60*5
#define CLOCK_PIN3 D2
#define DATA_PIN3 D1

// This is an array of leds.  One item for each led in your strip.
#define NUM_B_LED NUM_LEDS_1+NUM_LEDS_2
CRGB bLeds[NUM_B_LED];
#define NUM_L_LED NUM_LEDS_3
CRGB lLeds[NUM_L_LED];

// general controls
#define FRAMES_PER_SECOND   20UL

// Animation control
enum Anim_t { Off, Cheerlights, Manual };

// saved to EEPROM
struct Settings {
  byte color;
  byte sparkles;
  byte bright;
  byte segments;
};
Settings s;

CRGB newColor = CRGB::Black;
CRGB currentColor = CRGB::Black;

CRGBPalette16 currentPalette;

// All the Cheerlights Colors
byte cheerIndex = 0;
const TProgmemRGBPalette16 CheerLights_p FL_PROGMEM =
{ CRGB::Red, CRGB::Red, CRGB::OldLace, CRGB::OldLace,
  CRGB::Green, CRGB::Green, CRGB::OldLace, CRGB::OldLace,
  CRGB::Blue, CRGB::Cyan, CRGB::White, CRGB::Purple,
  CRGB::Magenta, CRGB::Yellow, CRGB::Orange, CRGB::Pink
};

// A mostly red palette with green accents and white trim.
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 RedGreenWhite_p FL_PROGMEM =
{ CRGB::Gray, CRGB::Gray, CRGB::Red, CRGB::Red,
  CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red,
  CRGB::Red, CRGB::Red, CRGB::Gray, CRGB::Gray,
  CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green
};

// A mostly (dark) green palette with red berries.
#define Holly_Green 0x00580c
#define Holly_Red   0xB00402
const TProgmemRGBPalette16 Holly_p FL_PROGMEM =
{ Holly_Green, Holly_Green, Holly_Green, Holly_Green,
  Holly_Green, Holly_Green, Holly_Green, Holly_Green,
  Holly_Green, Holly_Green, Holly_Green, Holly_Green,
  Holly_Green, Holly_Green, Holly_Green, Holly_Red
};

// A red and white striped palette
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 RedWhite_p FL_PROGMEM =
{ CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red,
  CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray,
  CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red,
  CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray
};

// A mostly blue palette with white accents.
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 BlueWhite_p FL_PROGMEM =
{ CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue,
  CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue,
  CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue,
  CRGB::Blue, CRGB::Gray, CRGB::Gray, CRGB::Gray
};

// A pure "fairy light" palette with some brightness variations
#define HALFFAIRY ((CRGB::FairyLight & 0xFEFEFE) / 2)
#define QUARTERFAIRY ((CRGB::FairyLight & 0xFCFCFC) / 4)
const TProgmemRGBPalette16 FairyLight_p FL_PROGMEM =
{ CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight,
  HALFFAIRY,        HALFFAIRY,        CRGB::FairyLight, CRGB::FairyLight,
  QUARTERFAIRY,     QUARTERFAIRY,     CRGB::FairyLight, CRGB::FairyLight,
  CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight
};

// A palette of soft snowflakes with the occasional bright one
const TProgmemRGBPalette16 Snow_p FL_PROGMEM =
{ 0x304048, 0x304048, 0x304048, 0x304048,
  0x304048, 0x304048, 0x304048, 0x304048,
  0x304048, 0x304048, 0x304048, 0x304048,
  0x304048, 0x304048, 0x304048, 0xE0F0FF
};

// A palette reminiscent of large 'old-school' C9-size tree lights
// in the five classic colors: red, orange, green, blue, and white.
#define C9_Red    0xB80400
#define C9_Orange 0x902C02
#define C9_Green  0x046002
#define C9_Blue   0x070758
#define C9_White  0x606820
const TProgmemRGBPalette16 RetroC9_p FL_PROGMEM =
{ C9_Red,    C9_Orange, C9_Red,    C9_Orange,
  C9_Orange, C9_Red,    C9_Orange, C9_Red,
  C9_Green,  C9_Green,  C9_Green,  C9_Green,
  C9_Blue,   C9_Blue,   C9_Blue,
  C9_White
};

// A cold, icy pale blue palette
#define Ice_Blue1 0x0C1040
#define Ice_Blue2 0x182080
#define Ice_Blue3 0x5080C0
const TProgmemRGBPalette16 Ice_p FL_PROGMEM =
{
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue2, Ice_Blue2, Ice_Blue2, Ice_Blue3
};

// A cold, icy pale blue palette
#define Dblu 0x1C74BA
#define LBlu 0x02B4F0
#define Dorn 0xF0592A
#define Wat  0xF7A619
#define Grod 0xF8A519
const TProgmemRGBPalette16 Perteet_p FL_PROGMEM =
{
  Dblu, Dblu, Dblu, Dblu,
  Wat, Wat, Dorn, Dorn,
  LBlu, LBlu, LBlu, LBlu,
  Grod, Grod, Dorn, Dorn
};

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
  FastLED.delay(100);

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

  client.stop();
  return ( lastColor );
}

void handleRoot() {
  ledOn();
  if (server.hasArg("Color")) {
    handleSubmit();
  }

  returnForm();
  ledOff();
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
  "<title>Patty's Desk Lights</title>"
  "<style>"
  "\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
  "</style>"
  "</head>"
  "<body>"
  "<h2>Patty's Desk Lights</h2>"
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

  message += "<h4>Controls</h4>";
  message += "Brightness ";
  message += numberInput("Brightness", s.bright, 0, 255);

  message += "  Sparkles ";
  message += numberInput("Sparkles", s.sparkles, 0, 255);

  message += "<h4>Color</h4>";
  message += radioInput("Color", "0", s.color == 0, "Off") + " ";
  message += radioInput("Color", "1", s.color == 1, "CheerLights") + " ";
  message += "<br><br>";

  message += "Whites: ";
  message += radioInput("Color", "12", s.color == 12, "FairyLight");
  message += radioInput("Color", "13", s.color == 13, "White");
  message += radioInput("Color", "14", s.color == 14, "Snow");
  message += "<br><br>";

  message += "CheerLights: ";
  message += radioInput("Color", "2", s.color == 2, "Red");
  message += radioInput("Color", "3", s.color == 3, "Green");
  message += radioInput("Color", "4", s.color == 4, "Blue");
  message += radioInput("Color", "5", s.color == 5, "Cyan");
  message += radioInput("Color", "6", s.color == 6, "OldLace");
  message += radioInput("Color", "7", s.color == 7, "Purple");
  message += radioInput("Color", "8", s.color == 8, "Magenta");
  message += radioInput("Color", "9", s.color == 9, "Yellow");
  message += radioInput("Color", "10", s.color == 10, "Orange");
  message += radioInput("Color", "11", s.color == 11, "Pink");
  message += "<br><br>";

  message += "For Patty: ";
  message += radioInput("Color", "15", s.color == 15, "DarkGreen");
  message += radioInput("Color", "16", s.color == 16, "Brown");
  message += radioInput("Color", "17", s.color == 17, "Crimson");
  message += radioInput("Color", "18", s.color == 18, "CornflowerBlue");
  message += radioInput("Color", "19", s.color == 19, "CadetBlue");
  message += radioInput("Color", "20", s.color == 20, "Indigo");
  message += radioInput("Color", "21", s.color == 21, "DeepPink");
  message += radioInput("Color", "22", s.color == 22, "LightCoral");
  message += radioInput("Color", "23", s.color == 23, "MediumVioletRed");
  message += radioInput("Color", "24", s.color == 24, "PaleVioletRed");
  message += radioInput("Color", "25", s.color == 25, "LightSlateGray");
  message += radioInput("Color", "26", s.color == 26, "Goldenrod");
  message += "<br><br>";

  message += "Palettes: ";
  message += radioInput("Color", "50", s.color == 50, "RedGreenWhite");
  message += radioInput("Color", "51", s.color == 51, "Holly");
  message += radioInput("Color", "52", s.color == 52, "RedWhite");
  message += radioInput("Color", "53", s.color == 53, "BlueWhite");
  message += radioInput("Color", "54", s.color == 54, "FairyLight");
  message += radioInput("Color", "55", s.color == 55, "Snow");
  message += radioInput("Color", "56", s.color == 56, "RetroC9");
  message += radioInput("Color", "57", s.color == 57, "Ice");
  message += radioInput("Color", "58", s.color == 58, "Perteet");
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
  s.segments = server.arg("Segments").toInt();

  // display
  Serial << F("Request: color=") << s.color << F(" sparkles=") << s.sparkles;
  Serial << F(" brightness=") << s.bright << F(" segments=") << s.segments << endl;

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

    case 1 :
      newColor = getCheerLightsColor();
      currentPalette = CheerLights_p;
      cheerIndex = 0;
      break;

    // https://github.com/FastLED/FastLED/wiki/Pixel-reference

    // Cheerlights colors
    case 2 : newColor = CRGB::Red; break;
    case 3 : newColor = CRGB::Green; break;
    case 4 : newColor = CRGB::Blue; break;
    case 5 : newColor = CRGB::Cyan; break;
    case 6 : newColor = CRGB::OldLace; break;
    case 7 : newColor = CRGB::Purple; break;
    case 8 : newColor = CRGB::Magenta; break;
    case 9 : newColor = CRGB::Yellow; break;
    case 10: newColor = CRGB::Orange; break;
    case 11: newColor = CRGB::Pink; break;

    // Whites
    case 12: newColor = CRGB::FairyLight; break;
    case 13: newColor = CRGB::White; break;
    case 14: newColor = CRGB::Snow; break;

    // Custom colors for Patty
    case 15: newColor = CRGB::DarkGreen; break;
    case 16: newColor = CRGB::Brown; break;
    case 17: newColor = CRGB::Crimson; break;
    case 18: newColor = CRGB::CornflowerBlue; break;
    case 19: newColor = CRGB::CadetBlue; break;
    case 20: newColor = CRGB::Indigo; break;
    case 21: newColor = CRGB::DeepPink; break;
    case 22: newColor = CRGB::LightCoral; break;
    case 23: newColor = CRGB::MediumVioletRed; break;
    case 24: newColor = CRGB::PaleVioletRed; break;
    case 25: newColor = CRGB::LightSlateGray; break;
    case 26: newColor = CRGB::Goldenrod; break;

    // Palettes
    case 50: currentPalette = RedGreenWhite_p; break;
    case 51: currentPalette = Holly_p; break;
    case 52: currentPalette = RedWhite_p; break;
    case 53: currentPalette = BlueWhite_p; break;
    case 54: currentPalette = FairyLight_p; break;
    case 55: currentPalette = Snow_p; break;
    case 56: currentPalette = RetroC9_p; break;
    case 57: currentPalette = Ice_p; break;
    case 58: currentPalette = Perteet_p; break;
  }

  // set master brightness control
  FastLED.setBrightness(s.bright);

  // push settings to EEPROM for power-up recovery
  EEPROM.put(0, s);
  EEPROM.commit();
}

void addSparkles(fract8 chanceOfGlitter) {
  if ( random8() < chanceOfGlitter) {
    bLeds[ random16(NUM_B_LED) ] += CRGB::White;
    lLeds[ random16(NUM_L_LED) ] += CRGB::White;
  }
}

void animations() {
  static byte maxBlend = 1;
  static byte colorIndex = 0;

  // do some periodic updates
  EVERY_N_MILLISECONDS( 1000UL / FRAMES_PER_SECOND ) {
    ledOn();

    if ( s.color >= 50 ) {
      // pallettes
      colorIndex ++;
      byte j = 0;
      for ( int i = 0; i < NUM_B_LED; i++) {
        bLeds[i] = ColorFromPalette( currentPalette, colorIndex + j, 255, LINEARBLEND);
        j++;
      }
      for ( int i = 0; i < NUM_L_LED; i++) {
        lLeds[i] = ColorFromPalette( currentPalette, colorIndex + j, 255, LINEARBLEND);
        j++;
      }
    } else {
      // solids

      // perform a blend across all leds to the new color
      if ( currentColor != newColor ) {
        //      Serial << currentColor.red << F("/") << currentColor.green << F("/") << currentColor.blue;
        //      Serial << F(" -> ");
        //      Serial << newColor.red << F("/") << newColor.green << F("/") << newColor.blue;
        //      Serial << endl;

        // do a shift, but not for too long
        maxBlend = qadd8(maxBlend, 1);
        if ( maxBlend < 255 ) {
          currentColor = blend(currentColor, newColor, maxBlend);
          // assume we need to fade into a new color for cheerlights, too.
          // assign half of the palette to the new color
          for ( byte k = 0; k < 8; k++ ) currentPalette[(k + cheerIndex) % 16] = currentColor;
        } else {
          // finished the transition
          currentColor = newColor;
        }

      } else {
        if ( maxBlend != 1 ) {
          // increment cheerIndex
          cheerIndex += 5;
          if (cheerIndex >= 16) cheerIndex = 0;
        }
        maxBlend = 1;
      }

      if ( s.color != 1 ) {
        // render
        fill_solid(bLeds, NUM_B_LED, currentColor);
        fill_solid(lLeds, NUM_L_LED, currentColor);
      } else {
        // cheerlights
        // pallettes
        colorIndex ++;
        byte j = 0;
        for ( int i = 0; i < NUM_B_LED; i++) {
          bLeds[i] = ColorFromPalette( currentPalette, colorIndex + j, 255, LINEARBLEND);
          j++;
        }
        for ( int i = 0; i < NUM_L_LED; i++) {
          lLeds[i] = ColorFromPalette( currentPalette, colorIndex + j, 255, LINEARBLEND);
          j++;
        }
      }
    }

    // add some glitter
    if ( s.color != 0 && s.sparkles > 0 ) addSparkles(s.sparkles);

  }

  // send the 'leds' array out to the actual LED strip
  FastLED.show();

  ledOff();
}

void ledOff() {
  digitalWrite(LED_BUILTIN, HIGH);
}
void ledOn() {
  digitalWrite(LED_BUILTIN, LOW);
}


void heartBeat() {
  ledOff();

  if ( WiFi.status() == WL_CONNECTION_LOST ) {
    Serial << F("WiFi connection lost!") << endl;
  }

  if ( WiFi.status() == WL_DISCONNECTED ) {
    Serial << F("WiFi disconnected!") << endl;
  }

  //connect wifi if not connected
  if (WiFi.status() != WL_CONNECTED) {
    delay(50);
    WiFi.disconnect();
    delay(50);
    connect(); // and reconnect
    return;
  }

  // auto update from the webz
  if ( s.color == 1 ) newColor = getCheerLightsColor();

  Serial << F("Status.");
  Serial << F("  Set Color: ");
  if ( s.color == 0 ) Serial << F("Off");
  if ( s.color == 1 ) Serial << F("CheerLights");
  if ( s.color == 2 ) Serial << F("FairyLight");
  if ( s.color == 3 ) Serial << F("White");
  if ( s.color == 4 ) Serial << F("Snow");
  if ( s.color == 5 ) Serial << F("OldLace");
  Serial << F("  Current RGB: ") << currentColor.red << F("/") << currentColor.green << F("/") << currentColor.blue;
  Serial << F("  Brightness: ") << s.bright;
  Serial << F("  Sparkles: ") << s.sparkles;
  Serial << F("  Segments: ") << s.segments;
  Serial << F("  FPS reported: ") << FastLED.getFPS();
  Serial << endl;

  ledOn();
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

  FastLED.delay(100);

  // Read all the lines of the reply from server and print them to Serial
  // 57744 16-12-22 20:41:02 00 1 0 193.5 UTC(NIST) *

  char buffer[12];
  String dateTime = "";

  while (client.available())
  {
    String line = client.readStringUntil('\r');

    if (line.indexOf("Date") != -1)
    {
      Serial.print("=====>");
    } else
    {
      Serial.println(line);
      String Hour = line.substring(16, 16 + 2);
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

void connect(void) {
  WiFiManager wifiManager;

  // will run the AP for three minutes and then try to reconnect
  wifiManager.setConfigPortalTimeout(180);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("DeskLightsAP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("subnet mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);

  if ( MDNS.begin ( "desklights" ) ) {
    Serial.println ( "MDNS responder started" );
  }

}

void setup(void) {
  Serial.begin(115200);

  // connect to WiFi
  connect();

  // http://esp8266.github.io/Arduino/versions/2.0.0/doc/libraries.html#eeprom
  EEPROM.begin(512);

  EEPROM.get(0, s);
  Serial << F("Size of Settings:") << sizeof(s) << endl;

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.print("Connect to http://");
  Serial.println(WiFi.localIP());

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  Serial.println(" ... or possibly http://treelights.local");

  pinMode(BUILTIN_LED, OUTPUT);
  ledOn();

  // all pwm is software emulation; need to crank up the frequency with FastLED
  analogWriteRange(256 - 1);
  //  analogWriteFreq(10000);

  // big strings, so let's avoid fragmentation by pre-allocating
  message.reserve(4096);

  // add LEDs
  FastLED.addLeds<WS2801, DATA_PIN1, CLOCK_PIN1, BGR>(bLeds, 0, NUM_LEDS_1);
  FastLED.addLeds<WS2801, DATA_PIN2, CLOCK_PIN2, BGR>(bLeds, NUM_LEDS_1, NUM_LEDS_2);

  FastLED.addLeds<APA102, DATA_PIN3, CLOCK_PIN3, BGR>(lLeds, NUM_L_LED);

  FastLED.setBrightness(255);
  //  FastLED.setDither( 0 ); // can't do this with WiFi stack?

  /*
    fill_solid(bLeds, NUM_B_LED, CRGB::Red);
    fill_solid(lLeds, NUM_L_LED, CRGB::Red);
    FastLED.show();
    delay(1000);

    fill_solid(bLeds, NUM_B_LED, CRGB::Green);
    fill_solid(lLeds, NUM_L_LED, CRGB::Green);
    FastLED.show();
    delay(1000);

    fill_solid(bLeds, NUM_B_LED, CRGB::Blue);
    fill_solid(lLeds, NUM_L_LED, CRGB::Blue);
    FastLED.show();
    delay(1000);

    FastLED.clear();
    FastLED.show();
  */

  FastLED.clear();
  currentColor = CRGB::Black;
  FastLED.show();

  updateFromSettings();

  Serial << F("Setup complete.") << endl;

}

void loop(void) {
  animations();

  server.handleClient();

  static Metro heartbeat(15000UL);
  if ( heartbeat.check() ) {
    heartBeat();
  }
}

// This function is like 'triwave8', which produces a
// symmetrical up-and-down triangle sawtooth waveform, except that this
// function produces a triangle wave with a faster attack and a slower decay:
//
//     / \ 
//    /     \ 
//   /         \ 
//  /             \ 
//

uint8_t attackDecayWave8( uint8_t i)
{
  if ( i < 86) {
    return i * 3;
  } else {
    i -= 86;
    return 255 - (i + (i / 2));
  }
}

// This function takes a pixel, and if its in the 'fading down'
// part of the cycle, it adjusts the color a little bit like the
// way that incandescent bulbs fade toward 'red' as they dim.
void coolLikeIncandescent( CRGB& c, uint8_t phase)
{
  if ( phase < 128) return;

  uint8_t cooling = (phase - 128) >> 4;
  c.g = qsub8( c.g, cooling);
  c.b = qsub8( c.b, cooling * 2);
}


// Add or remove palette names from this list to control which color
// palettes are used, and in what order.
const TProgmemRGBPalette16* ActivePaletteList[] = {
  &RetroC9_p,
  &BlueWhite_p,
  &RainbowColors_p,
  &FairyLight_p,
  &RedGreenWhite_p,
  &PartyColors_p,
  &RedWhite_p,
  &Snow_p,
  &Holly_p,
  &Ice_p,
  &Perteet_p
};


// Advance to the next color palette in the list (above).
void chooseNextColorPalette( CRGBPalette16& pal)
{
  const uint8_t numberOfPalettes = sizeof(ActivePaletteList) / sizeof(ActivePaletteList[0]);
  static uint8_t whichPalette = -1;
  whichPalette = addmod8( whichPalette, 1, numberOfPalettes);

  pal = *(ActivePaletteList[whichPalette]);
}
