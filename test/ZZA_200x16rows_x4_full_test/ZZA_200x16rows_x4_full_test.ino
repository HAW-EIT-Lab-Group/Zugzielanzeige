#include <digitalWriteFast.h>
#include <string.h>
#include <stdint.h>
#include "imageData.h"

/*
Pin 01   D_G         red
Pin 03   D_R         green
Pin 05   CLK         orange
Pin 07   STR         yellow
Pin 09   A0          white
Pin 11   A1          grey
Pin 13   A2          black
Pin 15   CS          brown
Pin 17   EN_R        blue
Pin 18   EN_G        purple
Pin 19   not used
sonst.   GND

// Section 1
PIN_1_R D28 (PA0)
PIN_1_G D29 (PA1)

// Section 2
PIN_2_R D26 (PA2)
PIN_2_G D27 (PA3)

// Section 3
PIN_3_R D24 (PA4)
PIN_3_G D25 (PA5)

// Section 4
PIN_4_R D22 (PA6)
PIN_4_G D23 (PA7)
*/
// Global for all Sections
#define PIN_CLK 38
#define PIN_STR 40
#define PIN_EN_R 42
#define PIN_EN_G 44

#define PIN_A0 46
#define PIN_A1 48
#define PIN_A2 50
#define PIN_CS 52

// Panel Measurements
#define WIDTH 200
#define HEIGHT_SECTION 16
#define HEIGHT_ALL 64
#define NR_OF_PXL_SECTION WIDTH*HEIGHT_SECTION
#define NR_OF_SECTIONS 4

// other
#define GREEN 1
#define RED 2
#define YELLOW 3

class Bitmap{
    private:
        uint8_t matrix[WIDTH][HEIGHT_SECTION];
    public:
        // konstruktor
        Bitmap(){ 
            fill(0);
        }

        // methods
        void fill(uint8_t color){
            /* color:
            0 = 0b00 = aus
            1 = 0b01 = green
            2 = 0b10 = red
            3 = 0b11 = yellow
            */
            uint8_t value = (color<<6) | // to fill each pixel with the same r/g configuration
                            (color<<4) | // color red = 0b10 -> matrix[x][y] = 0b10101010
                            (color<<2) | 
                            color; 
            memset(matrix,value,sizeof(matrix));
        }


        // set bitmap to picture stored in imageData.h
        void setPicture(){
            memcpy_P(matrix,imageData,sizeof(matrix));
        }

        void setPixel(uint8_t x, uint8_t y64, uint8_t color){
            uint8_t yshift,y16;
            yshift = ((NR_OF_SECTIONS-1) - y64/16) * 2; // calculate bit shift amount for area corresponding to section, *2 beacause 1bit/pixel
            y16 = y64%HEIGHT_SECTION;
            matrix[x][y16] = (matrix[x][y16] & ~(0b11<<yshift)) | color<<yshift; // copy row, delete pixel, insert new pixel
        }

        uint8_t get(uint8_t x, uint8_t y){
            return matrix[x][y];
        }
};


Bitmap bitmap;


// Inital setup
void setup() {
    // GPIO
    DDRA = 0xff; // set pins (PA0-PA7) output (ATmega640-1280-1281-2560-2561-Datasheet-DS40002211A.pdf, p.68)
    PORTA = 0x00; // off by default

    pinModeFast(PIN_CLK, OUTPUT);
    pinModeFast(PIN_STR, OUTPUT);
    pinModeFast(PIN_EN_R, OUTPUT);
    pinModeFast(PIN_EN_G, OUTPUT);

    pinModeFast(PIN_A0, OUTPUT);
    pinModeFast(PIN_A1, OUTPUT);
    pinModeFast(PIN_A2, OUTPUT);
    pinModeFast(PIN_CS, OUTPUT);

    // Inital Values
    analogWrite(PIN_EN_R, 0);  // active LOW  -> 100% Brightness = 0
    analogWrite(PIN_EN_G, 0);  //             -> 0% Brightness = 255

    bitmap.setPicture(); // bitmap variable overwritten in funtion
}

// Main Loop
void loop() {
    delayMicroseconds(10); // game calc sim
    // Set whole Diplay like in the matrices defined
    for (int y = 0; y<HEIGHT_SECTION; y++) {       
        //Row selection
        digitalWriteFast(PIN_A0, (y & 0x01) != 0);
        digitalWriteFast(PIN_A1, (y & 0x02) != 0);
        digitalWriteFast(PIN_A2, (y & 0x04) != 0);
        digitalWriteFast(PIN_CS, y >= 8);

        for(int x = 0; x<WIDTH;x++){
            PORTA = bitmap.get(x,y);
            digitalWriteFast(PIN_CLK, true);
            digitalWriteFast(PIN_CLK, false);
        }

        // Latch the whole display
        digitalWriteFast(PIN_STR, true);
        digitalWriteFast(PIN_STR, false);
    }
}