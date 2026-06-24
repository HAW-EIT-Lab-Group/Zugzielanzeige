#include <Arduino.h>
#include "Display.h"
#include "Graphics.h"
#include "config.h"

void setup(){
    Display::init();

    Graphics::loadImage();
}

void loop(){
    /* 
    fill matrix here using 
        - void clear();
        - void fill(uint8_t color);
        - void drawPixel(uint8_t x, uint8_t y, uint8_t color);
        - void loadImage();
    */
   
    Display::refresh();
}