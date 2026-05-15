#include "digitalWriteFast.h"
#include "header.h"



// -------------------- HARDWARE --------------------
//
void pinSetup(){
    /*
    über boards array loopen, s. header.h
    //board 0
    pinMode(PIN_DATA_R_B0, OUTPUT);
    pinMode(PIN_DATA_G_B0, OUTPUT);
    pinMode(PIN_A0_B0, OUTPUT);
    pinMode(PIN_A1_B0, OUTPUT);
    pinMode(PIN_A2_B0, OUTPUT);
    pinMode(PIN_CS_B0, OUTPUT);
    //board 1
    //board 2
    //board 3
    
    //for all rows
    pinMode(PIN_CLK, OUTPUT);
    pinMode(PIN_STR, OUTPUT);
    pinMode(PIN_EN_R, OUTPUT);
    pinMode(PIN_EN_G, OUTPUT);
    */
}

//pulses clock pin
void pulseClock(){
    digitalWriteFast(PIN_CLK,true);
    digitalWriteFast(PIN_CLK,false);
}

//pulses strobe pin
void pulseLatch(){
    digitalWriteFast(PIN_STR,true);
    digitalWriteFast(PIN_STR,false);
}

//compares row bitwise to set the corresponding pins
void setRow(int row)
{
    /*
    arbeiten mit boards array, s. header.h
    
    //row 0-15
    if(row <= B0_MAX){
        digitalWriteFast(PIN_A0_B0, row & 0x01);
        digitalWriteFast(PIN_A1_B0, row & 0x02);
        digitalWriteFast(PIN_A2_B0, row & 0x04);
        digitalWriteFast(PIN_CS_B0, row >= 8);
    //row 16-31
    }else if(row > B0_MAX && row <= B1_MAX){

    //row 32-47
    }else if(row > B1_MAX && row <= B2_MAX){

    //row 48-63
    }else{

    }
    */
    
}

//writes pixel matricies to the display
void writeToDisplay(bool pixels_red,bool pixels_green){
    /*
    for(int y=0;y<SIZE_Y;y++){
        setRow(y);
        for(int x=0;x<SIZE_X;x++){
            //set data red = pixels_red[x][y];
            //set data green = pixels_green[x][y];
            pulseClock();
        }
        pulseLatch();
    }
    */
}

//clears entire display
void clearDisplay(){
    /*
    pin_EN_R = 1; //turn display off for instant clear
    pin_EN_G = 1;
    bool zeros[SIZE_X][SIZE_Y] = {};
    writeToDisplay(zeros,zeros);
    pin_EN_R = 1; //turn display back on
    pin_EN_G = 1;
    */
}

//turns display on/off
void displayOn(bool on){
    /*
    weiß nicht ob man so eine funktion wirklich braucht
    macht es sinn die reihen nicht alle gleichzeitig anzuschalten, damit man keinen strom spike hat?

    digitalWriteFast(PIN_EN1, !on); //negieren wegen active low
    digitalWriteFast(PIN_EN2, !on); 
    */
}

// -------------------- SOFTWARE --------------------

/*
grafik:
- text -> matrix
- pixel malen
- linien malen
- formen malen

externer input;
- wireless
    - web server
    - ...
- wired
    - uart
    - ...
*/