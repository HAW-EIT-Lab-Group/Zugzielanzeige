#pragma once

const int SIZE_X = 200; //width of led matrix
const int SIZE_Y = 64; //height of led matrix

const int S0_MAX = 15; //upper edge led on section 0
const int S1_MAX = 31; //upper edge led on section 1
const int S2_MAX = 47; //upper edge led on section 2
const int S3_MAX = 63; //upper edge led on section 3

// -------------------- PIN DEFINITIONS --------------------

//row 0 of LED matrix modules
#define PIN_DATA_G_S0   22  //data green, section 0; ribbon pin -1
#define PIN_DATA_R_S0   24  //data red, section 0; ribbon pin -1
#define PIN_A0_S0  30       //row select, bit 0, section 0; ribbon pin -1
#define PIN_A1_S0  32       //row select, bit 1, section 0; ribbon pin 11
#define PIN_A2_S0  34       //row select, bit 2, section 0; ribbon pin 13
#define PIN_CS_S0  36       //chip select, 0-7/8-15, section 0; ribbon pin 15

//row 1 of LED matrix modules
#define PIN_DATA_G_S1   -1  //data green, section 1; ribbon pin -1
#define PIN_DATA_R_S1   -1  //data red, section 1; ribbon pin -1
#define PIN_A0_S1  -1       //row select, bit 0, section 1; ribbon pin -1
#define PIN_A1_S1  -1       //row select, bit 1, section 1; ribbon pin 11
#define PIN_A2_S1  -1       //row select, bit 2, section 1; ribbon pin 13
#define PIN_CS_S1  -1       //chip select, 0-7/8-15, section 1; ribbon pin 15

//row 2 of LED matrix modules
#define PIN_DATA_G_S2   -1  //data green, section 2; ribbon pin -1
#define PIN_DATA_R_S2   -1  //data red, section 2; ribbon pin -1
#define PIN_A0_S2  -1       //row select, bit 0, section 2; ribbon pin -1
#define PIN_A1_S2  -1       //row select, bit 1, section 2; ribbon pin 11
#define PIN_A2_S2  -1       //row select, bit 2, section 2; ribbon pin 13
#define PIN_CS_S2  -1       //chip select, 0-7/8-15, section 2; ribbon pin 15

//row 3 of LED matrix modules
#define PIN_DATA_G_S3   -1  //data green, section 3; ribbon pin -1
#define PIN_DATA_R_S3   -1  //data red, section 3; ribbon pin -1
#define PIN_A0_S3  -1       //row select, bit 0, section 3; ribbon pin -1
#define PIN_A1_S3  -1       //row select, bit 1, section 3; ribbon pin 11
#define PIN_A2_S3  -1       //row select, bit 2, section 3; ribbon pin 13
#define PIN_CS_S3  -1       //chip select, 0-7/8-15, section 3; ribbon pin 15

//for all rows
#define PIN_CLK 26   //universal clock pin
#define PIN_STR 28   //latch/strobe

#define PIN_EN_R 38 //output enable for red leds, active LOW; ribbon pin 17
#define PIN_EN_G 39 //output enable for green leds, active LOW; ribbon pin 18