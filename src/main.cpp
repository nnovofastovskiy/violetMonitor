/**
 * WiFiManager advanced demo, contains advanced configurartion options
 * Implements TRIGGEN_PIN button press, press for ondemand configportal, hold for 3 seconds for reset settings.
 */

// #include <FS.h>
#include <Arduino.h>
#include <main.h>
// #include <SPIFFS.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <Preferences.h>
// #include <display.h>
#include <GxEPD2.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_display_selection_new_style.h>
#include <FontsRus/FreeMonoBold18.h>
#include <FontsRus/FreeSerif10.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <HTTPClient.h>

// #define GxEPD2_DRIVER_CLASS GxEPD2_290_T94_V2
#include <Adafruit_NeoPixel.h>
// #include <display.h>

// #include <GxDEPG0290BS/GxDEPG0290BS.h> // 2.9" b/w Waveshare variant, TTGO T5 V2.4.1 2.9"

// #include GxEPD_BitmapExamples

// FreeFonts from Adafruit_GFX
// #include <Fonts/FreeMonoBold9pt7b.h>
// #include <Fonts/FreeMonoBold12pt7b.h>
// #include <Fonts/FreeMonoBold18pt7b.h>
// #include <Fonts/FreeMonoBold24pt7b.h>
// #include <Fonts/FreeMono9pt7b.h>
// #include <Fonts/FreeSerif9pt7b.h>

// #include <GxIO/GxIO_SPI/GxIO_SPI.h>
// #include <GxIO/GxIO.h>

// #define TRIGGER_PIN 0
#define OPTIONS_PIN GPIO_NUM_33
#define POWER_PIN GPIO_NUM_32

#define ASIDE_WIDTH 100
#define DISPLAY_PADDING 4

#define LED_PIN 19
#define BRIGHTNESS 40

#define WAKEUP_PINS_BITMAP 0x300000000

bool shouldSaveConfig = false;

// wifimanager can run in a blocking mode or a non blocking mode
// Be sure to know how to process loops with no delay() if using non blocking
bool wm_nonblocking = false; // change to true to use non blocking

WiFiManager wm;                    // global wm instance
WiFiManagerParameter custom_field; // global param ( for non blocking w params )
HTTPClient http;

Preferences preferences;

String url;
String getHooksUrl = "https://iron-violet.deno.dev/v1/available-webhooks";

// Allocate the JSON document
JsonDocument doc;

bool optionsBtnPressed = false;
RTC_DATA_ATTR volatile bool turnOffFlag = false;
RTC_DATA_ATTR volatile bool powerBtnPushed = false;

// // BUSY -> 4, RST -> 16, DC -> 17, CS -> SS(5), CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V
// GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/17, /*RST=*/16); // arbitrary selection of 17, 16
// GxEPD_Class display(io, /*RST=*/16, /*BUSY=*/4);        // arbitrary selection of (16), 4

Adafruit_NeoPixel pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800);

void IRAM_ATTR optionsIsr()
{
  http.end();
  optionsBtnPressed = true;
}

void IRAM_ATTR powerIsr()
{
  if (digitalRead(POWER_PIN))
  {
    // Serial.print("========powerBtnPushed = ");
    // Serial.println(powerBtnPushed);
    turnOffFlag = true;
    esp_sleep_enable_timer_wakeup(1);
    esp_deep_sleep_start();
  }
  else
  {

    // Serial.print("========powerBtnPressed = ");
    // Serial.println(powerBtnPressed);
    // if (powerBtnPushed)
    // {
    //   powerBtnPushed = false;
    //   if (turnOffFlag)
    //   {
    //     turnOffFlag = false;
    //   } else
    //   {
    //     turnOffFlag = true;
    //     esp_sleep_enable_timer_wakeup(1);
    //     esp_deep_sleep_start();
    //     // esp_sleep_enable_ext1_wakeup(0x100000000, ESP_EXT1_WAKEUP_ANY_HIGH); // 1 = High, 0 = Low
    //   }
    // }
    // powerBtnPushed = false;
  }
}

void IRAM_ATTR powerIsrPush()
{
  Serial.print("========powerBtnPushed = ");
  Serial.println(powerBtnPushed);
  powerBtnPushed = true;
}

