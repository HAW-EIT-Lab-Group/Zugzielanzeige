/*----------------------------------------------------------------------------
 * input.h  -  abstract controller input.
 *
 * The game does not care WHERE input comes from. Right now it is produced by
 * the self-playing AI (ai_player.cpp). To use a real joystick/buttons later,
 * write a controller.cpp that fills the same struct and swap it in main.cpp --
 * nothing in the game logic has to change.
 *--------------------------------------------------------------------------*/
#pragma once

#include "pacman_config.h"

struct Input {
    int8_t dir;     // DIR_UP/LEFT/DOWN/RIGHT, or DIR_NONE for "keep going"
    bool   start;   // the "press start" button (used on the title screen)
};
