#include <digitalWriteFast.h>
#include <string.h>
#include <stdint.h>
#include "imageData.h"

// Pin 01   D_G         red
// Pin 03   D_R         green
// Pin 05   CLK         orange
// Pin 07   STR         yellow
// Pin 09   A0          white
// Pin 11   A1          grey
// Pin 13   A2          black
// Pin 15   CS          brown
// Pin 17   EN_R        blue
// Pin 18   EN_G        purple
// Pin 19   not used
// sonst.   GND
/*
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

uint8_t bitmap[WIDTH][HEIGHT_SECTION];

/* mode:
0 = all 0
1 = all red 1
2 = all green 1
3 = all 1 
4 = file from imageData.h
*/
void setBitmap(uint8_t mode){
    switch (mode){
        case 0: // off
            memset(bitmap,0,sizeof(bitmap));
            break;
        case 1: // red
            memset(bitmap,0b10101010,sizeof(bitmap));
            break;
        case 2: // green
            memset(bitmap,0b01010101,sizeof(bitmap));
            break;
        case 3: // yellow
            memset(bitmap,0b11111111,sizeof(bitmap));
            break;
        case 4: // image
            memcpy_P(bitmap,imageData,sizeof(bitmap));
            break; 
        default: // pixel 1 set to red, rest off
            memset(bitmap,0,sizeof(bitmap));
            memset(bitmap,0b10000000,1);
            break;
    }
}


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

    setBitmap(4); // bitmap variable overwritten in funtion
}

// Main Loop
void loop() {
    // Set whole Diplay like in the matrices defined
    for (int y = 0; y<HEIGHT_SECTION; y++) {
        if(y == 15) y = 0;
        else y +=1;
        
        //Row selection
        digitalWriteFast(PIN_A0, (y & 0x01) != 0);
        digitalWriteFast(PIN_A1, (y & 0x02) != 0);
        digitalWriteFast(PIN_A2, (y & 0x04) != 0);
        digitalWriteFast(PIN_CS, y >= 8);

        for(int x = 0; x<WIDTH;x++){
            PORTA = bitmap[x][y];
            digitalWriteFast(PIN_CLK, true);
            digitalWriteFast(PIN_CLK, false);
        }

        // Latch the whole display
        digitalWriteFast(PIN_STR, true);
        digitalWriteFast(PIN_STR, false);
    }
}