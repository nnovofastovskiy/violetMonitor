/**
 * WiFiManager advanced demo, contains advanced configurartion options
 * Implements TRIGGEN_PIN button press, press for ondemand configportal, hold for 3 seconds for reset settings.
 */

// #include <FS.h>
#include <Arduino.h>
#include <main.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <HTTPClient.h>
// #include <SPIFFS.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <Preferences.h>
// #include <display.h>
#include <GxEPD.h>
// #include <display.h>

#include <GxDEPG0290BS/GxDEPG0290BS.h> // 2.9" b/w Waveshare variant, TTGO T5 V2.4.1 2.9"

#include GxEPD_BitmapExamples

// FreeFonts from Adafruit_GFX
// #include <Fonts/FreeMonoBold9pt7b.h>
// #include <Fonts/FreeMonoBold12pt7b.h>
// #include <Fonts/FreeMonoBold18pt7b.h>
// #include <Fonts/FreeMonoBold24pt7b.h>
// #include <Fonts/FreeMono9pt7b.h>
// #include <Fonts/FreeSerif9pt7b.h>
#include <FontsRus/FreeMonoBold18.h>
#include <FontsRus/FreeSerif10.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// #define TRIGGER_PIN 0
#define WAKE_UP_PIN GPIO_NUM_33

#define ASIDE_WIDTH 100
#define DISPLAY_PADDING 4

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

bool btnPressed = false;

void IRAM_ATTR isr()
{
  http.end();
  btnPressed = true;
  uint32_t num = 0;
  for (int i = 0; i < 1e6; i++)
  {
    if (digitalRead(WAKE_UP_PIN))
    {
      num++;
    }
    else
    {
      num--;
    }
  }
  //  checkButton();
}

GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/17, /*RST=*/16); // arbitrary selection of 17, 16
GxEPD_Class display(io, /*RST=*/16, /*BUSY=*/4);        // arbitrary selection of (16), 4

void setup()
{
  pinMode(WAKE_UP_PIN, INPUT_PULLDOWN);
  // pinMode(TRIGGER_PIN, INPUT_PULLUP);

  esp_sleep_enable_timer_wakeup(15e6);
  esp_sleep_enable_ext0_wakeup(WAKE_UP_PIN, 1); // 1 = High, 0 = Low

  Serial.begin(115200);
  delay(3000);
  attachInterrupt(WAKE_UP_PIN, isr, RISING);
  check_wakeup_reason();

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
    if (httpResponseCode > 0)
    {
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
      drawAsideText(aside);
      drawTimeText(timeString);
      drawLine1(line1);
      drawLine2(line2);
      display.update();
      display.powerDown();
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

void checkButton()
{
  // check for button press
  if (digitalRead(WAKE_UP_PIN) == HIGH)
  {
    // poor mans debounce/press-hold, code not ideal for production
    delayMicroseconds(50e3);
    if (digitalRead(WAKE_UP_PIN) == HIGH)
    {
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      // delayMicroseconds(3000e3); // reset delay hold
      // if (digitalRead(WAKE_UP_PIN) == HIGH)
      // {
      //   Serial.println("Button Held");
      //   Serial.println("Erasing Config, restarting");
      //   wm.resetSettings();
      //   preferences.begin("preferences", false);
      //   preferences.putString("url", "");
      //   preferences.end();
      //   ESP.restart();
      // }

      // start portal w delay
      Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(120);

      bool res;
      // res = wm.autoConnect(); // auto generated AP name from chipid
      // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
      res = wm.startConfigPortal("VioletMonitor", "");

      if (!res)
      {
        Serial.println("Failed to connect or hit timeout");
        // ESP.restart();
      }
      else
      {
        // if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
      }
    }
  }
}

void loop()
{
  if (wm_nonblocking)
    wm.process(); // avoid delays() in loop when non-blocking and other long running code
  if (btnPressed)
  {
    Serial.println("FROM LOOP BUTTON PRESSED");
    checkButton();
    btnPressed = false;
  }
  // put your main code here, to run repeatedly:
}

void check_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
  {
    Serial.println("Wakeup caused by external signal using RTC_IO");
    checkButton();
    break;
  }
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
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
