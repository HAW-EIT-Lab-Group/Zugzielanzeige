#include <Arduino.h>
#include "Graphics.h"
#include "Config.h"

uint8_t bitmap[WIDTH][HEIGHT];

// completly clear matrix
void Graphics::clear(){
    Graphics::fill(BLACK);
}

// fill matrix with one color
void Graphics::fill(uint8_t color){
    uint8_t value = 0;
    value = (color<<6) |
            (color<<4) |
            (color<<2) |
            color;
    memset(bitmap,value,sizeof(bitmap));
}

// set selected pixel to selected color, keep the rest 
void Graphics::drawPixel(uint8_t x, uint8_t y, uint8_t color){
    uint8_t yshift,y16;
    yshift = ((NR_OF_SECTIONS-1) - y64/16) * 2; // calculate bit shift amount for area corresponding to section, *2 beacause 1bit/pixel
    y16 = y64%HEIGHT_SECTION;
    matrix[x][y16] = (matrix[x][y16] & ~(0b11<<yshift)) | color<<yshift; // copy row, delete pixel, insert new pixel     
}

// fill rectangle with color
void Graphics::drawRect(uint8_t x1, uint8_t y1,uint8_t x2, uint8_t y2, uint8_t color){
    for(int x = 0;x<WIDTH;x++){
        for(int y = 0;y<HEIGHT_ALL;y++){
            if(x>=x1 && x<=x2 && y>=y1 && y<=y2){
                drawPixel(x,y,color);
            }
        }
    }
}

// copy image stored in imageData.h to matrix
void Graphics::loadImage(){
    memcpy_P(bitmap,imageData,sizeof(bitmap));
}