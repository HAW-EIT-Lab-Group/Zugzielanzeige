/*----------------------------------------------------------------------------
 * game.cpp  -  arcade Pac-Man logic + rendering.
 *
 * Movement model: entities live on the 50x16 tile grid but move 1 px at a time
 * for smoothness. Direction decisions happen only when an entity is exactly
 * tile-aligned (classic Pac-Man behaviour). Ghosts use the arcade target-tile
 * algorithm (pick the non-reverse move that minimises distance to a per-ghost
 * target), with scatter/chase/frightened/eaten modes.
 *--------------------------------------------------------------------------*/
#include "game.h"
#include "gfx.h"

#ifndef ARDUINO
  #include <cstdlib>
  static long random(long n) { return rand() % n; }
#endif

// ----- tunables -----
#define START_LIVES     3      // set to 1 if you want the title after every death
#define READY_TICKS     80
#define DEATH_TICKS     90
#define GAMEOVER_TICKS  150
#define SCATTER_TICKS   460    // ~7 s at 66 ticks/s
#define CHASE_TICKS     1320   // ~20 s
#define FRIGHT_TICKS    360    // ~5.4 s (shrinks with level)
#define EXTRA_LIFE_AT   10000UL

static const int8_t DX[4] = { 0, -1, 0, 1 };   // UP, LEFT, DOWN, RIGHT
static const int8_t DY[4] = { -1, 0, 1, 0 };
static inline int reverseDir(int d) { return d < 0 ? d : (d + 2) & 3; }

static GameState g;
const GameState &gameGetState() { return g; }

// ---------------------------------------------------------------- helpers ---
static inline bool aligned(const Entity &e) {
    return (e.px % TILE) == 0 && (e.py % TILE) == 0;
}

static void moveEntity(Entity &e) {
    if (e.dir < 0) return;
    e.px += DX[e.dir];
    e.py += DY[e.dir];
    const int maxpx = MZ_W * TILE;          // 200
    if (e.px < 0)        e.px += maxpx;      // side-tunnel wrap (pixel space)
    else if (e.px >= maxpx) e.px -= maxpx;
}

// distance helper accounting for the horizontal wrap
static long tileDist2(int x0, int y0, int x1, int y1) {
    long dx = x0 - x1;
    if (dx > MZ_W / 2) dx -= MZ_W; else if (dx < -MZ_W / 2) dx += MZ_W;
    long dy = y0 - y1;
    return dx * dx + dy * dy;
}

// -------------------------------------------------------------- placement ---
static void placeEntities() {
    g.pac.px = PAC_START_X * TILE;
    g.pac.py = PAC_START_Y * TILE;
    g.pac.dir = DIR_LEFT;
    g.pacWanted = DIR_NONE;
    g.pacLastDir = DIR_LEFT;
    g.mouth = 0;

    for (int i = 0; i < 4; i++) {
        g.ghosts[i].e.px = GHOST_START_X[i] * TILE;
        g.ghosts[i].e.py = GHOST_START_Y[i] * TILE;
        g.ghosts[i].e.dir = (i == 0) ? DIR_LEFT : DIR_UP;
        g.ghosts[i].mode = GHOST_INHOUSE[i] ? GM_HOUSE : GM_SCATTER;
    }
    g.ghosts[1].releaseTimer = 60;
    g.ghosts[2].releaseTimer = 180;
    g.ghosts[3].releaseTimer = 300;

    g.globalScatter = true;
    g.globalModeTimer = SCATTER_TICKS;
    g.frightTimer = 0;
    g.ghostChain = 0;
}

static void startNewGame() {
    g.score = 0;
    g.lives = START_LIVES;
    g.level = 1;
    g.extraLifeGiven = false;
    mazeResetDots();
    placeEntities();
}

static void nextLevel() {
    g.level++;
    mazeResetDots();
    placeEntities();
}

// ---------------------------------------------------------- mode switching ---
static void reverseActiveGhosts() {
    for (int i = 0; i < 4; i++)
        if (g.ghosts[i].mode == GM_SCATTER || g.ghosts[i].mode == GM_CHASE ||
            g.ghosts[i].mode == GM_FRIGHT)
            g.ghosts[i].e.dir = reverseDir(g.ghosts[i].e.dir);
}

