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
        - void Graphics::clear();
        - void Graphics::fill(uint8_t color);
        - void Graphics::drawPixel(uint8_t x, uint8_t y, uint8_t color);
        - void Graphics::loadImage();
    */
   
    Display::refresh();
}