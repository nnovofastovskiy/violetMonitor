// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// Released under the GPLv3 license to match the rest of the
// Adafruit NeoPixel library

#include <Adafruit_NeoPixel.h>

// Which pin on the Arduino is connected to the NeoPixels?
#define LED_PIN        5
#define BRIGHTNESS    100

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);


void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
  // END of Trinket-specific code.

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}

void loop() {
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.setPixelColor(0, pixels.Color(100, 0, 0));
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(2000);
  pixels.setPixelColor(0, pixels.Color(0, 100, 0));
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(2000);
  pixels.setPixelColor(0, pixels.Color(0, 0, 100));
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(2000);
  


}