void setup()
{
  // pixels.clear(); // Set all pixel colors to 'off'
  // pinMode(TRIGGER_PIN, INPUT_PULLUP);
  check_wakeup_reason();
  if (turnOffFlag)
  {
    Serial.println("TURNING OFF BY powerBtnPressed");
    pixels.clear();
    pixels.show();
    display.init(115200); // enable diagnostic output on Serial
    display.flush();
    drawTurnOff();
    // display.update();
    // display.updateWindow(0, 0, display.width(), display.height());
    // display.update();
    display.powerOff();
    esp_sleep_enable_ext1_wakeup(0x100000000, ESP_EXT1_WAKEUP_ANY_HIGH); // 1 = High, 0 = Low
    esp_deep_sleep_start();
  }
  attachInterrupt(POWER_PIN, powerIsr, RISING);
  attachInterrupt(OPTIONS_PIN, optionsIsr, RISING);
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  // esp_sleep_enable_ext0_wakeup(OPTIONS_PIN, 1); // 1 = High, 0 = Low

  Serial.begin(115200);
  // delay(3000);
  // attachInterrupt(POWER_PIN, powerIsrOff, FALLING);
  // while(powerBtnPushed);

  esp_sleep_enable_ext1_wakeup(WAKEUP_PINS_BITMAP, ESP_EXT1_WAKEUP_ANY_HIGH); // 1 = High, 0 = Low
  esp_sleep_enable_timer_wakeup(15e6);

  Serial.println("\n Starting");
  preferences.begin("preferences", false);
  url = preferences.getString("url");
  preferences.end();
  Serial.println("URL = " + url);
  wm.setTitle("Violet monitor");
  // wm.setCustomMenuHTML("<laber for='custom_url'>URL для получения данных</label><br><input type='text' id='custom_url'/>");

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  Serial.setDebugOutput(true);

  // pinMode(TRIGGER_PIN, INPUT);

  // wm.resetSettings(); // wipe settings

  if (wm_nonblocking)
    wm.setConfigPortalBlocking(false);

  // add a custom input field
  int customFieldLength = 40;

  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");

  // test custom html input type(checkbox)
  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type

  // test custom html(radio)
  // const char *custom_input_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
  const String input_html = "<laber for='custom_url'>URL для получения данных</label><br><input type='text' id='custom_url' name='custom_url' value='" + (url ? url : "") + "'/>";
  unsigned int l = input_html.length();
  char *custom_input_str = new char[l + 1];
  strcpy(custom_input_str, input_html.c_str());
  new (&custom_field) WiFiManagerParameter(custom_input_str); // custom html input

  wm.addParameter(&custom_field);
  wm.setSaveParamsCallback(saveParamCallback);

  // custom menu via array or vector
  //
  // menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
  // const char* menu[] = {"wifi","info","param","sep","restart","exit"};
  // wm.setMenu(menu,6);
  std::vector<const char *> menu = {"wifi", "param", "sep", "restart", "exit", "custom"};
  wm.setMenu(menu);

  // set dark theme
  wm.setClass("invert");

  // wm.setConnectTimeout(20); // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(90); // auto close configportal after n seconds
  // wm.setCaptivePortalEnable(false); // disable captive portal redirection
  wm.setAPClientCheck(true); // avoid timeout if client connected to softap

  // wifi scan settings
  // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
  // wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
  // wm.setShowInfoErase(false);      // do not show erase button on info page
  // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons

  // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("VioletMonitor", ""); // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect or hit timeout");

    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    // Serial.println(url);

    http.begin(url);

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200)
    {
      // wm.disconnect();
      WiFi.mode(WIFI_OFF);
      Serial.print("HTTP ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println();
      Serial.println(payload);
      display.init(115200); // enable diagnostic output on Serial
      display.flush();

      DeserializationError error = deserializeJson(doc, payload);

      // Test if parsing succeeds
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      const char *timeString = doc["timeString"];
      const char *aside = doc["aside"];
      const char *line1 = doc["line1"];
      const char *line2 = doc["line2"];
      const char *status = doc["status"];
      Serial.print("timeString = ");
      Serial.println(timeString);
      Serial.print("asise = ");
      Serial.println(aside);
      Serial.print("line1 = ");
      Serial.println(line1);
      Serial.print("line2 = ");
      Serial.println(line2);
      Serial.print("status = ");
      Serial.println(status);

      if (!strcmp(status, "good"))
      {
        pixels.setPixelColor(0, pixels.Color(0, BRIGHTNESS, 0));
      }
      else if (!strcmp(status, "bad"))
      {
        pixels.setPixelColor(0, pixels.Color(BRIGHTNESS, 0, 0));
      }
      else if (!strcmp(status, "neutral"))
      {
        pixels.setPixelColor(0, pixels.Color(0, 0, BRIGHTNESS));
      }
      pixels.show(); // Send the updated pixel colors to the hardware.

      display.setFullWindow();
      display.firstPage();
      do
      {
        drawAsideText(aside);
        drawTimeText(timeString);
        drawLine1(line1);
        drawLine2(line2);
      } while (display.nextPage());
      display.powerOff();
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      Serial.println(":-(");
    }
    http.end();

    esp_deep_sleep_start();
  }
}

