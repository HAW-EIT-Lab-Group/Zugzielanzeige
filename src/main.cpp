#include <Arduino.h>
#include "Display.h"
#include "Graphics.h"
#include "Game.h"
#include "config.h"

void setup(){
    Display::init();
    Game::init(); // startet Serial (9600 Baud) und zeichnet das Labyrinth

    Display::refresh();
}

void loop(){
    // Game::update() blockiert nie (kein delay()), damit die Matrix
    // ohne Flackern in der geforderten Rate weiterläuft (s. README)
    Game::update();
    Display::refresh();
}