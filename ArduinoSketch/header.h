const int SIZE_X = 200; //width of led matrix
const int SIZE_Y = 64; //height of led matrix

const int B0_MAX = 15; //upper edge led nr on board 0
const int B1_MAX = 31; //upper edge led nr on board 1
const int B2_MAX = 47; //upper edge led nr on board 2
const int B3_MAX = 63; //upper edge led nr on board 3

// -------------------- PIN DEFINITIONS --------------------

/*zurzeit unschön, vielleicht in struct speichern?
struct board_pins{
    int data_r;
    int data_g;
    int a0;
    int a1;
    int a2;
    int cs;
}

dann

board_pins boards[] = {
{22, 24, 30, 32, 34, 36}, //board 0
{}, //board 1
{}, //board 2
{} //board 3
};
*/

//row 0 of LED matrix modules
#define PIN_DATA_G_B0   22  //data green, board 0; ribbon pin ??
#define PIN_DATA_R_B0   24  //data red, board 0; ribbon pin ??
#define PIN_A0_B0  30       //row select, bit 0, board 0; ribbon pin ??
#define PIN_A1_B0  32       //row select, bit 1, board 0; ribbon pin 11
#define PIN_A2_B0  34       //row select, bit 2, board 0; ribbon pin 13
#define PIN_CS_B0  36       //chip select, 0-7/8-15, board 0; ribbon pin 15

//row 1 of LED matrix modules
#define PIN_DATA_G_B1   ??  //data green, board 1; ribbon pin ??
#define PIN_DATA_R_B1   ??  //data red, board 1; ribbon pin ??
#define PIN_A0_B1  ??       //row select, bit 0, board 1; ribbon pin ??
#define PIN_A1_B1  ??       //row select, bit 1, board 1; ribbon pin 11
#define PIN_A2_B1  ??       //row select, bit 2, board 1; ribbon pin 13
#define PIN_CS_B1  ??       //chip select, 0-7/8-15, board 1; ribbon pin 15

//row 2 of LED matrix modules
#define PIN_DATA_G_B2   ??  //data green, board 2; ribbon pin ??
#define PIN_DATA_R_B2   ??  //data red, board 2; ribbon pin ??
#define PIN_A0_B2  ??       //row select, bit 0, board 2; ribbon pin ??
#define PIN_A1_B2  ??       //row select, bit 1, board 2; ribbon pin 11
#define PIN_A2_B2  ??       //row select, bit 2, board 2; ribbon pin 13
#define PIN_CS_B2  ??       //chip select, 0-7/8-15, board 2; ribbon pin 15

//row 3 of LED matrix modules
#define PIN_DATA_G_B3   ??  //data green, board 3; ribbon pin ??
#define PIN_DATA_R_B3   ??  //data red, board 3; ribbon pin ??
#define PIN_A0_B3  ??       //row select, bit 0, board 3; ribbon pin ??
#define PIN_A1_B3  ??       //row select, bit 1, board 3; ribbon pin 11
#define PIN_A2_B3  ??       //row select, bit 2, board 3; ribbon pin 13
#define PIN_CS_B3  ??       //chip select, 0-7/8-15, board 3; ribbon pin 15

//for all rows
#define PIN_CLK 26   //universal clock pin
#define PIN_STR 28   //latch/strobe

#define PIN_EN_R 38 //output enable for red leds, active LOW; ribbon pin 17
#define PIN_EN_G 39 //output enable for green leds, active LOW; ribbon pin 18