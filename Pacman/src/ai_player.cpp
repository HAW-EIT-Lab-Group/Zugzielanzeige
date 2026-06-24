/*----------------------------------------------------------------------------
 * ai_player.cpp  -  decides Pac-Man's moves automatically.
 *
 * Strategy (recomputed only at tile centres, so it costs ~nothing per frame):
 *   1. Mark tiles occupied/adjacent by DANGEROUS ghosts (chasing/scattering)
 *      as off-limits.
 *   2. Breadth-first search from pac over the maze to the NEAREST goal
 *      (a dot, a power pellet, or a frightened ghost to eat), never stepping
 *      into a danger tile. Return the first move of that shortest path.
 *   3. If everything reachable is blocked by ghosts, flee: step toward the
 *      tile that maximises distance from the closest ghost.
 *
 * BFS over 50x16 = 800 tiles is trivial for the Mega; buffers are static so
 * they never touch the stack.
 *--------------------------------------------------------------------------*/
#include <string.h>
#include "ai_player.h"
#include "maze.h"

static const int8_t DX[4] = { 0, -1, 0, 1 };   // UP, LEFT, DOWN, RIGHT
static const int8_t DY[4] = { -1, 0, 1, 0 };

#define NCELLS (MZ_W * MZ_H)

static uint8_t  firstMove[NCELLS];              // 0xFF=unvisited, 0xFE=start, else dir
static uint16_t queue[NCELLS];                  // linear queue: each cell is pushed once
static uint8_t  dangerBits[(NCELLS + 7) / 8];

static inline void dangerSet(int idx) { dangerBits[idx >> 3] |= (uint8_t)(1u << (idx & 7)); }
static inline bool dangerGet(int idx) { return (dangerBits[idx >> 3] >> (idx & 7)) & 1; }

static bool ghostDangerous(const Ghost &gh) {
    return gh.mode == GM_SCATTER || gh.mode == GM_CHASE || gh.mode == GM_LEAVING;
}

static bool frightGhostAt(const GameState &gs, int cx, int cy) {
    for (int i = 0; i < 4; i++)
        if (gs.ghosts[i].mode == GM_FRIGHT &&
            tileX(gs.ghosts[i].e) == cx && tileY(gs.ghosts[i].e) == cy)
            return true;
    return false;
}

static void buildDanger(const GameState &gs) {
    memset(dangerBits, 0, sizeof(dangerBits));
    for (int i = 0; i < 4; i++) {
        if (!ghostDangerous(gs.ghosts[i])) continue;
        int gx = tileX(gs.ghosts[i].e), gy = tileY(gs.ghosts[i].e);
        for (int d = -1; d < 4; d++) {
            int nx = (d < 0) ? gx : mazeWrapX(gx + DX[d]);
            int ny = (d < 0) ? gy : gy + DY[d];
            if (ny < 0 || ny >= MZ_H) continue;
            dangerSet(ny * MZ_W + nx);
        }
    }
}

static int aiFlee(const GameState &gs) {
    int pcx = tileX(gs.pac), pcy = tileY(gs.pac);
    int rev = (gs.pac.dir < 0) ? -1 : (gs.pac.dir + 2) & 3;
    int best = DIR_NONE; long bestScore = -1;
    for (int d = 0; d < 4; d++) {
        int nx = mazeWrapX(pcx + DX[d]), ny = pcy + DY[d];
        if (ny < 0 || ny >= MZ_H || !mazeOpenForPac(nx, ny)) continue;
        long nearest = 0x7FFFFFFFL;
        for (int i = 0; i < 4; i++) {
            if (!ghostDangerous(gs.ghosts[i])) continue;
            long dx = nx - tileX(gs.ghosts[i].e), dy = ny - tileY(gs.ghosts[i].e);
            long dist = dx * dx + dy * dy;
            if (dist < nearest) nearest = dist;
        }
        if (d == rev) nearest -= 2;                 // mild penalty for U-turns
        if (nearest > bestScore) { bestScore = nearest; best = d; }
    }
    return best;
}

static int aiChooseDir(const GameState &gs) {
    buildDanger(gs);
    memset(firstMove, 0xFF, sizeof(firstMove));

    int pcx = tileX(gs.pac), pcy = tileY(gs.pac);
    int startIdx = pcy * MZ_W + pcx;
    int head = 0, tail = 0;
    firstMove[startIdx] = 0xFE;
    queue[tail++] = (uint16_t)startIdx;

    while (head < tail) {
        int idx = queue[head++];
        int cx = idx % MZ_W, cy = idx / MZ_W;
        if (idx != startIdx &&
            (mazeHasDot(cx, cy) || mazeHasPellet(cx, cy) || frightGhostAt(gs, cx, cy)))
            return firstMove[idx];

        for (int d = 0; d < 4; d++) {
            int nx = mazeWrapX(cx + DX[d]), ny = cy + DY[d];
            if (ny < 0 || ny >= MZ_H) continue;
            if (!mazeOpenForPac(nx, ny)) continue;
            int nidx = ny * MZ_W + nx;
            if (firstMove[nidx] != 0xFF) continue;
            if (dangerGet(nidx)) continue;
            firstMove[nidx] = (firstMove[idx] == 0xFE) ? (uint8_t)d : firstMove[idx];
            if ((unsigned)tail < sizeof(queue) / sizeof(queue[0]))
                queue[tail++] = (uint16_t)nidx;
        }
    }
    return aiFlee(gs);   // nothing safe reachable
}

Input aiPoll(const GameState &gs) {
    Input in;
    in.dir = DIR_NONE;
    in.start = false;

    if (gs.phase == PH_TITLE) {
        in.start = (gs.phaseTimer > 40);    // let the title show ~0.6 s, then press
        return in;
    }
    if (gs.phase != PH_PLAY) return in;

    static int8_t cached = DIR_NONE;
    if ((gs.pac.px % TILE) == 0 && (gs.pac.py % TILE) == 0)   // only at tile centres
        cached = (int8_t)aiChooseDir(gs);
    in.dir = cached;
    return in;
}
