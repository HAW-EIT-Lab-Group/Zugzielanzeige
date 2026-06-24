/*----------------------------------------------------------------------------
 * gfx.h  -  tiny drawing helpers + 5x7 text on top of the framebuffer.
 *--------------------------------------------------------------------------*/
#pragma once

#include "display.h"

// Colours (bit0 = red, bit1 = green).
enum { CLR_BLACK = 0, CLR_RED = 1, CLR_GREEN = 2, CLR_ORANGE = 3 };

void gfxFillRect(int x, int y, int w, int h, uint8_t c);
void gfxRect(int x, int y, int w, int h, uint8_t c);
void gfxHLine(int x, int y, int w, uint8_t c);
void gfxVLine(int x, int y, int h, uint8_t c);

void gfxChar(int x, int y, char ch, uint8_t c);   // 5x7 glyph, top-left at x,y
int  gfxTextWidth(const char *s);                  // pixels (5px glyph + 1 gap)
int  gfxText(int x, int y, const char *s, uint8_t c);
void gfxTextCentered(int y, const char *s, uint8_t c);