static void enterFright() {
    int t = FRIGHT_TICKS - (int)(g.level - 1) * 40;
    if (t < 60) t = 60;
    g.frightTimer = t;
    g.ghostChain = 0;
    for (int i = 0; i < 4; i++)
        if (g.ghosts[i].mode == GM_SCATTER || g.ghosts[i].mode == GM_CHASE)
            g.ghosts[i].mode = GM_FRIGHT;
    reverseActiveGhosts();
}

static void endFright() {
    for (int i = 0; i < 4; i++)
        if (g.ghosts[i].mode == GM_FRIGHT)
            g.ghosts[i].mode = g.globalScatter ? GM_SCATTER : GM_CHASE;
}

static void updateGlobalModes() {
    if (g.frightTimer > 0) return;          // scatter/chase clock pauses in fright
    if (--g.globalModeTimer > 0) return;
    g.globalScatter = !g.globalScatter;
    g.globalModeTimer = g.globalScatter ? SCATTER_TICKS : CHASE_TICKS;
    for (int i = 0; i < 4; i++)
        if (g.ghosts[i].mode == GM_SCATTER || g.ghosts[i].mode == GM_CHASE)
            g.ghosts[i].mode = g.globalScatter ? GM_SCATTER : GM_CHASE;
    reverseActiveGhosts();
}

// ------------------------------------------------------------------- pac ----
static void addScore(uint32_t pts) {
    g.score += pts;
    if (!g.extraLifeGiven && g.score >= EXTRA_LIFE_AT) {
        g.extraLifeGiven = true;
        g.lives++;
    }
    if (g.score > g.highScore) g.highScore = g.score;
}

static void pacStep() {
    if (aligned(g.pac)) {
        int cx = tileX(g.pac), cy = tileY(g.pac);
        if (mazeHasDot(cx, cy))    { mazeEatDot(cx, cy);    addScore(10); }
        if (mazeHasPellet(cx, cy)) { mazeEatPellet(cx, cy); addScore(50); enterFright(); }

        if (g.pacWanted != DIR_NONE) {
            int nx = mazeWrapX(cx + DX[g.pacWanted]), ny = cy + DY[g.pacWanted];
            if (mazeOpenForPac(nx, ny)) g.pac.dir = g.pacWanted;
        }
        if (g.pac.dir != DIR_NONE) {
            int nx = mazeWrapX(cx + DX[g.pac.dir]), ny = cy + DY[g.pac.dir];
            if (!mazeOpenForPac(nx, ny)) g.pac.dir = DIR_NONE;   // wall ahead: stop
        }
    }
    if (g.pac.dir != DIR_NONE) { g.pacLastDir = g.pac.dir; moveEntity(g.pac); }
    g.mouth = (uint8_t)((g.mouth + 1) & 7);
}

// ----------------------------------------------------------------- ghosts ---
static void ghostTarget(int i, int *tx, int *ty) {
    if (g.globalScatter && g.ghosts[i].mode == GM_SCATTER) {
        *tx = GHOST_SCATTER_X[i]; *ty = GHOST_SCATTER_Y[i];
        return;
    }
    int pcx = tileX(g.pac), pcy = tileY(g.pac);
    int pd  = g.pacLastDir < 0 ? DIR_LEFT : g.pacLastDir;
    switch (i) {
        case 0:                                   // Blinky: pac's tile
            *tx = pcx; *ty = pcy; break;
        case 1:                                   // Pinky: 4 tiles ahead
            *tx = pcx + 4 * DX[pd]; *ty = pcy + 4 * DY[pd]; break;
        case 2: {                                 // Inky: 2*ahead2 - Blinky
            int ax = pcx + 2 * DX[pd], ay = pcy + 2 * DY[pd];
            int bx = tileX(g.ghosts[0].e), by = tileY(g.ghosts[0].e);
            *tx = 2 * ax - bx; *ty = 2 * ay - by; break;
        }
        default: {                                // Clyde: chase if far, else corner
            int cx = tileX(g.ghosts[i].e), cy = tileY(g.ghosts[i].e);
            if (tileDist2(cx, cy, pcx, pcy) > 64) { *tx = pcx; *ty = pcy; }
            else { *tx = GHOST_SCATTER_X[i]; *ty = GHOST_SCATTER_Y[i]; }
        }
    }
}

