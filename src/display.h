#include <GxDEPG0290BS/GxDEPG0290BS.h>

void drawAsideText(GxDEPG0290BS *display, const char *string, const GFXfont *font);

void drawTimeText(GxDEPG0290BS *display, const char *string, const GFXfont *font);

void drawLine1(GxDEPG0290BS *display, const char *string, const GFXfont *font);

void drawLine2(GxDEPG0290BS *display, const char *string, const GFXfont *font);

void drawTurnOff(GxDEPG0290BS *display, const char *string, const GFXfont *font);

void drawBat(GxDEPG0290BS *display, const char *string, const GFXfont *font, bool partial); // 1 - 25%, 2 - 50%, 3 - 75%, 4 - 100%, 5 - charging