String getParam(String name)
{
  // read parameter from server, for customhmtl input
  String value;
  if (wm.server->hasArg(name))
  {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback()
{
  url = getParam("custom_url");
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM custom_url = " + url);
  preferences.begin("preferences", false);
  preferences.putString("url", url);
  preferences.end();
}

void loop()
{
  if (wm_nonblocking)
    wm.process(); // avoid delays() in loop when non-blocking and other long running code
  if (optionsBtnPressed)
  {
    Serial.println("FROM LOOP BUTTON PRESSED");
    optionsBtnPressed = false;
  }
}

void check_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;
  uint64_t wake_pin;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  Serial.println("===========WAKE UP");
  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    // checkButton();
    break;

  case ESP_SLEEP_WAKEUP_EXT1:
    wake_pin = esp_sleep_get_ext1_wakeup_status();
    wake_pin = log(wake_pin) / log(2);
    Serial.print("GPIO ");
    Serial.println(wake_pin);
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    if (wake_pin == 32)
    {
      turnOffFlag = !turnOffFlag;
      // powerBtnPushed = true;
      // powerBtnPressed = false;
      // powerBtnPressed = !powerBtnPressed;
      // Serial.print("powerBtnPressed ");
      // Serial.println(powerBtnPressed);
    }
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void drawAsideText(const char *string)
{
  // Serial.println("drawHelloWorld");
  display.setRotation(1);
  display.setFont(&FreeMonoBold18pt8b);
  display.setTextColor(GxEPD_WHITE);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(string, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  // uint16_t x = 10;
  uint16_t x = ((ASIDE_WIDTH - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  // display.fillScreen(GxEPD_WHITE);
  // display.drawRect(0, 0, 100, display.height());
  display.fillRect(0, 0, ASIDE_WIDTH, display.height(), GxEPD_BLACK);
  display.setCursor(x, y);
  display.print(string);
  // Serial.println("drawHelloWorld done");
}
void drawTimeText(const char *string)
{
  // Serial.println("drawHelloWorld");
  display.setRotation(1);
  // display.setFont(&FreeMono10pt8b);
  display.setFont(&FreeSerif10pt8b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(string, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  // uint16_t x = 10;
  uint16_t x = display.width() - tbw - DISPLAY_PADDING;
  uint16_t y = tbh + DISPLAY_PADDING;
  // display.fillScreen(GxEPD_WHITE);
  // display.drawRect(0, 0, 100, display.height());
  // display.fillRect(0, 0, ASIDE_WIDTH, display.height(), GxEPD_BLACK);
  display.setCursor(x, y);
  display.print(string);
  // Serial.println("drawHelloWorld done");
}
void drawLine1(const char *string)
{
  // Serial.println("drawHelloWorld");
  display.setRotation(1);
  display.setFont(&FreeSerif10pt8b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(string, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  // uint16_t x = 10;
  uint16_t x = ASIDE_WIDTH + DISPLAY_PADDING;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  // display.fillScreen(GxEPD_WHITE);
  // display.drawRect(0, 0, 100, display.height());
  // display.fillRect(0, 0, ASIDE_WIDTH, display.height(), GxEPD_BLACK);
  display.setCursor(x, y);
  display.print(string);
  // Serial.println("drawHelloWorld done");
}
void drawLine2(const char *string)
{
  // Serial.println("drawHelloWorld");
  display.setRotation(1);
  display.setFont(&FreeSerif10pt8b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(string, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  // uint16_t x = 10;
  uint16_t x = ASIDE_WIDTH + DISPLAY_PADDING;
  uint16_t y = display.height() - tbh - DISPLAY_PADDING;
  // display.fillScreen(GxEPD_WHITE);
  // display.drawRect(0, 0, 100, display.height());
  // display.fillRect(0, 0, ASIDE_WIDTH, display.height(), GxEPD_BLACK);
  display.setCursor(x, y);
  display.print(string);
  // Serial.println("drawHelloWorld done");
}

// void drawInfo(const char *aside, const char *timeString, const char *line1, const char *line2)
// {
//   display.setRotation(1);
//   display.setFont(&FreeMonoBold18pt8b);
//   display.setTextColor(GxEPD_WHITE);
//   int16_t tbx, tby;
//   uint16_t tbw, tbh;

//   display.getTextBounds(aside, 0, 0, &tbx, &tby, &tbw, &tbh);
//   uint16_t x = ASIDE_WIDTH + DISPLAY_PADDING;
//   uint16_t y = display.height() - tbh - DISPLAY_PADDING;
// }

void drawTurnOff()
{
  const char text[] = "Выключено";
  display.setRotation(1);
  display.setFont(&FreeSerif10pt8b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  // here we use paged drawing, even if the processor has enough RAM for full buffer
  // so this can be used with any supported processor board.
  // the cost in code overhead and execution time penalty is marginal
  // tell the graphics class to use paged drawing mode
  display.firstPage();
  do
  {
    // this part of code is executed multiple times, as many as needed,
    // in case of full buffer it is executed once
    // IMPORTANT: each iteration needs to draw the same, to avoid strange effects
    // use a copy of values that might change, don't read e.g. from analog or pins in the loop!
    display.fillScreen(GxEPD_WHITE); // set the background to white (fill the buffer with value for white)
    display.setCursor(x, y);         // set the postition to start printing text
    display.print(text);             // print some text
    // end of part executed multiple times
  }
  // tell the graphics class to transfer the buffer content (page) to the controller buffer
  // the graphics class will command the controller to refresh to the screen when the last page has been transferred
  // returns true if more pages need be drawn and transferred
  // returns false if the last page has been transferred and the screen refreshed for panels without fast partial update
  // returns false for panels with fast partial update when the controller buffer has been written once more, to make the differential buffers equal
  // (for full buffered with fast partial update the (full) buffer is just transferred again, and false returned)
  while (display.nextPage());
  // Serial.println("helloWorld done");
  // Serial.println("drawHelloWorld done");
}