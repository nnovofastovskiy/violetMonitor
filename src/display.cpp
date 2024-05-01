// #include <display.h>
// #include <GxEPD.h>
// #include <display.h>

// #include <GxDEPG0290BS/GxDEPG0290BS.h> // 2.9" b/w Waveshare variant, TTGO T5 V2.4.1 2.9"

// #include GxEPD_BitmapExamples

// // FreeFonts from Adafruit_GFX
// #include <Fonts/FreeMonoBold9pt7b.h>
// #include <Fonts/FreeMonoBold12pt7b.h>
// #include <Fonts/FreeMonoBold18pt7b.h>
// #include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
// #include <Fonts/FreeSerif9pt7b.h>

// #include <GxIO/GxIO_SPI/GxIO_SPI.h>
// #include <GxIO/GxIO.h>

// // for SPI pin definitions see e.g.:
// // C:\Users\xxx\Documents\Arduino\hardware\espressif\esp32\variants\lolin32\pins_arduino.h

// // BUSY -> 4, RST -> 16, DC -> 17, CS -> SS(5), CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V
// // GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/17, /*RST=*/16); // arbitrary selection of 17, 16
// // GxEPD_Class display(io, /*RST=*/16, /*BUSY=*/4);        // arbitrary selection of (16), 4
// // for LILYGO® TTGO T5 2.66 board uncomment next two lines instead of previous two lines
// // GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/ 19, /*RST=*/ 4); // LILYGO® TTGO T5 2.66
// // GxEPD_Class display(io, /*RST=*/ 4, /*BUSY=*/ 34); // LILYGO® TTGO T5 2.66

// #define ASIDE_WIDTH 100
// #define PADDING 4

// char timeString[] = "16:38 Fr 05.04.2024";
// char line1[] = "Contractation 88.18%";
// char line2[] = "Trans. 94.68%";
// char status[] = "good";
// char aside[] = "KTP";

// // const char HelloWorld[] = "Hello World!";

// // void drawHelloWorld()
// // {
// //   //Serial.println("drawHelloWorld");
// //   display.setRotation(1);
// //   display.setFont(&FreeMonoBold24pt7b);
// //   display.setTextColor(GxEPD_BLACK);
// //   int16_t tbx, tby; uint16_t tbw, tbh;
// //   display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
// //   // center bounding box by transposition of origin:
// //   uint16_t x = ((display.width() - tbw) / 2) - tbx;
// //   uint16_t y = ((display.height() - tbh) / 2) - tby;
// //   display.fillScreen(GxEPD_WHITE);
// //   display.setCursor(x, y);
// //   display.print(HelloWorld);
// //   //Serial.println("drawHelloWorld done");
// // }

// // void setup()
// // {
// //   Serial.begin(115200);
// //   Serial.println();
// //   Serial.println("setup");

// // display.init(115200); // enable diagnostic output on Serial
// // display.flush();
// //   drawAsideText(aside);
// //   drawTimeText(timeString);
// //   display.update();
// //   display.powerDown();
// //   Serial.println("setup done");
// // }

// // void loop() {
// //   // display.fillScreen(GxEPD_WHITE);
// //   // display.updateWindow(0, 0, display.width(), display.height());
// //   // drawAsideText(aside);
// //   // display.updateWindow(0, 0, display.width(), display.height());
// //   // display.powerDown();
// //   // delay(3000);
// //   // drawHelloWorld();
// //   // display.updateWindow(0, 0, display.width(), display.height());
// //   // display.powerDown();
// //   // delay(3000);
// // };

// // const char HelloWorld[] = "Hello World!";

// void drawAsideText(char *string)
// {
//   // Serial.println("drawHelloWorld");
//   display.setRotation(1);
//   display.setFont(&FreeMonoBold24pt7b);
//   display.setTextColor(GxEPD_WHITE);
//   int16_t tbx, tby;
//   uint16_t tbw, tbh;
//   display.getTextBounds(string, 0, 0, &tbx, &tby, &tbw, &tbh);
//   // center bounding box by transposition of origin:
//   // uint16_t x = 10;
//   uint16_t x = ((ASIDE_WIDTH - tbw) / 2) - tbx;
//   uint16_t y = ((display.height() - tbh) / 2) - tby;
//   // display.fillScreen(GxEPD_WHITE);
//   // display.drawRect(0, 0, 100, display.height());
//   display.fillRect(0, 0, ASIDE_WIDTH, display.height(), GxEPD_BLACK);
//   display.setCursor(x, y);
//   display.print(string);
//   // Serial.println("drawHelloWorld done");
// }
// void drawTimeText(char *string)
// {
//   // Serial.println("drawHelloWorld");
//   display.setRotation(1);
//   display.setFont(&FreeSerif9pt7b);
//   display.setTextColor(GxEPD_BLACK);
//   int16_t tbx, tby;
//   uint16_t tbw, tbh;
//   display.getTextBounds(string, 0, 0, &tbx, &tby, &tbw, &tbh);
//   // center bounding box by transposition of origin:
//   // uint16_t x = 10;
//   uint16_t x = display.width() - tbw - PADDING;
//   uint16_t y = tbh + PADDING;
//   // display.fillScreen(GxEPD_WHITE);
//   // display.drawRect(0, 0, 100, display.height());
//   // display.fillRect(0, 0, ASIDE_WIDTH, display.height(), GxEPD_BLACK);
//   display.setCursor(x, y);
//   display.print(string);
//   // Serial.println("drawHelloWorld done");
// }
