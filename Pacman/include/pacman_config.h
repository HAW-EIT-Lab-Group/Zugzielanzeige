/*----------------------------------------------------------------------------
 * pacman_config.h  -  hardware + timing configuration
 *
 * Reuses the ORIGINAL project's hardware definitions from ../ArduinoSketch
 * (this is the "accesses the original scripts" part of the task). We only add
 * what the 200x64 / 4-section panel needs that the original code did not yet
 * implement (data pins for sections 2..4, and the timing constants).
 *
 * Display facts (from README.md + src/main.cpp of the original project):
 *   - 200 x 64 pixels, bi-colour: red + green (=> red / green / orange).
 *   - Built from 4 ribbon cables, each driving a 200 x 16 "section".
 *   - GLOBAL signals shared by all sections: A0,A1,A2,CS,CLK,STR,EN_R,EN_G.
 *   - PER-SECTION signals: only the two data lines (red + green).
 *   - A 74HC238 only holds a row while it is addressed -> we must multiplex
 *     all 16 (local) rows continuously. Here that is done from a timer ISR.
 *--------------------------------------------------------------------------*/
#pragma once

#include <Arduino.h>
#include "../../ArduinoSketch/header.h"   // original SIZE_X/SIZE_Y + section-0 pins

// ----- panel geometry -----
#define DISP_W      SIZE_X      // 200
#define DISP_H      SIZE_Y      // 64
#define SECTIONS    4
#define SECTION_H   16          // rows per ribbon/section (74HC238: 3 addr + CS)

// ----- GLOBAL control pins (shared by all 4 sections) -----
// These match src/main.cpp of the original (tested) firmware.
#define P_CLK   PIN_CLK         // 26
#define P_STR   PIN_STR         // 28
#define P_A0    PIN_A0_S0       // 30
#define P_A1    PIN_A1_S0       // 32
#define P_A2    PIN_A2_S0       // 34
#define P_CS    PIN_CS_S0       // 36
#define P_EN_R  PIN_EN_R        // 38
#define P_EN_G  PIN_EN_G        // 39

// ----- PER-SECTION data pins (red, green) -----
// Section 0 is confirmed (22/24 from the original). Sections 1..3 are NOT wired
// in the original yet (header.h has them as -1), so we assign free Mega pins
// here.  >>> VERIFY THESE MATCH YOUR ACTUAL RIBBON WIRING BEFORE RUNNING <<<
#define P_R0    PIN_DATA_R_S0   // 24  (confirmed)
#define P_G0    PIN_DATA_G_S0   // 22  (confirmed)
#define P_R1    40              // TODO: verify
#define P_G1    41              // TODO: verify
#define P_R2    42              // TODO: verify
#define P_G2    43              // TODO: verify
#define P_R3    44              // TODO: verify
#define P_G3    45              // TODO: verify

// ----- output-enable polarity -----
// src/main.cpp drives EN HIGH to switch a colour ON  -> active-HIGH (default).
// If your image is inverted (whole panel lit / dark), flip this to 0.
#define ENABLE_ACTIVE_HIGH  1
#if ENABLE_ACTIVE_HIGH
  #define EN_ON   HIGH
  #define EN_OFF  LOW
#else
  #define EN_ON   LOW
  #define EN_OFF  HIGH
#endif

// If the picture comes out mirrored left<->right, set this to 1.
#define COL_REVERSED  0

// ----- timing -----
// One local row is scanned per timer interrupt. 16 local rows => one full
// 64-row frame. At 800us/row that is 16*800us = 12.8ms  ->  ~78 Hz refresh,
// rock-steady because it runs from Timer1, independent of the game code.
#define ROW_SCAN_US     800UL
#define ROW_SCAN_TICKS  ((F_CPU / 8UL) * ROW_SCAN_US / 1000000UL)  // Timer1, /8

// Fixed game tick: this is your "15 ms to calculate per frame" budget.
#define FRAME_US        15000UL

// ----- directions (shared) -----
// Tie-break preference order matches the arcade: UP, LEFT, DOWN, RIGHT.
enum { DIR_UP = 0, DIR_LEFT = 1, DIR_DOWN = 2, DIR_RIGHT = 3 };
#define DIR_NONE  (-1)
