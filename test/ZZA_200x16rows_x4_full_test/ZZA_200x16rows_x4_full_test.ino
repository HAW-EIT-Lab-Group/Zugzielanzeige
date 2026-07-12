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

https://docs.arduino.cc/resources/pinouts/A000067-full-pinout.pdf
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

PIN_CS D46 (PL3)
PIN_A2 D47 (PL2)
PIN_A1 D48 (PL1)
PIN_A0 D49 (PL0)
*/

// Global for all Sections
#define PIN_CLK 38
#define PIN_STR 40
#define PIN_EN_R 11
#define PIN_EN_G 12


// Panel Measurements
#define WIDTH 200
#define HEIGHT_SECTION 16
#define HEIGHT_ALL 64
#define NR_OF_PXL_SECTION WIDTH*HEIGHT_SECTION

// other
#define GREEN 1
#define RED 2
#define YELLOW 3

uint8_t bitmap[WIDTH][HEIGHT_SECTION];

/* mode:
0 = all 0
1 = all red 1
2 = all green 1
3 = all 1 
4 = file from imageData.h
*/
void fillMatrix(uint8_t color){
        uint8_t value = 0;
        value = (color<<6) |
                (color<<4) |
                (color<<2) |
                color;
        memset(bitmap,value,sizeof(bitmap));
}

void pictureToMatrix(){
    memcpy_P(bitmap,imageData,sizeof(bitmap));
}

// Inital setup
void setup() {
    // GPIO
    DDRA = 0xff; // set pins (PA0-PA7) output (ATmega640-1280-1281-2560-2561-Datasheet-DS40002211A.pdf, p.68)
    PORTA = 0x00; // off by default

    DDRL |= 0x0f; // pins PL7, PL5, PL3, PL1 (CS, A2, A1, A0) as output
    PORTL &= ~0x0f;  // off by default

    pinModeFast(PIN_CLK, OUTPUT);
    pinModeFast(PIN_STR, OUTPUT);
    pinModeFast(PIN_EN_R, OUTPUT);
    pinModeFast(PIN_EN_G, OUTPUT);

    // Inital Values
    analogWrite(PIN_EN_R, 0);  // active LOW  -> 100% Brightness = 0
    analogWrite(PIN_EN_G, 0);  //             -> 0% Brightness = 255

    //fillMatrix(YELLOW); // bitmap variable overwritten in funtion
    pictureToMatrix();
}

// Main Loop
void loop() {
    // Set whole Diplay like in the matrices defined
    for (int y = 0; y<HEIGHT_SECTION; y++) {
        uint8_t yfix;
        if(y == 15) yfix = 0;
        else yfix = y + 1;
        
        //Row selection
        PORTL = (PORTL & 0xf0) | (0x0f & y); // clear bits, copy masked y into register

        for(int x = 0; x<WIDTH;x++){
            PORTA = bitmap[x][yfix];
            digitalWriteFast(PIN_CLK, true);
            digitalWriteFast(PIN_CLK, false);
        }

        // Latch the whole display
        digitalWriteFast(PIN_STR, true);
        digitalWriteFast(PIN_STR, false);
    }
    /*
    analogWrite(PIN_EN_R, 255);  // active LOW  -> 100% Brightness = 0
    analogWrite(PIN_EN_G, 255);  //             -> 0% Brightness = 255
    delay(15);
    analogWrite(PIN_EN_R, 0);  // active LOW  -> 100% Brightness = 0
    analogWrite(PIN_EN_G, 0);  //             -> 0% Brightness = 255
    */
}