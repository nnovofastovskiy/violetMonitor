/**
 * WiFiManager advanced demo, contains advanced configurartion options
 * Implements TRIGGEN_PIN button press, press for ondemand configportal, hold for 3 seconds for reset settings.
 */

// #include <FS.h>
#include <Arduino.h>
#include <main.h>
#include <display.h>
#include <GxEPD.h>

// #include <SPIFFS.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <Preferences.h>
// #include <display.h>
// #include <GxEPD2.h>
// #include <GxEPD2_BW.h>
// #include <GxEPD2_display_selection_new_style.h>
// #include <FontsRus/FreeMonoBold18.h>
// #include <FontsRus/FreeSerif10.h>
// #include <FontsRus/TimesNRCyr10.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <HTTPClient.h>
#include <FontsRus/MSSansSerif14.h>
#include <FontsRus/MSSansSerif18.h>
#include <FontsRus/MSSansSerif30.h>
#include <FontsRus/BatFont.h>

// #define GxEPD2_DRIVER_CLASS GxEPD2_290_T94_V2
#include <Adafruit_NeoPixel.h>

// #define STANDALONE

#include <GxDEPG0290BS/GxDEPG0290BS.h> // 2.9" b/w Waveshare variant, TTGO T5 V2.4.1 2.9"

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// #define TRIGGER_PIN 0
#define OPTIONS_PIN GPIO_NUM_33
#define POWER_PIN GPIO_NUM_32

#define ASIDE_WIDTH 100
#define DISPLAY_PADDING 4

#define LED_PIN GPIO_NUM_19
#define BRIGHTNESS 40 // 0 - 255

#define CHARGER_PIN GPIO_NUM_25
#define BAT_LEVEL_PIN GPIO_NUM_26

#define WAKEUP_PINS_BITMAP 0x302000000
// long WAKEUP_PINS_BITMAP = 1 << OPTIONS_PIN | 1 << POWER_PIN | 1 << CHARGER_PIN;
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
int chargerOn = false;

int startTime = 0;
int batLevel = 0;
int levelFronts = 0;
int betweenFronts = 0;
int prevMillis = 0;
bool levelReady = false;
bool levelDone = false;

bool chargeDone = false;
bool prevBatLevel;
char *batLevelStr[6] = {"0", "1", "2", "3", "4", "5"};
// RTC_DATA_ATTR volatile bool powerBtnPushed = false;

// BUSY -> 4, RST -> 16, DC -> 17, CS -> SS(5), CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V
GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/17, /*RST=*/16); // arbitrary selection of 17, 16
GxEPD_Class display(io, /*RST=*/16, /*BUSY=*/4);        // arbitrary selection of (16), 4

Adafruit_NeoPixel pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800);

void IRAM_ATTR optionsIsr()
{
  http.end();
  optionsBtnPressed = true;
}

void IRAM_ATTR powerIsr()
{
  Serial.print("POWER_PIN = ");
  Serial.println(digitalRead(POWER_PIN));
  if (digitalRead(POWER_PIN))
  {
    // Serial.print("========powerBtnPushed = ");
    // Serial.println(powerBtnPushed);
    turnOffFlag = true;
    esp_sleep_enable_timer_wakeup(1);
    esp_deep_sleep_start();
  }
}

void IRAM_ATTR chargerIsr()
{
  if (digitalRead(CHARGER_PIN))
  {
    chargerOn = true;
  }
  else
  {
    chargerOn = false;
    esp_sleep_enable_timer_wakeup(1);
    esp_deep_sleep_start();
  }
}

// hw_timer_t

void IRAM_ATTR batLeverIsr()
{
  delayMicroseconds(100);
  int level = digitalRead(BAT_LEVEL_PIN);
  // Serial.print("********** batLeverIsr______");
  // Serial.println(digitalRead(BAT_LEVEL_PIN));
  // delay(1);
  if (level == 0)
  {

    int time = millis();
    int dt = time - prevMillis;
    if (!levelDone)
    {

      if (levelFronts == 0)
      {
        levelFronts++;
      }
      else
      {
        if (dt > 1000)
        {
          if (levelReady)
          {
            levelDone = true;
            batLevel = levelFronts;
            detachInterrupt(BAT_LEVEL_PIN);
          }
          else
          {
            levelReady = true;
            levelFronts = 1;
          }
        }
        else
        {
          if (levelReady)
            levelFronts++;
        }
      }
      prevMillis = time;
      // Serial.print("**************** levelFronts = ");
      // Serial.println(levelFronts);
      // Serial.print("**************** levelReady = ");
      // Serial.println(levelReady);
      // Serial.print("**************** levelDone = ");
      // Serial.println(levelDone);
      // Serial.print("**************** dt = ");
      // Serial.println(dt);
    }
  }
}

