#include <Arduino.h>
#include "Display.h"
#include "config.h"
#include "digitalWriteFast.h"
#include "Graphics.h"
/*
// Flachbandkabel
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

// Arduino
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

void Display::init(){
    // GPIO
    DDRA = 0xff; // set pins (PA0-PA7) output (ATmega640-1280-1281-2560-2561-Datasheet-DS40002211A.pdf, p.68)
    PORTA = 0x00; // off by default

    DDRL |= 0x0f; // pins PL7, PL5, PL3, PL1 (CS, A2, A1, A0) as output
    PORTL &= ~0x0f;  // off by default

    pinModeFast(PIN_CLK, OUTPUT);
    pinModeFast(PIN_STR, OUTPUT);
    pinModeFast(PIN_EN_R, OUTPUT);
    pinModeFast(PIN_EN_G, OUTPUT);
}

// Set whole Diplay to the values from bitmap
void Display::refresh(){
    Display::enable(0,0); // turn display on
    for (int y = 0; y<HEIGHT_SECTION; y++) {
        uint8_t yfix;
        if(y == 15) yfix = 0;
        else yfix = y + 1;
        
        //Row selection
        PORTL = (PORTL & 0xf0) | (0x0f & y); // clear bits, copy masked y into register

        for(int x = 0; x<WIDTH;x++){
            PORTA = Graphics::bitmap[x][yfix];
            digitalWriteFast(PIN_CLK, true);
            digitalWriteFast(PIN_CLK, false);
        }

        // Latch the whole display
        digitalWriteFast(PIN_STR, true);
        digitalWriteFast(PIN_STR, false);

    }
    Display::enable(1,1); // turn off while calculating other things, prevents last line from looking brighter
}

// enable(red,green), active LOW -> 1 = off, 0 = on
// example: enable(0,1) -> red on, green off
void Display::enable(uint8_t r,uint8_t g){
    digitalWriteFast(PIN_EN_R,r);
    digitalWriteFast(PIN_EN_G,g);
}