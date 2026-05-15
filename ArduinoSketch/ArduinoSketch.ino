/*----------------------------------------
Project: Bachelorprojekt - Zugzielanzeige
File: ArduinoSketch.ino
Description: main file
Group Members:  
----------------------------------------*/

#include "functions.cpp"
#include "header.h"


bool pixels_red[SIZE_X][SIZE_Y] = {}; // global matrix storing the red pixels of the displayed image
bool pixels_green[SIZE_X][SIZE_Y] = {}; // global matrix storing the green pixels of the displayed image 

void setup(){
    pinSetup();
    
    digitalWriteFast(PIN_EN_R, LOW);
    digitalWriteFast(PIN_EN_G, LOW);
}

void loop(){
    // do something
    writeToDisplay(pixels_red,pixels_green);
}