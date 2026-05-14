/*----------------------------------------
Project: Bachelorprojekt - Zugzielanzeige
File: ArduinoSketch.ino
Description: main file
Group Members: 
----------------------------------------*/

#include "functions.cpp"
#include "digitalWriteFast.h"
#include "header.h"

bool pixels_red[SIZE_X][SIZE_Y] = {}; //matrix storing the red pixels of the displayed image
bool pixels_green[SIZE_X][SIZE_Y] = {}; //matrix storing the green pixels of the displayed image

void setup(){
    pinSetup();
    
    digitalWriteFast(PIN_EN1, LOW);
    digitalWriteFast(PIN_EN2, LOW);
}

void loop(){
    //do something
    writeToDisplay(pixels_red,pixels_green);
}