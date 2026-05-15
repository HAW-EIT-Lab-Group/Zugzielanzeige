#pragma once

#include "header.h"

void pinSetup();

void pulseClock();
void pulseLatch();

void setRow(int row);

void writeToDisplay(
    bool pixels_red[SIZE_X][SIZE_Y],
    bool pixels_green[SIZE_X][SIZE_Y]
);

void clearDisplay();

void drawPixel(
    bool pixels_red[SIZE_X][SIZE_Y],
    bool pixels_green[SIZE_X][SIZE_Y],
    int x,
    int y,
    bool red,
    bool green
);