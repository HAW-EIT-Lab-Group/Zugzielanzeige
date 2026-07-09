#pragma once
#include <stdint.h>
#include "config.h"

namespace Graphics{
    extern uint8_t bitmap[WIDTH][HEIGHT];
    // set whole bitmap to BLACK, overwrites current values
    void clear();

    // fill bitmap with color, overwrites current values
    // (<color>)
    // BLACK = 0
    // GREEN = 1
    // RED = 2
    // YELLOW = 3
    void fill(uint8_t color);

    // set a single pixel to color, overwrites current value
    // (<x coord>, <y coord>, <color>)
    // BLACK = 0
    // GREEN = 1
    // RED = 2
    // YELLOW = 3
    void drawPixel(uint8_t x, uint8_t y, uint8_t color);

    // set a rectange to color, overwrites current values
    // can be used to draw horizontal and vertical lines
    // (<x coord upper left corner>, <y coordupper left corner>,<x coord lower right corner>, <y coord lower right corner>, <color>)
    // BLACK = 0
    // GREEN = 1
    // RED = 2
    // YELLOW = 3
    void drawRect(uint8_t x1, uint8_t y1,uint8_t x2, uint8_t y2, uint8_t color)

    // loads image store in data/imageData.h into matrix
    void loadImage();
}