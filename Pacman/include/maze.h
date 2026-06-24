/*----------------------------------------------------------------------------
 * maze.h  -  the play field (50 x 16 tiles, 4 px each => 200 x 64 px).
 *
 * The maze is GENERATED at boot from simple corridor rules, so connectivity is
 * guaranteed (every horizontal corridor crosses every vertical corridor) and
 * there is no hand-typed layout to get wrong. State is kept as bitsets to fit
 * the 8 KB SRAM.
 *--------------------------------------------------------------------------*/
#pragma once

#include "pacman_config.h"

#define MZ_W        50
#define MZ_H        16
#define TILE        4
#define TUNNEL_ROW  7

// Ghost-house reference tiles.
#define HOUSE_DOOR_X 24
#define HOUSE_DOOR_Y 6
#define HOUSE_EXIT_X 24      // corridor tile just above the door
#define HOUSE_EXIT_Y 4

extern const uint8_t PAC_START_X, PAC_START_Y;
extern const uint8_t GHOST_START_X[4], GHOST_START_Y[4];
extern const bool    GHOST_INHOUSE[4];
extern const uint8_t GHOST_SCATTER_X[4], GHOST_SCATTER_Y[4];

void mazeInit();          // build walls + dots (call once)
void mazeResetDots();     // refill dots/pellets for a new level

bool mazeOpenForPac(int x, int y);                  // x is wrapped (tunnel)
bool mazeOpenForGhost(int x, int y, bool allowDoor);
bool mazeIsDoor(int x, int y);

bool mazeHasDot(int x, int y);
bool mazeHasPellet(int x, int y);
void mazeEatDot(int x, int y);
void mazeEatPellet(int x, int y);
int  mazeDotsLeft();

// Wrap an x tile coordinate through the side tunnel.
static inline int mazeWrapX(int x) {
    if (x < 0) x += MZ_W;
    else if (x >= MZ_W) x -= MZ_W;
    return x;
}
