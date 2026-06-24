/*----------------------------------------------------------------------------
 * gfx.cpp  -  drawing helpers + a compact 5x7 font (lives in flash/PROGMEM).
 *
 * Each glyph = 7 bytes (one per row). Within a byte, columns 0..4 map to mask
 * (0x10 >> col), so the binary literals below read left-to-right as drawn.
 *--------------------------------------------------------------------------*/
#include <avr/pgmspace.h>
#include "gfx.h"

// Supported characters, in the same order as FONT[] below.
static const char FONT_CHARS[] = " -!0123456789ACDEGHILMNOPRSTUVY";

static const uint8_t FONT[][7] PROGMEM = {
    {0,0,0,0,0,0,0},                                  // ' '
    {0,0,0,0b01110,0,0,0},                            // '-'
    {0b00100,0b00100,0b00100,0b00100,0b00100,0,0b00100}, // '!'
    {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110}, // '0'
    {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110}, // '1'
    {0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111}, // '2'
    {0b01110,0b10001,0b00001,0b00110,0b00001,0b10001,0b01110}, // '3'
    {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010}, // '4'
    {0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110}, // '5'
    {0b01110,0b10000,0b10000,0b11110,0b10001,0b10001,0b01110}, // '6'
    {0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000}, // '7'
    {0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110}, // '8'
    {0b01110,0b10001,0b10001,0b01111,0b00001,0b00001,0b01110}, // '9'
    {0b01110,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001}, // 'A'
    {0b01110,0b10001,0b10000,0b10000,0b10000,0b10001,0b01110}, // 'C'
    {0b11100,0b10010,0b10001,0b10001,0b10001,0b10010,0b11100}, // 'D'
    {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b11111}, // 'E'
    {0b01110,0b10001,0b10000,0b10111,0b10001,0b10001,0b01110}, // 'G'
    {0b10001,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001}, // 'H'
    {0b01110,0b00100,0b00100,0b00100,0b00100,0b00100,0b01110}, // 'I'
    {0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b11111}, // 'L'
    {0b10001,0b11011,0b10101,0b10101,0b10001,0b10001,0b10001}, // 'M'
    {0b10001,0b11001,0b10101,0b10101,0b10011,0b10001,0b10001}, // 'N'
    {0b01110,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110}, // 'O'
    {0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000}, // 'P'
    {0b11110,0b10001,0b10001,0b11110,0b10100,0b10010,0b10001}, // 'R'
    {0b01111,0b10000,0b10000,0b01110,0b00001,0b00001,0b11110}, // 'S'
    {0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b00100}, // 'T'
    {0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110}, // 'U'
    {0b10001,0b10001,0b10001,0b10001,0b10001,0b01010,0b00100}, // 'V'
    {0b10001,0b10001,0b01010,0b00100,0b00100,0b00100,0b00100}, // 'Y'
};

void gfxFillRect(int x, int y, int w, int h, uint8_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            dispSetPixel(x + i, y + j, c);
}

void gfxRect(int x, int y, int w, int h, uint8_t c) {
    gfxHLine(x, y, w, c);
    gfxHLine(x, y + h - 1, w, c);
    gfxVLine(x, y, h, c);
    gfxVLine(x + w - 1, y, h, c);
}

void gfxHLine(int x, int y, int w, uint8_t c) {
    for (int i = 0; i < w; i++) dispSetPixel(x + i, y, c);
}

void gfxVLine(int x, int y, int h, uint8_t c) {
    for (int j = 0; j < h; j++) dispSetPixel(x, y + j, c);
}

static int charIndex(char ch) {
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    for (int i = 0; FONT_CHARS[i]; i++)
        if (FONT_CHARS[i] == ch) return i;
    return -1;
}

void gfxChar(int x, int y, char ch, uint8_t c) {
    int idx = charIndex(ch);
    if (idx < 0) return;
    for (int row = 0; row < 7; row++) {
        uint8_t bits = pgm_read_byte(&FONT[idx][row]);
        for (int col = 0; col < 5; col++)
            if (bits & (0x10 >> col))
                dispSetPixel(x + col, y + row, c);
    }
}

int gfxTextWidth(const char *s) {
    int n = 0;
    while (*s++) n++;
    return n > 0 ? n * 6 - 1 : 0;   // 5px glyph + 1px gap, minus trailing gap
}

int gfxText(int x, int y, const char *s, uint8_t c) {
    int cx = x;
    while (*s) {
        gfxChar(cx, y, *s++, c);
        cx += 6;
    }
    return cx - 1;
}

void gfxTextCentered(int y, const char *s, uint8_t c) {
    int w = gfxTextWidth(s);
    gfxText((DISP_W - w) / 2, y, s, c);
}
