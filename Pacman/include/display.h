/*----------------------------------------------------------------------------
 * display.h  -  bit-packed framebuffer + constant-rate multiplex driver
 *
 * The framebuffer is two bit-planes (red, green). 200x64 bits each = 1600 B
 * per plane (3200 B total) -- fits the ATmega1280's 8 KB SRAM, unlike the
 * original header.h's bool[200][64] (25.6 KB, impossible here).
 *
 * dispBegin() starts a Timer1 ISR that scans ONE local row per interrupt.
 * The refresh rate is therefore fixed by the timer and is NOT affected by how
 * long the game logic takes -- exactly the "constant frame rate" requirement.
 *--------------------------------------------------------------------------*/
#pragma once

#include "pacman_config.h"

#define FB_BYTES  ((DISP_W + 7) / 8)   // 25 bytes per row

extern uint8_t fbR[DISP_H][FB_BYTES];
extern uint8_t fbG[DISP_H][FB_BYTES];

void dispBegin();     // configure pins + start the multiplex ISR
void dispClear();     // blank the framebuffer (memset)

// Set a pixel. colour bit0 = red, bit1 = green (so 3 = orange).
static inline void dispSetPixel(int x, int y, uint8_t colour) {
    if ((unsigned)x >= (unsigned)DISP_W || (unsigned)y >= (unsigned)DISP_H) return;
    uint8_t b = (uint8_t)(x >> 3);
    uint8_t m = (uint8_t)(1u << (x & 7));
    if (colour & 1) fbR[y][b] |= m; else fbR[y][b] &= (uint8_t)~m;
    if (colour & 2) fbG[y][b] |= m; else fbG[y][b] &= (uint8_t)~m;
}
