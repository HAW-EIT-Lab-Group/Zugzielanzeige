#include <Arduino.h>
#include "Graphics.h"
#include "config.h"
#include "imageData.h"

uint8_t Graphics::bitmap[WIDTH][HEIGHT_SECTION];

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
void Graphics::drawPixel(uint8_t x, uint8_t y64, uint8_t color){
    uint8_t yshift,y16;
    yshift = ((NR_OF_SECTIONS-1) - y64/16) * 2; // calculate bit shift amount for area corresponding to section, *2 because 1bit/pixel
    y16 = y64 % HEIGHT_SECTION;
    bitmap[x][y16] = (bitmap[x][y16] & ~(0b11<<yshift)) | color<<yshift; // copy row, delete pixel, insert new pixel     
}

// original: fill rectangle with color
/* 
void Graphics::drawRect(uint8_t x1, uint8_t y1,uint8_t x2, uint8_t y2, uint8_t color){
    for(int x = 0;x<WIDTH;x++){
        for(int y = 0;y<HEIGHT_ALL;y++){
            if(x>=x1 && x<=x2 && y>=y1 && y<=y2){
                drawPixel(x,y,color);
            }
        }
    }
}
 */


 /*
 Prompt:

I have the following code to set bits in a larger matrix. The matrix has a weird but mandatory construction. uint8_t bitmap[200][16] is storing data for leds of a 64x200 led matrix. The matrix is split into 4 sections with 16 pixels each. Each pixel has two leds with the colors red and green.

Each byte of bitmap holds the data for one pixel in each section, eg. bitmap[0][0] has 8 bit:
Bit 7 hold data for the red led of pixel 0,0 of section 0
Bit 6 hold data for the green led of pixel 0,0 of section 0
Bit 5 holds data for the red led of pixel 0,0 of section 1
bit 4 hold data for the green led of pixel 0,0 of section 1
and so on.

I want you to rewrite the drawRect(x1,y1,x2,y1,color) function to allow for faster drawing without repeatedly calling drawPixel().
 
*/
// ChatGpt1: better fill rectangle with color
 
void Graphics::drawRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color){
    // koordinaten normalisieren
    if(x1 > x2){
        uint8_t t = x1;
        x1 = x2;
        x2 = t;
    }

    if(y1 > y2){
        uint8_t t = y1;
        y1 = y2;
        y2 = t;
    }

    for(uint8_t x = x1; x <= x2; ++x){
        for(uint8_t y=y1;y<=y2;++y){
            uint8_t section = y / HEIGHT_SECTION;
            uint8_t y16 = y % HEIGHT_SECTION;

            uint8_t yshift = ((NR_OF_SECTIONS - 1) - section) * 2;
            uint8_t mask   = 0b11 << yshift;

            bitmap[x][y16] = (bitmap[x][y16] & ~mask) | (color << yshift);
        }
    }
}


// ChatGpt2: better fill rectangle with color (theoretisch noch schneller, da mehrere reihen zur zeit processed werden)
/* 
void Graphics::drawRect(uint8_t x1, uint8_t y1,
                        uint8_t x2, uint8_t y2,
                        uint8_t color)
{
    if (x1 > x2) {
        uint8_t t = x1;
        x1 = x2;
        x2 = t;
    }

    if (y1 > y2) {
        uint8_t t = y1;
        y1 = y2;
        y2 = t;
    }

    for (uint8_t x = x1; x <= x2; ++x) {

        uint8_t y = y1;

        while (y <= y2) {
            uint8_t section = y / HEIGHT_SECTION;

            // Last row belonging to this 16-pixel section
            uint8_t sectionEnd = ((section + 1) * HEIGHT_SECTION) - 1;

            if (sectionEnd > y2)
                sectionEnd = y2;

            uint8_t y16 = y % HEIGHT_SECTION;

            uint8_t yshift = ((NR_OF_SECTIONS - 1) - section) * 2;

            uint8_t mask = 0b11 << yshift;
            uint8_t value = color << yshift;

            // Fill all rows in this section
            while (y <= sectionEnd) {
                bitmap[x][y16] = (bitmap[x][y16] & ~mask) | value;
                ++y;
                ++y16;
            }
        }
    }
}
 */

//ChatGpt3: noch schneller (laut ChatGpt)
/* 
void Graphics::drawRect(uint8_t x1, uint8_t y1,
                        uint8_t x2, uint8_t y2,
                        uint8_t color)
{
    if (x1 > x2) {
        uint8_t t = x1;
        x1 = x2;
        x2 = t;
    }

    if (y1 > y2) {
        uint8_t t = y1;
        y1 = y2;
        y2 = t;
    }

    for (uint8_t x = x1; x <= x2; ++x) {

        // Process each 16-pixel section touched by the rectangle
        uint8_t firstSection = y1 / HEIGHT_SECTION;
        uint8_t lastSection  = y2 / HEIGHT_SECTION;

        for (uint8_t section = firstSection;
             section <= lastSection;
             ++section)
        {
            uint8_t firstY = (section == firstSection)
                           ? y1 % HEIGHT_SECTION
                           : 0;

            uint8_t lastY = (section == lastSection)
                          ? y2 % HEIGHT_SECTION
                          : HEIGHT_SECTION - 1;

            uint8_t shift =
                ((NR_OF_SECTIONS - 1) - section) * 2;

            uint8_t mask = 0b11 << shift;
            uint8_t value = color << shift;

            for (uint8_t y16 = firstY; y16 <= lastY; ++y16) {
                bitmap[x][y16] =
                    (bitmap[x][y16] & ~mask) | value;
            }
        }
    }
}
 */



// copy image stored in imageData.h to matrix
void Graphics::loadImage(){
    memcpy_P(bitmap,imageData,sizeof(bitmap));
}