/*----------------------------------------------------------------------------
 * display.cpp  -  4-section multiplex driver, driven by Timer1.
 *
 * Wiring model (from the original README): the row address (A0/A1/A2/CS),
 * clock, strobe and the two enables are GLOBAL; only the red/green DATA lines
 * are per-section. So for one "local row" L (0..15) every section outputs its
 * own row L simultaneously while we clock 200 columns:
 *      section 0 -> display rows  0..15
 *      section 1 -> display rows 16..31
 *      section 2 -> display rows 32..47
 *      section 3 -> display rows 48..63
 *--------------------------------------------------------------------------*/
#include <digitalWriteFast.h>
#include <string.h>
#include "display.h"

uint8_t fbR[DISP_H][FB_BYTES];
uint8_t fbG[DISP_H][FB_BYTES];

static volatile uint8_t curLocalRow = 0;

void dispClear() {
    memset(fbR, 0, sizeof(fbR));
    memset(fbG, 0, sizeof(fbG));
}

// One bit out of a bit-packed framebuffer row.
static inline uint8_t bit(const uint8_t *rowbuf, uint8_t byteIdx, uint8_t mask) {
    return (uint8_t)(rowbuf[byteIdx] & mask);
}

// Scan a single local row (0..15) across all four sections.
static void scanLocalRow(uint8_t L) {
    // Pointers to the four framebuffer rows that share this local row.
    const uint8_t *r0r = fbR[L];        const uint8_t *r0g = fbG[L];
    const uint8_t *r1r = fbR[L + 16];   const uint8_t *r1g = fbG[L + 16];
    const uint8_t *r2r = fbR[L + 32];   const uint8_t *r2g = fbG[L + 32];
    const uint8_t *r3r = fbR[L + 48];   const uint8_t *r3g = fbG[L + 48];

    // Blank outputs while shifting to avoid ghosting during the address change.
    digitalWriteFast(P_EN_R, EN_OFF);
    digitalWriteFast(P_EN_G, EN_OFF);

    for (uint16_t x = 0; x < DISP_W; x++) {
#if COL_REVERSED
        uint16_t col = (uint16_t)(DISP_W - 1 - x);
#else
        uint16_t col = x;
#endif
        uint8_t bi = (uint8_t)(col >> 3);
        uint8_t mk = (uint8_t)(1u << (col & 7));

        // digitalWriteFast needs a compile-time HIGH/LOW, hence the if/else.
        if (bit(r0g, bi, mk)) digitalWriteFast(P_G0, HIGH); else digitalWriteFast(P_G0, LOW);
        if (bit(r0r, bi, mk)) digitalWriteFast(P_R0, HIGH); else digitalWriteFast(P_R0, LOW);
        if (bit(r1g, bi, mk)) digitalWriteFast(P_G1, HIGH); else digitalWriteFast(P_G1, LOW);
        if (bit(r1r, bi, mk)) digitalWriteFast(P_R1, HIGH); else digitalWriteFast(P_R1, LOW);
        if (bit(r2g, bi, mk)) digitalWriteFast(P_G2, HIGH); else digitalWriteFast(P_G2, LOW);
        if (bit(r2r, bi, mk)) digitalWriteFast(P_R2, HIGH); else digitalWriteFast(P_R2, LOW);
        if (bit(r3g, bi, mk)) digitalWriteFast(P_G3, HIGH); else digitalWriteFast(P_G3, LOW);
        if (bit(r3r, bi, mk)) digitalWriteFast(P_R3, HIGH); else digitalWriteFast(P_R3, LOW);

        digitalWriteFast(P_CLK, HIGH);
        digitalWriteFast(P_CLK, LOW);
    }

    // Select the (global) row address: A0..A2 = bits 0..2, CS = bit 3.
    if (L & 0x01) digitalWriteFast(P_A0, HIGH); else digitalWriteFast(P_A0, LOW);
    if (L & 0x02) digitalWriteFast(P_A1, HIGH); else digitalWriteFast(P_A1, LOW);
    if (L & 0x04) digitalWriteFast(P_A2, HIGH); else digitalWriteFast(P_A2, LOW);
    if (L & 0x08) digitalWriteFast(P_CS, HIGH); else digitalWriteFast(P_CS, LOW);

    // Latch the shifted data, then re-enable the outputs.
    digitalWriteFast(P_STR, HIGH);
    digitalWriteFast(P_STR, LOW);

    digitalWriteFast(P_EN_R, EN_ON);
    digitalWriteFast(P_EN_G, EN_ON);
}

ISR(TIMER1_COMPA_vect) {
    scanLocalRow(curLocalRow);
    curLocalRow = (uint8_t)((curLocalRow + 1) & 0x0F);
}

void dispBegin() {
    const uint8_t outPins[] = {
        P_CLK, P_STR, P_A0, P_A1, P_A2, P_CS, P_EN_R, P_EN_G,
        P_R0, P_G0, P_R1, P_G1, P_R2, P_G2, P_R3, P_G3
    };
    for (uint8_t i = 0; i < sizeof(outPins); i++) {
        pinMode(outPins[i], OUTPUT);
        digitalWrite(outPins[i], LOW);
    }
    // Start with the panel blanked until the first row is latched.
    digitalWrite(P_EN_R, EN_OFF);
    digitalWrite(P_EN_G, EN_OFF);

    dispClear();

    // Timer1: CTC, prescaler /8, fire every ROW_SCAN_US.
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;
    OCR1A  = (uint16_t)(ROW_SCAN_TICKS - 1);
    TCCR1B |= (1 << WGM12);              // CTC
    TCCR1B |= (1 << CS11);               // prescaler /8
    TIMSK1 |= (1 << OCIE1A);             // enable compare-A interrupt
    interrupts();
}
