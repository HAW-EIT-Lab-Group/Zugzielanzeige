#pragma once
#include <stdint.h>

namespace SimDisplay
{
    // Farbe (0..3 = BLACK/GREEN/RED/YELLOW) des physikalischen Pixels (x,y)
    // auf dem 200x64-Panel, ausgelesen aus Graphics::bitmap.
    //
    // yfixWieRefresh == false (Standard):
    //     genaue Umkehrung von Graphics::drawPixel() - das ist das Bild, das
    //     das Spiel zu zeichnen glaubt.
    // yfixWieRefresh == true:
    //     zusaetzlich die Zeilenverschiebung aus Display::refresh() (Zeilen-
    //     auswahl y, ausgegeben wird aber bitmap-Zeile y+1). Zum Vergleichen,
    //     falls das echte Panel um eine Zeile versetzt aussieht.
    uint8_t pixelFarbe(int x, int y, bool yfixWieRefresh);

    // Anzahl der Aufrufe von Display::refresh() seit Programmstart
    unsigned long refreshZaehler();
}