static void chooseToward(int i, int tx, int ty, bool allowDoor) {
    Ghost &gh = g.ghosts[i];
    int cx = tileX(gh.e), cy = tileY(gh.e);
    int best = -1; long bestD = 0x7FFFFFFFL;
    for (int d = 0; d < 4; d++) {                 // preference order UP,LEFT,DOWN,RIGHT
        if (d == reverseDir(gh.e.dir)) continue;
        int nx = mazeWrapX(cx + DX[d]), ny = cy + DY[d];
        if (!mazeOpenForGhost(nx, ny, allowDoor)) continue;
        long dd = tileDist2(nx, ny, tx, ty);
        if (dd < bestD) { bestD = dd; best = d; }
    }
    if (best < 0 && gh.e.dir >= 0) {              // dead end -> allow reverse
        int d = reverseDir(gh.e.dir);
        int nx = mazeWrapX(cx + DX[d]), ny = cy + DY[d];
        if (mazeOpenForGhost(nx, ny, allowDoor)) best = d;
    }
    if (best >= 0) gh.e.dir = best;
}

static void chooseRandom(int i, bool allowDoor) {
    Ghost &gh = g.ghosts[i];
    int cx = tileX(gh.e), cy = tileY(gh.e);
    int opts[4], n = 0;
    for (int d = 0; d < 4; d++) {
        if (d == reverseDir(gh.e.dir)) continue;
        int nx = mazeWrapX(cx + DX[d]), ny = cy + DY[d];
        if (mazeOpenForGhost(nx, ny, allowDoor)) opts[n++] = d;
    }
    if (n > 0) gh.e.dir = opts[random(n)];
    else if (gh.e.dir >= 0) gh.e.dir = reverseDir(gh.e.dir);
}

static void ghostDecide(int i) {
    Ghost &gh = g.ghosts[i];
    int cx = tileX(gh.e), cy = tileY(gh.e);

    switch (gh.mode) {
        case GM_HOUSE:
            if (gh.releaseTimer <= 0) { gh.mode = GM_LEAVING; gh.e.dir = DIR_UP; break; }
            if (gh.e.dir != DIR_UP && gh.e.dir != DIR_DOWN) gh.e.dir = DIR_UP;
            if (cy <= 7) gh.e.dir = DIR_DOWN;          // bob within the house
            else if (cy >= 8) gh.e.dir = DIR_UP;
            break;

        case GM_LEAVING:
            gh.e.dir = DIR_UP;
            if (cy <= HOUSE_EXIT_Y) {                  // out in the corridor now
                gh.mode = g.globalScatter ? GM_SCATTER : GM_CHASE;
                gh.e.dir = DIR_LEFT;
            }
            break;

        case GM_EATEN:
            if (cx == HOUSE_DOOR_X && cy >= HOUSE_DOOR_Y) {   // reached the house
                gh.e.px = GHOST_START_X[0] * TILE; gh.e.py = 7 * TILE;
                gh.mode = GM_HOUSE; gh.releaseTimer = 60; gh.e.dir = DIR_UP;
            } else {
                chooseToward(i, HOUSE_DOOR_X, HOUSE_DOOR_Y, true);
            }
            break;

        case GM_FRIGHT:
            chooseRandom(i, false);
            break;

        default: {                                     // SCATTER / CHASE
            int tx, ty; ghostTarget(i, &tx, &ty);
            chooseToward(i, tx, ty, false);
        }
    }
}

static void ghostStep(int i) {
    if (aligned(g.ghosts[i].e)) ghostDecide(i);
    moveEntity(g.ghosts[i].e);
}

static int ghostSpeedSteps(int i) {
    switch (g.ghosts[i].mode) {
        case GM_EATEN:  return 2;                       // eyes return fast
        case GM_FRIGHT: return (g.tick & 1) ? 1 : 0;    // ~50%
        case GM_HOUSE:  return (g.tick & 1) ? 1 : 0;    // slow bob
        default:        return ((g.tick & 15) == 0) ? 0 : 1; // ~94% of pac
    }
}

// --------------------------------------------------------------- collisions ---
static bool overlapsPac(const Entity &e) {
    int dx = (g.pac.px + TILE / 2) - (e.px + TILE / 2);
    int dy = (g.pac.py + TILE / 2) - (e.py + TILE / 2);
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx < 3 && dy < 3;
}

