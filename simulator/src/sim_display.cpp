// ---------------------------------------------------------------------------
// Ersatz fuer lib/Display/Display.cpp: statt Schieberegister/GPIO wird der
// Bitmap-Inhalt nur gezaehlt - das Zeichnen uebernimmt sim_main.cpp.
// Die Bitverteilung ist exakt die des echten Panels (siehe Graphics::drawPixel).
// ---------------------------------------------------------------------------
#include <stdint.h>

#include "config.h"
#include "Graphics.h"
#include "Display.h"
#include "sim_display.h"

namespace
{
    unsigned long g_refreshs = 0;
}

void Display::init() {}

void Display::refresh() { g_refreshs++; }

void Display::enable(uint8_t) {}

unsigned long SimDisplay::refreshZaehler() { return g_refreshs; }

uint8_t SimDisplay::pixelFarbe(int x, int y, bool yfixWieRefresh)
{
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT_ALL) return 0;

    int zeile = y % HEIGHT_SECTION;

    // Display::refresh() waehlt Zeile y an, schiebt aber bitmap[x][y+1] heraus
    // (y == 15 -> 0). Optional nachbilden, um Zeilenversatz zu untersuchen.
    if (yfixWieRefresh) zeile = (zeile == HEIGHT_SECTION - 1) ? 0 : zeile + 1;

    // gleiche Rechnung wie in Graphics::drawPixel(), nur rueckwaerts
    int shift = ((NR_OF_SECTIONS - 1) - (y / HEIGHT_SECTION)) * 2;

    return (uint8_t)((Graphics::bitmap[x][zeile] >> shift) & 0x03);
}
