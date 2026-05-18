#include "digitalWriteFast.h"
#include "header.h"



// -------------------- HARDWARE --------------------
//
void pinSetup(){
    const int pins[] = {
    PIN_DATA_G_S0, PIN_DATA_R_S0, PIN_A0_S0, PIN_A1_S0, PIN_A2_S0, PIN_CS_S0,
    PIN_DATA_G_S1, PIN_DATA_R_S1, PIN_A0_S1, PIN_A1_S1, PIN_A2_S1, PIN_CS_S1,
    PIN_DATA_G_S2, PIN_DATA_R_S2, PIN_A0_S2, PIN_A1_S2, PIN_A2_S2, PIN_CS_S2,
    PIN_DATA_G_S3, PIN_DATA_R_S3, PIN_A0_S3, PIN_A1_S3, PIN_A2_S3, PIN_CS_S3,
    PIN_CLK, PIN_STR, PIN_EN_R, PIN_EN_G
    };

    const int count = sizeof(pins) / sizeof(pins[0]);

    for (int i = 0; i < count; ++i) {
        int p = pins[i];
        pinModeFast(p, OUTPUT);
        digitalWriteFast(p, LOW);
    }
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