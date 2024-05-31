// #include <GxEPD.h>
#include <GxDEPG0290BS/GxDEPG0290BS.h>
#include <display.h>

#define DISPLAY_PADDING 4
#define ASIDE_WIDTH 100

void drawAsideText(GxDEPG0290BS *display, const char *string, const GFXfont *font)
{
    display->setRotation(3);
    display->setFont(font);
    display->setTextColor(GxEPD_WHITE);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display->getTextBounds(string, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((ASIDE_WIDTH - tbw) / 2) - tbx;
    uint16_t y = ((display->height() - tbh) / 2) - tby;
    display->fillRect(0, 0, ASIDE_WIDTH, display->height(), GxEPD_BLACK);
    display->setCursor(x, y);
    display->print(string);
}

void drawTimeText(GxDEPG0290BS *display, const char *string, const GFXfont *font)
{
    display->setRotation(3);
    display->setFont(font);
    display->setTextColor(GxEPD_BLACK);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display->getTextBounds(string, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = display->width() - tbw - DISPLAY_PADDING;
    uint16_t y = tbh + DISPLAY_PADDING;
    display->setCursor(x, y);
    display->print(string);
}
void drawLine1(GxDEPG0290BS *display, const char *string, const GFXfont *font)
{
    display->setRotation(3);
    display->setFont(font);
    display->setTextColor(GxEPD_BLACK);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display->getTextBounds(string, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ASIDE_WIDTH + DISPLAY_PADDING;
    uint16_t y = ((display->height() - tbh) / 2) - tby;
    display->setCursor(x, y);
    display->print(string);
}
void drawLine2(GxDEPG0290BS *display, const char *string, const GFXfont *font)
{
    display->setRotation(3);
    display->setFont(font);
    display->setTextColor(GxEPD_BLACK);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display->getTextBounds(string, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ASIDE_WIDTH + DISPLAY_PADDING;
    uint16_t y = display->height() - DISPLAY_PADDING;
    Serial.print("=============== display height = ");
    Serial.println(display->height());
    Serial.print("=============== y = ");
    Serial.println(y);
    Serial.print("=============== tby = ");
    Serial.println(tby);
    Serial.print("=============== tbh = ");
    Serial.println(tbh);
    display->setCursor(x, y);
    display->print(string);
}

void drawTurnOff(GxDEPG0290BS *display, const char *string, const GFXfont *font)
{
    const char text[] = "Выключено";
    display->setRotation(3);
    display->setFont(font);
    display->setTextColor(GxEPD_BLACK);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display->getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((display->width() - tbw) / 2) - tbx;
    uint16_t y = ((display->height() - tbh) / 2) - tby;
    display->update();
    display->powerDown();
}
