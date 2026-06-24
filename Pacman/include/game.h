/*----------------------------------------------------------------------------
 * game.h  -  arcade Pac-Man state machine + entities.
 *
 * The whole GameState is exposed (read-only intent) so the self-playing AI in
 * ai_player.cpp can see pac, the ghosts and their modes to make decisions.
 *--------------------------------------------------------------------------*/
#pragma once

#include "pacman_config.h"
#include "input.h"
#include "maze.h"

// High-level phases: TITLE -> READY -> PLAY -> DYING -> (READY | GAMEOVER -> TITLE)
enum {
    PH_TITLE = 0, PH_READY, PH_PLAY, PH_DYING, PH_GAMEOVER
};

// Ghost behaviour modes.
enum {
    GM_SCATTER = 0, GM_CHASE, GM_FRIGHT, GM_EATEN, GM_HOUSE, GM_LEAVING
};

struct Entity {
    int16_t px, py;     // pixel pos = top-left of the tile-aligned cell (0..199 / 0..63)
    int8_t  dir;        // DIR_* or DIR_NONE
};

struct Ghost {
    Entity  e;
    uint8_t mode;
    int16_t releaseTimer;   // ticks until it leaves the house (GM_HOUSE)
};

struct GameState {
    uint8_t  phase;
    uint16_t phaseTimer;
    uint32_t tick;

    uint32_t score;
    uint32_t highScore;
    uint8_t  lives;
    uint8_t  level;
    bool     extraLifeGiven;

    Entity   pac;
    int8_t   pacWanted;
    int8_t   pacLastDir;     // for ghost targeting (never NONE once moving)
    uint8_t  mouth;          // mouth animation counter

    Ghost    ghosts[4];
    bool     globalScatter;
    int16_t  globalModeTimer;
    int16_t  frightTimer;
    uint8_t  ghostChain;     // 0..3 -> 200/400/800/1600 points
};

void gameInit();
void gameUpdate(const Input &in);
void gameRender();
const GameState &gameGetState();

// pixel<->tile helpers shared with the AI
static inline int tileX(const Entity &e) { return e.px / TILE; }
static inline int tileY(const Entity &e) { return e.py / TILE; }
