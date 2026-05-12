/**
 * matrix_config.h
 *
 * Pin-Definitionen und Displaykonstanten für die 64×200-Bi-Color-LED-Matrix
 * (Zugzielanzeige, 4 Flachbandkabel-Segmente, 5 V-Logik, Arduino Mega 1280).
 *
 * Verdrahtungsübersicht
 * ─────────────────────
 * Jedes Flachbandkabel trägt die gleichen Steuersignale (Takt, Latch, Adresse,
 * CS, Enable), aber eigene Datenleitungen (Grün + Rot). Durch die Bündelung auf
 * zwei Hardware-Ports (PORTC für Daten, PORTL für Steuerung) lassen sich in
 * einem einzigen Schreibbefehl alle 8 Datenpins oder alle Steuerpins gleichzeitig
 * setzen – wesentlich schneller als einzelne digitalWrite()-Aufrufe.
 *
 * PORTC – Datenpins (Arduino-Mega-Pins 30–37)
 * ────────────────────────────────────────────
 *   PC0 / Pin 37 : Panel 0 – Data Green
 *   PC1 / Pin 36 : Panel 0 – Data Red
 *   PC2 / Pin 35 : Panel 1 – Data Green
 *   PC3 / Pin 34 : Panel 1 – Data Red
 *   PC4 / Pin 33 : Panel 2 – Data Green
 *   PC5 / Pin 32 : Panel 2 – Data Red
 *   PC6 / Pin 31 : Panel 3 – Data Green
 *   PC7 / Pin 30 : Panel 3 – Data Red
 *
 * PORTL – Steuerpins (Arduino-Mega-Pins 42–49)
 * ─────────────────────────────────────────────
 *   PL0 / Pin 49 : CLK   – Schieberegister-Takt
 *   PL1 / Pin 48 : LATCH – Datenübernahme (Strobe)
 *   PL2 / Pin 47 : A0    – Zeilenadresse Bit 0
 *   PL3 / Pin 46 : A1    – Zeilenadresse Bit 1
 *   PL4 / Pin 45 : A2    – Zeilenadresse Bit 2
 *   PL5 / Pin 44 : CS    – Chip-Select (LOW = Zeilen 0–7, HIGH = Zeilen 8–15)
 *   PL6 / Pin 43 : EN1   – Enable / Helligkeit 1
 *   PL7 / Pin 42 : EN2   – Enable / Helligkeit 2
 */

#pragma once
#include <stdint.h>

// ─── Displaygeometrie ─────────────────────────────────────────────────────────

static constexpr uint8_t  MATRIX_NUM_PANELS   = 4;    // Anzahl Flachbandkabel-Segmente
static constexpr uint8_t  MATRIX_ROWS_PANEL   = 16;   // Zeilen pro Segment (A0/A1/A2 + CS)
static constexpr uint8_t  MATRIX_ROWS         = MATRIX_NUM_PANELS * MATRIX_ROWS_PANEL; // 64
static constexpr uint16_t MATRIX_COLS         = 200;  // Spalten gesamt
static constexpr uint8_t  MATRIX_BYTES_ROW    = MATRIX_COLS / 8; // 25 Bytes pro Farbkanal pro Zeile

// ─── Arduino-Pin-Nummern (nur für begin()-Initialisierung via pinMode) ────────

// Datenpins – Panel 0 … 3 (Green, Red)
// Präfix MTX_ vermeidet Konflikte mit Arduino-Framework-Makros (z.B. PIN_A0…A7)
static constexpr uint8_t MTX_DATA_G0 = 37;
static constexpr uint8_t MTX_DATA_R0 = 36;
static constexpr uint8_t MTX_DATA_G1 = 35;
static constexpr uint8_t MTX_DATA_R1 = 34;
static constexpr uint8_t MTX_DATA_G2 = 33;
static constexpr uint8_t MTX_DATA_R2 = 32;
static constexpr uint8_t MTX_DATA_G3 = 31;
static constexpr uint8_t MTX_DATA_R3 = 30;

// Steuerpins
static constexpr uint8_t MTX_CLK   = 49;
static constexpr uint8_t MTX_LATCH = 48;
static constexpr uint8_t MTX_A0    = 47;
static constexpr uint8_t MTX_A1    = 46;
static constexpr uint8_t MTX_A2    = 45;
static constexpr uint8_t MTX_CS    = 44;
static constexpr uint8_t MTX_EN1   = 43;
static constexpr uint8_t MTX_EN2   = 42;

// ─── PORTL-Bitmasks (für direkten Registerzugriff) ───────────────────────────

static constexpr uint8_t PL_CLK   = (1u << 0); // PL0
static constexpr uint8_t PL_LATCH = (1u << 1); // PL1
static constexpr uint8_t PL_A0    = (1u << 2); // PL2
static constexpr uint8_t PL_A1    = (1u << 3); // PL3
static constexpr uint8_t PL_A2    = (1u << 4); // PL4
static constexpr uint8_t PL_CS    = (1u << 5); // PL5
static constexpr uint8_t PL_EN1   = (1u << 6); // PL6
static constexpr uint8_t PL_EN2   = (1u << 7); // PL7

// Maske aller Adress-/Select-Bits (wird beim Setzen der Zeilenadresse maskiert)
static constexpr uint8_t PL_ADDR_MASK = PL_A0 | PL_A1 | PL_A2 | PL_CS;
