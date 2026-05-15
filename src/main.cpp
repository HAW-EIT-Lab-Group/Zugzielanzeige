/**
 * Zugzielanzeige – Einzelkabel-Test mit digitalWriteFast
 *
 * Kernproblem das vorher nicht funktioniert hat:
 * Der 74HC238-Decoder hält eine Zeile NUR aktiv solange sie adressiert ist.
 * Die Software muss deshalb alle 16 Zeilen permanent im Kreis durchlaufen
 * (Multiplexing). Ein einmaliges setup() reicht nicht.
 *
 * Ziel: ~80 Hz Bildwiederholrate (16 Zeilen × 5ms pro Zeile = 80Hz)
 * Mit digitalWriteFast statt digitalWrite: Faktor 10-20 schneller,
 * da die Pin-Prüfung zur Compile-Zeit erledigt wird.
 *
 * Bibliothek installieren (Arduino IDE):
 *   Sketch → Bibliothek einbinden → Bibliotheken verwalten → "digitalWriteFast"
 */

#include <Arduino.h>
#include <digitalWriteFast.h>

// ─── Pin-Definitionen ─────────────────────────────────────────────────────────

#define PIN_G   22   // Data Green
#define PIN_R   24   // Data Red
#define PIN_CLK 26   // Clock
#define PIN_LAT 28   // Latch

// Arduino Mega belegt PIN_A0-A2 intern – überschreiben
#undef PIN_A0
#undef PIN_A1
#undef PIN_A2
#define PIN_A0  30   // Zeilenadresse Bit 0
#define PIN_A1  32   // Zeilenadresse Bit 1
#define PIN_A2  34   // Zeilenadresse Bit 2
#define PIN_CS  36   // Chip-Select: LOW = Zeilen 0-7, HIGH = Zeilen 8-15
#define PIN_EN1 38   // Enable Rot
#define PIN_EN2 39   // Enable Grün

// ─── Konstanten ───────────────────────────────────────────────────────────────

#define NUM_COLS 200
#define NUM_ROWS  16

// ─── Eine Zeile scannen ───────────────────────────────────────────────────────

/**
 * Ablauf pro Zeile:
 * 1. Enable aus  → verhindert Geisterbilder während Adresswechsel
 * 2. 200 Bits einschieben
 * 3. Zeilenadresse + CS setzen
 * 4. Latch
 * 5. Enable ein → Zeile leuchtet bis zum nächsten Durchlauf
 */
static void scanRow(uint8_t row, bool green, bool red) {
    // 1. Enable aus
    digitalWriteFast(PIN_EN1, LOW);
    digitalWriteFast(PIN_EN2, LOW);

    // 2. 200 Bits einschieben
    for (uint16_t col = 0; col < NUM_COLS; col++) {
        if (green) { digitalWriteFast(PIN_G, HIGH); } else { digitalWriteFast(PIN_G, LOW); }
        if (red)   { digitalWriteFast(PIN_R, HIGH); } else { digitalWriteFast(PIN_R, LOW); }
        digitalWriteFast(PIN_CLK, HIGH);
        digitalWriteFast(PIN_CLK, LOW);
    }

    // 3. Zeilenadresse setzen (A0/A1/A2 = Bits 0-2, CS für obere Hälfte)
    // if/else nötig: digitalWriteFast braucht HIGH/LOW als Compile-Zeit-Konstante
    if (row & 0x01) { digitalWriteFast(PIN_A0, HIGH); } else { digitalWriteFast(PIN_A0, LOW); }
    if (row & 0x02) { digitalWriteFast(PIN_A1, HIGH); } else { digitalWriteFast(PIN_A1, LOW); }
    if (row & 0x04) { digitalWriteFast(PIN_A2, HIGH); } else { digitalWriteFast(PIN_A2, LOW); }
    if (row >= 8)   { digitalWriteFast(PIN_CS, HIGH); } else { digitalWriteFast(PIN_CS, LOW); }

    // 4. Latch – Daten in Ausgangslatch übernehmen
    digitalWriteFast(PIN_LAT, HIGH);
    digitalWriteFast(PIN_LAT, LOW);

    // 5. Enable ein (EN1=Rot, EN2=Grün)
    if (red)   { digitalWriteFast(PIN_EN1, HIGH); } else { digitalWriteFast(PIN_EN1, LOW); }
    if (green) { digitalWriteFast(PIN_EN2, HIGH); } else { digitalWriteFast(PIN_EN2, LOW); }
}

// ─── Arduino-Einstiegspunkte ──────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

    pinModeFast(PIN_G,   OUTPUT);
    pinModeFast(PIN_R,   OUTPUT);
    pinModeFast(PIN_CLK, OUTPUT);
    pinModeFast(PIN_LAT, OUTPUT);
    pinModeFast(PIN_A0,  OUTPUT);
    pinModeFast(PIN_A1,  OUTPUT);
    pinModeFast(PIN_A2,  OUTPUT);
    pinModeFast(PIN_CS,  OUTPUT);
    pinModeFast(PIN_EN1, OUTPUT);
    pinModeFast(PIN_EN2, OUTPUT);

    // Alle Pins sicher auf LOW
    digitalWriteFast(PIN_G,   LOW);
    digitalWriteFast(PIN_R,   LOW);
    digitalWriteFast(PIN_CLK, LOW);
    digitalWriteFast(PIN_LAT, LOW);
    digitalWriteFast(PIN_A0,  LOW);
    digitalWriteFast(PIN_A1,  LOW);
    digitalWriteFast(PIN_A2,  LOW);
    digitalWriteFast(PIN_CS,  LOW);
    digitalWriteFast(PIN_EN1, LOW);
    digitalWriteFast(PIN_EN2, LOW);

    Serial.println(F("Multiplexing gestartet – alle Zeilen Orange"));
}

void loop() {
    // Alle 16 Zeilen so schnell wie möglich durchlaufen.
    // Der 74HC238-Decoder hält eine Zeile nur aktiv solange sie adressiert ist
    // → kein delay, kein warten, permanente Endlosschleife.
    for (uint8_t row = 0; row < NUM_ROWS; row++) {
        scanRow(row, true, true); // true/true = Orange (Grün + Rot)
    }
}
