/*----------------------------------------------------------------------------
 * maze.cpp  -  procedural maze generation + queries.
 *--------------------------------------------------------------------------*/
#include "maze.h"

// ----- fixed reference tiles -----
const uint8_t PAC_START_X = 24, PAC_START_Y = 13;

// Blinky starts just outside the door; the rest start inside the house.
const uint8_t GHOST_START_X[4] = {24, 24, 25, 25};
const uint8_t GHOST_START_Y[4] = { 4,  7,  7,  8};
const bool    GHOST_INHOUSE[4] = {false, true, true, true};

// Scatter corners: Blinky=TR, Pinky=TL, Inky=BR, Clyde=BL.
const uint8_t GHOST_SCATTER_X[4] = {48,  1, 48,  1};
const uint8_t GHOST_SCATTER_Y[4] = { 1,  1, 14, 14};

// ----- house geometry -----
#define HX0 20
#define HX1 29
#define HY0 6
#define HY1 9

// ----- state bitsets (7 bytes/row covers 50 columns) -----
static uint8_t wallPac[MZ_H][7];   // 1 = pac cannot enter (walls + door)
static uint8_t dotBits[MZ_H][7];
static uint8_t pelBits[MZ_H][7];
static int     dotsLeft;

static inline bool getBit(const uint8_t b[][7], int x, int y) {
    return (b[y][x >> 3] >> (x & 7)) & 1;
}
static inline void putBit(uint8_t b[][7], int x, int y, bool v) {
    uint8_t m = (uint8_t)(1u << (x & 7));
    if (v) b[y][x >> 3] |= m; else b[y][x >> 3] &= (uint8_t)~m;
}

// ----- generation rules -----
static bool rowCorr(int y) {
    return y == 1 || y == 4 || y == 7 || y == 10 || y == 13;
}
static bool colCorr(int x) {
    switch (x) {
        case 1: case 4: case 7: case 10: case 13: case 16: case 19: case 22:
        case 27: case 30: case 33: case 36: case 39: case 42: case 45: case 48:
            return true;
    }
    return false;
}
static bool isDoorCell(int x, int y)   { return y == HY0 && (x == 24 || x == 25); }
static bool inHouse(int x, int y)      { return x >= HX0 && x <= HX1 && y >= HY0 && y <= HY1; }
static bool houseWall(int x, int y)    { return inHouse(x, y) && (x == HX0 || x == HX1 || y == HY0 || y == HY1) && !isDoorCell(x, y); }
static bool houseInside(int x, int y)  { return inHouse(x, y) && !(x == HX0 || x == HX1 || y == HY0 || y == HY1); }
static bool carveExit(int x, int y)    { return (x == 24 || x == 25) && y == 5; }
static bool isPellet(int x, int y) {
    return (x == 1 && y == 1) || (x == 48 && y == 1) ||
           (x == 1 && y == 14) || (x == 48 && y == 14);
}

static bool wallForPac(int x, int y) {
    if (x == 0 || x == MZ_W - 1 || y == 0 || y == MZ_H - 1) {
        if (y == TUNNEL_ROW && (x == 0 || x == MZ_W - 1)) return false; // tunnel mouth
        return true;
    }
    if (isDoorCell(x, y)) return true;      // door blocks pac
    if (houseWall(x, y))  return true;
    if (houseInside(x, y)) return false;    // open, but pac can't reach (door blocks)
    if (carveExit(x, y))  return false;
    if (rowCorr(y) || colCorr(x)) return false;
    return true;                            // wall pillar
}

static void fillDots() {
    dotsLeft = 0;
    for (int y = 0; y < MZ_H; y++) {
        for (int x = 0; x < MZ_W; x++) {
            putBit(dotBits, x, y, false);
            putBit(pelBits, x, y, false);
            if (wallForPac(x, y)) continue;                 // walls: nothing
            if (x == 0 || x == MZ_W - 1 || y == 0 || y == MZ_H - 1) continue; // tunnel mouths
            if (houseInside(x, y) || isDoorCell(x, y) || carveExit(x, y)) continue;
            if (x == PAC_START_X && y == PAC_START_Y) continue;
            if (isPellet(x, y)) { putBit(pelBits, x, y, true); dotsLeft++; }
            else                { putBit(dotBits, x, y, true); dotsLeft++; }
        }
    }
}

void mazeInit() {
    for (int y = 0; y < MZ_H; y++)
        for (int x = 0; x < MZ_W; x++)
            putBit(wallPac, x, y, wallForPac(x, y));
    fillDots();
}

void mazeResetDots() { fillDots(); }

bool mazeOpenForPac(int x, int y) {
    x = mazeWrapX(x);
    if (y < 0 || y >= MZ_H) return false;
    return !getBit(wallPac, x, y);
}

bool mazeIsDoor(int x, int y) {
    x = mazeWrapX(x);
    return isDoorCell(x, y);
}

bool mazeOpenForGhost(int x, int y, bool allowDoor) {
    x = mazeWrapX(x);
    if (y < 0 || y >= MZ_H) return false;
    if (isDoorCell(x, y)) return allowDoor;
    // ghosts may also walk inside the house; only the door gates entry/exit.
    return !getBit(wallPac, x, y) || houseInside(x, y);
}

bool mazeHasDot(int x, int y)    { x = mazeWrapX(x); return getBit(dotBits, x, y); }
bool mazeHasPellet(int x, int y) { x = mazeWrapX(x); return getBit(pelBits, x, y); }

void mazeEatDot(int x, int y) {
    x = mazeWrapX(x);
    if (getBit(dotBits, x, y)) { putBit(dotBits, x, y, false); dotsLeft--; }
}
void mazeEatPellet(int x, int y) {
    x = mazeWrapX(x);
    if (getBit(pelBits, x, y)) { putBit(pelBits, x, y, false); dotsLeft--; }
}
int mazeDotsLeft() { return dotsLeft; }