// void IRAM_ATTR powerIsrPush()
// {
//   Serial.print("========powerBtnPushed = ");
//   Serial.println(powerBtnPushed);
//   powerBtnPushed = true;
// }

// void handleClose()
// {
//   Serial.println("========= Web portal CLOSE");
//   wm.handleClose();
// }

void saveWifiCallback()
{
  Serial.println("[CALLBACK] saveCallback fired");
}

void timeoutCallback()
{
  Serial.println("TURNING OFF BY timeout");
  turningOff();
  // pixels.clear();
  // pixels.show();
  // String text = "Выключено";
  // drawStatusText(&display, strToChar(utf8rus(text)), &MSSansSerif14);
  // esp_sleep_enable_ext1_wakeup(0x100000000, ESP_EXT1_WAKEUP_ANY_HIGH); // 1 = High, 0 = Low
  // esp_deep_sleep_start();
}

void handleExit()
{
  Serial.println("========= Web portal CLOSE");
  wm.handleExit();
  wm.stopWebPortal();
  esp_restart();
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  String text = "Точка доступа";
  drawStatusText(&display, strToChar(utf8rus(text)), &MSSansSerif14);
  Serial.println("[CALLBACK] configModeCallback fired");
  // myWiFiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  // Serial.println(WiFi.softAPIP());
  // if you used auto generated SSID, print it
  // Serial.println(myWiFiManager->getConfigPortalSSID());
  //
  // esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
}

// void saveParamCallback()
// {
//   Serial.println("[CALLBACK] saveParamCallback fired");
//   // wm.stopConfigPortal();
// }

void bindServerCallback()
{
  // wm.server->on("/custom",handleRoute);

  // you can override wm route endpoints, I have not found a way to remove handlers, but this would let you disable them or add auth etc.
  // wm.server->on("/info",handleNotFound);
  // wm.server->on("/update",handleNotFound);
  // wm.server->on("/erase",handleNotFound); // disable erase
  wm.server->on("/exit", handleExit);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("_____START_____");
#ifndef STANDALONE
  check_wakeup_reason();
  delay(100);
  Serial.print(" +++++++++++++++++ turnOffFlag = ");
  Serial.println(turnOffFlag);
  if (turnOffFlag)
  {
    Serial.println("TURNING OFF BY powerBtnPressed");
    turningOff();
  }
  pinMode(CHARGER_PIN, INPUT);
  pinMode(BAT_LEVEL_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(POWER_PIN), powerIsr, RISING);
  attachInterrupt(digitalPinToInterrupt(OPTIONS_PIN), optionsIsr, RISING);
  attachInterrupt(digitalPinToInterrupt(CHARGER_PIN), chargerIsr, CHANGE);
#endif
  // drawBat(&display, batLevelStr[5], &BatFont, false);
  attachInterrupt(BAT_LEVEL_PIN, batLeverIsr, CHANGE);

  // while (!levelDone)
  // {
  //   delay(1);
  // }
  // Serial.print("========= batLevel = ");
  // Serial.println(batLevel);

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  // esp_sleep_enable_ext0_wakeup(OPTIONS_PIN, 1); // 1 = High, 0 = Low
  chargerOn = digitalRead(CHARGER_PIN);
  Serial.print("chargerOn = ");
  Serial.print(digitalRead(CHARGER_PIN));
  Serial.print("   ");
  Serial.println(chargerOn);
  // delay(3000);
  // attachInterrupt(POWER_PIN, powerIsrOff, FALLING);
  // while(powerBtnPushed);

  esp_sleep_enable_ext1_wakeup(WAKEUP_PINS_BITMAP, ESP_EXT1_WAKEUP_ANY_HIGH); // 1 = High, 0 = Low
  esp_sleep_enable_timer_wakeup(15e6);
  Serial.println("\n Starting");

  configWM();

  preferences.begin("preferences", false);
  url = preferences.getString("url");
  if (url == "")
  {
    String text = "URL пуст";
    drawStatusText(&display, strToChar(utf8rus(text)), &MSSansSerif14);
    if (!wm.startConfigPortal("VioletMonitor", ""))
    {
      turningOff();
    }
    ESP.restart();
  }
  preferences.end();
  Serial.println("URL = " + url);

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
      // Serial.println(payload);
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

      while (!levelDone)
      {
        delay(1);
      }
      Serial.print("\n========= batLevel = ");
      Serial.println(batLevel);

      const char *timeString = strToChar(utf8rus(doc["timeString"].as<String>()));
      const char *aside = strToChar(utf8rus(doc["aside"].as<String>()));
      const char *line1 = strToChar(utf8rus(doc["line1"].as<String>()));
      const char *line2 = strToChar(utf8rus(doc["line2"].as<String>()));
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
      drawAsideText(&display, aside, &MSSansSerif30);
      drawTimeText(&display, timeString, &MSSansSerif14, false);
      drawLine1(&display, line1, &MSSansSerif14, false);
      drawLine2(&display, line2, &MSSansSerif14, false);
      if (chargerOn)
      {
        drawBat(&display, batLevelStr[5], &BatFont, false);
      }
      else
      {
        drawBat(&display, batLevelStr[batLevel], &BatFont, false);
      }
      // switch (batLevel)
      // {
      // case 1:
      //   drawBat(&display, "1", &BatFont, false);
      //   break;
      // case 2:
      //   drawBat(&display, "2", &BatFont, false);
      //   break;
      // case 3:
      //   drawBat(&display, "3", &BatFont, false);
      //   break;
      // case 4:
      //   drawBat(&display, "4", &BatFont, false);
      //   break;

      // default:
      //   break;
      // }
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
  }
  if (!chargerOn)
  {
    esp_deep_sleep_start();
  }
  else
  {
    delay(15000);
    esp_restart();
  }
}

