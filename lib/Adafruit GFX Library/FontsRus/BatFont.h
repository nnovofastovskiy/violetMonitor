const uint8_t BatFontBitmaps[] PROGMEM = {
    0xFF, 0xD0, 0x0A, 0x01, 0xC0, 0x38, 0x05, 0xFF, 0x80, 0xFF, 0xDC, 0x0B,
    0x81, 0xF0, 0x3E, 0x05, 0xFF, 0x80, 0xFF, 0xDF, 0x0B, 0xE1, 0xFC, 0x3F,
    0x85, 0xFF, 0x80, 0xFF, 0xDF, 0xCB, 0xF9, 0xFF, 0x3F, 0xE5, 0xFF, 0x80,
    0xFF, 0xDF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFD, 0xFF, 0x80, 0xFF, 0xD7, 0x7B,
    0x67, 0xF3, 0x7F, 0x75, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const GFXglyph BatFontGlyphs[] PROGMEM = {
    {0, 11, 6, 11, 0, -5},      // 0x30 '0'
    {9, 11, 6, 11, 0, -5},      // 0x31 '1'
    {18, 11, 6, 11, 0, -5},     // 0x32 '2'
    {27, 11, 6, 11, 0, -5},     // 0x33 '3'
    {36, 11, 6, 11, 0, -5},     // 0x34 '4'
    {45, 11, 6, 11, 0, -5},     // 0x35 '5'
    {54, 12, 16, 14, 1, -15},   // 0x36 '6'
    {78, 12, 16, 14, 1, -15},   // 0x37 '7'
    {102, 12, 16, 14, 1, -15},  // 0x38 '8'
    {126, 12, 16, 14, 1, -15}}; // 0x39 '9'

const GFXfont BatFont PROGMEM = {
    (uint8_t *)BatFontBitmaps,
    (GFXglyph *)BatFontGlyphs, 0x30, 0x39, 20};

// Approx. 227 bytes
