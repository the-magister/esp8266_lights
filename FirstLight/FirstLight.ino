// Use if you want to force the software SPI subsystem to be used for some reason (generally, you don't)
// #define FASTLED_FORCE_SOFTWARE_SPI
// Use if you want to force non-accelerated pin access (hint: you really don't, it breaks lots of things)
// #define FASTLED_FORCE_SOFTWARE_SPI
// #define FASTLED_FORCE_SOFTWARE_PINS
#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ESP8266_D1_PIN_ORDER
// #define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 1

#include "FastLED.h"

///////////////////////////////////////////////////////////////////////////////////////////
//
// Move a white dot along the strip of leds.  This program simply shows how to configure the leds,
// and then how to turn a single pixel white and then off, moving down the line of pixels.
//

// How many leds are in the strip?
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

// This function sets up the ledsand tells the controller about them
void setup() {
  // sanity check delay - allows reprogramming if accidently blowing power w/leds
  delay(2000);

  FastLED.addLeds<WS2801, DATA_PIN1, CLOCK_PIN1, BGR>(bLeds, 0, NUM_LEDS_1);
  FastLED.addLeds<WS2801, DATA_PIN2, CLOCK_PIN2, BGR>(bLeds, NUM_LEDS_1, NUM_LEDS_2);

  FastLED.addLeds<APA102, DATA_PIN3, CLOCK_PIN3, BGR>(lLeds, NUM_L_LED);

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

  pinMode(BUILTIN_LED, OUTPUT);
}

// This function runs over and over, and is where you do the magic to light
// your leds.
void loop() {
  static boolean ledState = false;

  fadeToBlackBy( bLeds, NUM_B_LED, 16);
  fadeToBlackBy( lLeds, NUM_L_LED, 16);

  // Move a single white led
  static int bWhite = 0;
  bWhite++;
  if ( bWhite >= NUM_B_LED ) bWhite = 0;

  bLeds[bWhite] = CRGB::White;

  // Move a single white led
  static int lWhite = 0;
  lWhite++;
  if ( lWhite >= NUM_L_LED ) lWhite = 0;

  lLeds[lWhite] = CRGB::White;

  // Show the leds (only one of which is set to white, from above)
  FastLED.show();

  // Wait a little bit
  delay(100);

  ledState = !ledState;
  digitalWrite(BUILTIN_LED, ledState);

}
