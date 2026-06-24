#pragma once
#include <stdint.h>
#include "config.h"

namespace Graphics{
    extern uint8_t bitmap[WIDTH][HEIGHT];

    void clear();
    void fill(uint8_t color);
    void drawPixel(uint8_t x, uint8_t y, uint8_t color);
    void loadImage();
}