static bool handleCollisions() {
    for (int i = 0; i < 4; i++) {
        Ghost &gh = g.ghosts[i];
        if (!overlapsPac(gh.e)) continue;
        if (gh.mode == GM_FRIGHT) {
            uint32_t pts = 200UL << g.ghostChain;       // 200/400/800/1600
            if (g.ghostChain < 3) g.ghostChain++;
            addScore(pts);
            gh.mode = GM_EATEN;
        } else if (gh.mode == GM_SCATTER || gh.mode == GM_CHASE || gh.mode == GM_LEAVING) {
            return true;                                 // pac dies
        }
    }
    return false;
}

// ================================================================ update ====
void gameInit() {
    mazeInit();
    g.phase = PH_TITLE;
    g.phaseTimer = 0;
    g.tick = 0;
    g.score = 0;
    g.highScore = 0;
    g.lives = 0;
    g.level = 1;
    placeEntities();
}

void gameUpdate(const Input &in) {
    g.tick++;

    switch (g.phase) {
        case PH_TITLE:
            g.phaseTimer++;
            if (in.start) { startNewGame(); g.phase = PH_READY; g.phaseTimer = 0; }
            break;

        case PH_READY:
            g.phaseTimer++;
            if (g.phaseTimer >= READY_TICKS) { g.phase = PH_PLAY; g.phaseTimer = 0; }
            break;

        case PH_PLAY: {
            g.phaseTimer++;
            if (in.dir != DIR_NONE) g.pacWanted = in.dir;

            updateGlobalModes();
            if (g.frightTimer > 0 && --g.frightTimer == 0) endFright();

            pacStep();

            for (int i = 0; i < 4; i++)
                if (g.ghosts[i].mode == GM_HOUSE && g.ghosts[i].releaseTimer > 0)
                    g.ghosts[i].releaseTimer--;

            for (int i = 0; i < 4; i++) {
                int steps = ghostSpeedSteps(i);
                for (int s = 0; s < steps; s++) ghostStep(i);
            }

            if (handleCollisions()) { g.phase = PH_DYING; g.phaseTimer = 0; break; }
            if (mazeDotsLeft() == 0) { nextLevel(); g.phase = PH_READY; g.phaseTimer = 0; }
            break;
        }

        case PH_DYING:
            g.phaseTimer++;
            if (g.phaseTimer >= DEATH_TICKS) {
                if (g.lives > 0) g.lives--;
                if (g.lives == 0) { g.phase = PH_GAMEOVER; g.phaseTimer = 0; }
                else { placeEntities(); g.phase = PH_READY; g.phaseTimer = 0; }
            }
            break;

        case PH_GAMEOVER:
            g.phaseTimer++;
            if (g.phaseTimer >= GAMEOVER_TICKS) { g.phase = PH_TITLE; g.phaseTimer = 0; }
            break;
    }
}

// ================================================================ render ====
static void drawDotsAndWalls() {
    for (int cy = 0; cy < MZ_H; cy++) {
        for (int cx = 0; cx < MZ_W; cx++) {
            if (!mazeOpenForPac(cx, cy) && !mazeIsDoor(cx, cy)) {
                // wall block (skip the door so it reads as a gap)
                if (cy == 0 || cx == 0 || cy == MZ_H - 1 || cx == MZ_W - 1) {
                    // border: thin so the field feels open
                    gfxFillRect(cx * TILE, cy * TILE, TILE, TILE, CLR_GREEN);
                } else {
                    gfxFillRect(cx * TILE + 1, cy * TILE + 1, TILE - 1, TILE - 1, CLR_GREEN);
                }
            }
            if (mazeIsDoor(cx, cy))
                gfxHLine(cx * TILE, cy * TILE + 1, TILE, CLR_RED);
        }
    }
    bool pelletOn = ((g.tick >> 3) & 1) != 0;
    for (int cy = 0; cy < MZ_H; cy++) {
        for (int cx = 0; cx < MZ_W; cx++) {
            if (mazeHasDot(cx, cy))
                dispSetPixel(cx * TILE + 1, cy * TILE + 1, CLR_RED);
            else if (mazeHasPellet(cx, cy) && pelletOn)
                gfxFillRect(cx * TILE, cy * TILE, 2, 2, CLR_ORANGE);
        }
    }
}