void loop()
{
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

void check_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;
  uint64_t wake_pin;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  Serial.println("===========WAKE UP");
  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
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
    if (wake_pin == POWER_PIN)
    {
      turnOffFlag = !turnOffFlag;
      Serial.print("========== turnOffFlag = ");
      Serial.println(turnOffFlag);
      while (digitalRead(POWER_PIN))
      {
      }
    }
    if (wake_pin == CHARGER_PIN)
    {
      chargerOn = true;
    }
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

String utf8rus(String source)
{
  int i, k;
  String target;
  unsigned char n;
  char m[2] = {'0', '\0'};
  k = source.length();
  i = 0;
  while (i < k)
  {
    n = source[i];
    i++;
    if (n >= 0xC0)
    {
      switch (n)
      {
      case 0xD0:
      {
        n = source[i];
        i++;
        if (n == 0x81)
        {
          n = 0xA8;
          break;
        }
        if (n >= 0x90 && n <= 0xBF)
          n = n + 0x30;
        break;
      }
      case 0xD1:
      {
        n = source[i];
        i++;
        if (n == 0x91)
        {
          n = 0xB8;
          break;
        }
        if (n >= 0x80 && n <= 0x8F)
          n = n + 0x70;
        break;
      }
      }
    }
    m[0] = n;
    target = target + String(m);
  }
  return target;
}

const char *strToChar(String str)
{
  const int length = str.length();
  char *char_array = new char[length + 1];

  // copying the contents of the
  // string to char array
  strcpy(char_array, str.c_str());
  const char *out = char_array;
  return out;
}

void configWM()
{
  wm.setWebServerCallback(bindServerCallback);
  wm.setSaveConfigCallback(saveWifiCallback);
  wm.setAPCallback(configModeCallback);
  wm.setSaveConfigCallback(saveWifiCallback);
  wm.setConfigPortalTimeoutCallback(timeoutCallback);
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

  wm.setConnectTimeout(10);      // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(30); // auto close configportal after n seconds
  // wm.setCaptivePortalEnable(false); // disable captive portal redirection
  wm.setAPClientCheck(true); // avoid timeout if client connected to softap

  // wifi scan settings
  // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
  // wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
  // wm.setShowInfoErase(false);      // do not show erase button on info page
  // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons

  // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails
  // bool isSaved = wm.getWiFiIsSaved();
  // String savedSSID = wm.getWiFiSSID(true);
}

void turningOff()
{
  turnOffFlag = true;
  String text = "Выключено";
  pixels.clear();
  pixels.show();
  drawTurnOff(&display, strToChar(utf8rus(text)), &MSSansSerif14);
  esp_sleep_enable_ext1_wakeup(0x100000000, ESP_EXT1_WAKEUP_ANY_HIGH); // 1 = High, 0 = Low
  esp_deep_sleep_start();
}