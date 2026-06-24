/*----------------------------------------------------------------------------
 * ai_player.h  -  the self-playing "script".
 *
 * This is the stand-in for a human controller while no joystick is connected.
 * It looks at the live GameState and returns the next Input (a direction, plus
 * the "start" press on the title screen). Swap this out for a controller.cpp
 * that fills the same Input struct to play it by hand later.
 *--------------------------------------------------------------------------*/
#pragma once

#include "game.h"
#include "input.h"

Input aiPoll(const GameState &gs);