static void drawPac(int px, int py, int dir, bool open) {
    gfxFillRect(px, py, TILE, TILE, CLR_ORANGE);
    // round the corners
    dispSetPixel(px, py, CLR_BLACK);
    dispSetPixel(px + TILE - 1, py, CLR_BLACK);
    dispSetPixel(px, py + TILE - 1, CLR_BLACK);
    dispSetPixel(px + TILE - 1, py + TILE - 1, CLR_BLACK);
    if (open && dir >= 0) {                         // cut a mouth on the leading edge
        if (dir == DIR_RIGHT) { dispSetPixel(px + TILE - 1, py + 1, CLR_BLACK); dispSetPixel(px + TILE - 1, py + 2, CLR_BLACK); }
        else if (dir == DIR_LEFT) { dispSetPixel(px, py + 1, CLR_BLACK); dispSetPixel(px, py + 2, CLR_BLACK); }
        else if (dir == DIR_UP) { dispSetPixel(px + 1, py, CLR_BLACK); dispSetPixel(px + 2, py, CLR_BLACK); }
        else { dispSetPixel(px + 1, py + TILE - 1, CLR_BLACK); dispSetPixel(px + 2, py + TILE - 1, CLR_BLACK); }
    }
}

static const uint8_t GHOST_COL[4] = { CLR_RED, CLR_ORANGE, CLR_GREEN, CLR_RED };

static void drawGhost(int i) {
    const Ghost &gh = g.ghosts[i];
    int px = gh.e.px, py = gh.e.py;
    if (gh.mode == GM_EATEN) {                      // just the eyes
        dispSetPixel(px + 1, py + 1, CLR_GREEN);
        dispSetPixel(px + 2, py + 1, CLR_GREEN);
        return;
    }
    uint8_t col;
    if (gh.mode == GM_FRIGHT) {
        bool flash = g.frightTimer < 120 && ((g.tick >> 2) & 1);
        col = flash ? CLR_ORANGE : CLR_GREEN;
    } else {
        col = GHOST_COL[i];
    }
    gfxFillRect(px, py, TILE, TILE, col);
    dispSetPixel(px, py, CLR_BLACK);                // round top corners
    dispSetPixel(px + TILE - 1, py, CLR_BLACK);
    dispSetPixel(px + 1, py + TILE - 1, CLR_BLACK); // little feet
}

static void drawHUD() {
    char buf[12];
    uint32_t s = g.score;
    int n = 0; char tmp[11];
    if (s == 0) tmp[n++] = '0';
    while (s > 0 && n < 10) { tmp[n++] = (char)('0' + s % 10); s /= 10; }
    int k = 0; while (n > 0) buf[k++] = tmp[--n];
    buf[k] = 0;
    gfxText(2, 0, buf, CLR_ORANGE);                 // score, top-left
    for (int i = 0; i < g.lives && i < 6; i++)      // lives, top-right
        gfxFillRect(DISP_W - 6 - i * 6, 1, 4, 4, CLR_ORANGE);
}

static void drawTitle() {
    gfxTextCentered(8, "PAC-MAN", CLR_ORANGE);
    if (((g.tick >> 4) & 1))
        gfxTextCentered(28, "PUSH START", CLR_GREEN);
    gfxTextCentered(44, "DEMO - AI PLAY", CLR_RED);
    // a little row of characters under the title: pac chased by 4 ghosts
    drawPac(70, 56, DIR_RIGHT, ((g.tick >> 2) & 1) != 0);
    for (int i = 0; i < 4; i++) {
        int gx = 86 + i * 8, gy = 56;
        gfxFillRect(gx, gy, TILE, TILE, GHOST_COL[i]);
        dispSetPixel(gx, gy, CLR_BLACK);
        dispSetPixel(gx + TILE - 1, gy, CLR_BLACK);
    }
}

void gameRender() {
    dispClear();

    if (g.phase == PH_TITLE) { drawTitle(); return; }

    drawDotsAndWalls();

    // pac (blink while dying)
    bool drawThePac = true;
    if (g.phase == PH_DYING) drawThePac = ((g.tick >> 2) & 1) != 0;
    if (drawThePac)
        drawPac(g.pac.px, g.pac.py, g.pacLastDir, (g.mouth & 4) != 0);

    if (g.phase != PH_DYING)
        for (int i = 0; i < 4; i++) drawGhost(i);

    drawHUD();

    if (g.phase == PH_READY)    gfxTextCentered(34, "READY", CLR_ORANGE);
    if (g.phase == PH_GAMEOVER) gfxTextCentered(28, "GAME OVER", CLR_RED);
}
